#include "bail_camera_trace.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <algorithm>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path data_directory() { return std::filesystem::current_path()/"bail-trace-test-data"; }
bool stock_first_person_requested() { return true; }
bool stock_first_person_active() { return true; }
int stock_camera_spring_enabled() { return 0; }
}
void check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
std::vector<std::string> fields(const std::string& text) {
    std::vector<std::string> result;std::istringstream input(text);std::string value;
    while(std::getline(input,value,',')) result.push_back(value);
    return result;
}
int main() {
    using namespace jishsfirstpersonmod;using namespace dingosdk::first_person;
    const auto path=data_directory()/"bail-camera-trace-v3.csv";
    std::filesystem::remove(path);
    const Matrix pose{1,0,0,0,0,1,0,0,0,0,1,0,1,2,3,1};
    ComfortSnapshot state;state.core=1; // invalid animation identity must not be dereferenced
    CameraMovementSample movement;movement.mode=2;movement.state[0]=100;
    Config config;config.spring.enabled=true;
    record_bail_camera(pose,pose,state,movement,config,{},0,0);
    movement.bail_recovery=true;
    record_bail_camera(pose,pose,state,movement,config,{},0,0);
    check(!std::filesystem::exists(path),"camera callback performed file I/O");
    flush_bail_camera_trace();
    std::ifstream input(path);std::string line;std::getline(input,line);const auto header=fields(line);
    const auto column=[&](const char* name) { auto it=std::find(header.begin(),header.end(),name);check(it!=header.end(),"CSV column missing");return static_cast<std::size_t>(it-header.begin()); };
    column("reconciling");column("reconcile_remaining_deg");column("manual_takeover");column("facing_eligible");
    unsigned count=0;std::vector<std::string> row;
    while(std::getline(input,line)) { row=fields(line);check(row.size()==header.size(),"CSV field count mismatch");++count; }
    check(count==2,"pre-bail context not captured");
    check(row[column("phase")]=="1" && row[column("effective_spring")]=="0" && row[column("saved_spring")]=="1",
        "trace lost effective/preferred Spring distinction");
    check(row[column("head_valid")]=="0" && row[column("camera_head_distance_m")]=="-1","missing animation data looked valid");
    check(row[column("custom_position_delta_m")]=="0" && row[column("custom_angle_delta_deg")]=="0","native passthrough was logged as a custom change");
    input.close();
    const auto size=std::filesystem::file_size(path);
    for(int i=0;i<600;++i) {
        movement.bail_recovery=false;record_bail_camera(pose,pose,state,movement,config,{},0,0);
        movement.bail_recovery=true;record_bail_camera(pose,pose,state,movement,config,{},0,0);
    }
    check(std::filesystem::file_size(path)==size,"queued diagnostics wrote on camera thread");
    flush_bail_camera_trace();
    input.open(path);std::getline(input,line);count=0;
    while(std::getline(input,line)) { row=fields(line);check(row.size()==header.size(),"buffered CSV row malformed");++count; }
    check(count<=1026 && std::stoul(row[column("dropped")])>0,"diagnostic queue was unbounded");
    input.close();
    // Oversized existing traces rotate; recording continues in a fresh CSV.
    std::filesystem::resize_file(path,8*1024*1024+1);
    movement.bail_recovery=false;record_bail_camera(pose,pose,state,movement,config,{},0,0);
    movement.bail_recovery=true;record_bail_camera(pose,pose,state,movement,config,{},0,0);
    check(std::filesystem::file_size(path)==8*1024*1024+1,"rotation ran on camera thread");
    flush_bail_camera_trace();
    check(std::filesystem::exists(std::filesystem::path(path.string()+".1")),"large trace not archived");
    check(std::filesystem::file_size(path)<8*1024*1024,"new trace not bounded");
    input.open(path);std::getline(input,line);check(fields(line)==header,"new CSV lacks correct header");
    check(bool(std::getline(input,line)) && fields(line).size()==header.size(),"new trace lost samples");input.close();
    const auto before=std::filesystem::file_size(path);
    movement.bail_recovery=false;record_bail_camera(pose,pose,state,movement,config,{},0,0);
    movement.bail_recovery=true;record_bail_camera(pose,pose,state,movement,config,{},0,0);
    flush_bail_camera_trace();check(std::filesystem::file_size(path)>before,"capture stopped after rotation");
    std::cout<<"Bail trace pre-context, deferred I/O, queue bounds and rotation and continued capture passed\n";
}
