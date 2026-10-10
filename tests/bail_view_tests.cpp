#include "bail_view.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
using namespace jishsfirstpersonmod;using namespace dingosdk::first_person;
void check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
float difference(const Matrix& a,const Matrix& b) { return length(rotation_vector(multiply(conjugate(orientation(a)),orientation(b)))); }
Matrix pose(float yaw=0,float x=0) { Matrix result{1,0,0,0,0,1,0,0,0,0,1,0,x,0,0,1};return target(result,{}, {0,yaw}); }
HeadPose head(float x=0) { HeadPose result;result.valid=true;result.head=pose(0,x);result.root=result.head;return result; }
std::vector<std::string> fields(const std::string& line) { std::vector<std::string> values;std::istringstream in(line);std::string word;while(std::getline(in,word,',')) values.push_back(word);return values; }
void replay(const char* path) {
    std::ifstream file(path);check(bool(file),"trace cannot be read");std::string line;std::getline(file,line);
    auto names=fields(line);std::map<std::string,unsigned> columns;for(unsigned i=0;i<names.size();++i) columns[names[i]]=i;
    BailView view;std::string event;Matrix held{};bool have_hold=false;unsigned rows=0,bail_rows=0,events=0,attached_rows=0;
    while(std::getline(file,line)) {
        auto values=fields(line);auto value=[&](const std::string& key) { return std::stod(values.at(columns.at(key))); };
        if(values[columns.at("event")]!=event) { event=values[columns.at("event")];view.reset();have_hold=false;++events; }
        auto matrix=[&](const std::string& prefix) { auto result=pose();write(result,{static_cast<float>(value(prefix+"qx")),static_cast<float>(value(prefix+"qy")),static_cast<float>(value(prefix+"qz")),static_cast<float>(value(prefix+"qw"))}, {static_cast<float>(value(prefix+"x")),static_cast<float>(value(prefix+"y")),static_cast<float>(value(prefix+"z"))});return result; };
        const auto native=matrix("native_");HeadPose current;current.valid=value("head_valid")!=0;current.head=matrix("head_");current.root=matrix("root_");
        const bool bail=value("bail_latched") || value("wipeout_request") || value("animation_wipeout") || value("recovery");
        const bool normal=value("state_valid") && value("animation_valid") && value("custom_mode") && !bail;
        const auto output=view.apply(native,current,bail,normal,value("time_ms")/1000.);
        if(bail) {
            if(!have_hold) { held=output;have_hold=true; }
            check(view.info().blocked && difference(output,held)<.00001f,"trace bail orientation was not held");++bail_rows;
        }
        if(view.info().attached) { ++attached_rows;check(length(subtract({output[12],output[13],output[14]},{current.head[12],current.head[13],current.head[14]}))<=.40001f,"trace head attachment drifted"); }
        if(!view.info().blocked) view.remember(value("phase")==0 ? matrix("final_"):output,native,current,normal);
        ++rows;
    }
    check(events>=6 && bail_rows>250,"six-bail replay incomplete");
    std::cout<<"Replayed "<<events<<" bail events, "<<rows<<" samples, "<<bail_rows<<" held bail samples, "<<attached_rows<<" head-attached samples\n";
}
int main(int argc,char** argv) {
    if(argc==2) { replay(argv[1]);return 0; }
    BailView view;auto base=pose(20,.1f);view.apply(base,head(),false,true,0);view.remember(base,base,head(),true);
    auto moving_head=head(2);const auto held=view.apply(pose(170,4),moving_head,true,false,.016);
    check(view.info().blocked && view.info().attached && std::abs(held[12]-2.1f)<.00001f,"head attachment did not move directly");
    check(difference(base,held)<.00001f,"bail direction changed");
    auto revised=pose(-170,4);revised[3]=.123f;
    auto same=view.apply(revised,moving_head,true,false,.016);
    check(difference(held,same)<.00001f && same[3]==.123f,"same-frame hold or metadata failed");
    HeadPose missing;auto fallback=view.apply(pose(90,3),missing,true,false,.032);
    check(fallback[12]==3 && !view.info().attached,"missing head data did not use native position");
    auto restored=view.apply(pose(90,3),head(2),false,true,.048);
    check(!view.info().blocked && view.info().phase==0 && difference(restored,pose(90))<.00001f,
        "bail clear did not restore camera immediately");
    auto unstable=view.apply(pose(-90,3),head(2),false,false,.064);
    check(!view.info().blocked && difference(unstable,pose(-90))<.00001f,"angular instability delayed restored control");
    auto reheld=view.apply(pose(170,3),head(2),true,false,.080);
    check(view.info().blocked && view.info().phase==1,"renewed bail did not hold");
    view.reset();auto distant=pose(0,1);view.apply(distant,head(),false,true,0);view.remember(distant,distant,head(),true);
    auto rejected=view.apply(pose(90,2),head(1),true,false,.016);
    check(!view.info().calibrated && rejected[12]==2,"invalid offset calibration was accepted");
    view.reset();view.apply(base,head(),false,true,0);view.remember(base,base,head(),true);
    auto attached=view.attach(pose(45,10),head(2));
    check(std::abs(attached[12]-2.1f)<.00001f && difference(attached,pose(45))<.00001f,"hippy attachment changed orientation or lagged position");
    view.apply(pose(179,2),head(2),true,false,.016);
    view.apply(pose(-179,2),head(2),false,true,.032);
    check(!view.info().blocked,"yaw wrap delayed immediate release");
    view.apply(pose(0,20),head(20),false,true,.048);
    check(!view.info().blocked,"teleport reused bail view");
    view.reset();view.apply(base,head(),false,true,0);view.remember(base,base,head(),true);
    view.apply(pose(60,2),head(2),true,false,.016);
    for(int i=2;i<=40;++i) view.apply(pose(60,2),head(2),false,true,i*.016);
    check(!view.info().blocked,"settled recovery never released camera");
    std::cout<<"Bail hold, attachment, immediate release, metadata and lifecycle tests passed\n";
}
