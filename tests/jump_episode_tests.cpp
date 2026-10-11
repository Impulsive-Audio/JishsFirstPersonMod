#include "jump_state_contract.h"
#include "camera_policy.h"
#include "bail_view.h"
#include "camera_transition.h"
#include <iostream>
#include <stdexcept>
#include <map>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
using namespace jishsfirstpersonmod;
using namespace dingosdk::first_person;
void check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
Matrix pose(float x,float y=0) { return {1,0,0,0,0,1,0,0,0,0,1,0,x,y,0,1}; }
JumpEpisodeEvidence evidence(JumpStage stage,float x=0) {
    JumpEpisodeEvidence e;e.owner={1,2,3,4};e.valid=e.position_valid=true;e.stage=stage;e.position={x,0,0};return e;
}
void contract_test() {
    namespace jc=jump_state_contract;
    std::map<std::uintptr_t,std::vector<unsigned char>> bytes;
    const auto add=[&](std::uintptr_t site,const auto& prefix) { bytes[site]={prefix.begin(),prefix.end()}; };
    add(jc::trajectory_constructor_site,jc::trajectory_constructor_prefix);add(jc::ragdoll_constructor_site,jc::ragdoll_constructor_prefix);
    add(jc::trajectory_update_site,jc::trajectory_update_prefix);add(jc::ragdoll_update_site,jc::ragdoll_update_prefix);
    const auto pointer=[&](const auto& child) { auto& data=bytes[child.vtable+0x18];data.resize(8);std::memcpy(data.data(),&child.update,8); };
    pointer(jc::trajectory);pointer(jc::simulated_ragdoll);
    const auto read=[&](std::uintptr_t site,void* out,std::size_t size) {
        const auto it=bytes.find(site);if(it==bytes.end() || it->second.size()!=size) return false;
        std::memcpy(out,it->second.data(),size);return true;
    };
    check(jc::verified(read,0),"reviewed jump contracts rejected");
    for(auto& [site,data]:bytes) {
        data[0]^=1;check(!jc::verified(read,0),"changed constructor/update/vtable accepted");data[0]^=1;
    }
    constexpr std::uintptr_t parent=0x10000,base=0x140000000;
    check(jc::stage(true,parent,parent+jc::trajectory.offset,base+jc::trajectory.vtable,base,true)==JumpStage::trajectory,"trajectory identity failed");
    check(jc::stage(true,parent,parent+jc::trajectory.offset+8,base+jc::trajectory.vtable,base,true)==JumpStage::unknown,"wrong child ownership accepted");
    check(jc::stage(true,parent,parent+jc::simulated_ragdoll.offset,base+jc::simulated_ragdoll.vtable,base,false)==JumpStage::unknown,"unverified expanded state enabled");
}
std::vector<std::string> fields(const std::string& line) {
    std::istringstream in(line);std::vector<std::string> result;std::string field;
    while(std::getline(in,field,',')) result.push_back(field);return result;
}
void replay(const char* path) {
    std::ifstream in(path);check(bool(in),"gameplay trace missing");std::string line;std::getline(in,line);
    const auto header=fields(line);std::map<std::string,std::size_t> columns;
    for(std::size_t i=0;i<header.size();++i) columns[header[i]]=i;
    JumpEpisodeTracker tracker;unsigned covered=0,simulated=0,trajectory=0,pending_hippy=0;
    JumpStage previous_stage=JumpStage::unknown;
    std::uintptr_t image_base{};
    while(std::getline(in,line)) {
        const auto row=fields(line);if(row.size()!=header.size()) continue;
        const auto number=[&](const char* name) { return std::stod(row.at(columns.at(name))); };
        if(number("time_ms")<78916000) continue;
        const auto vt=static_cast<std::uintptr_t>(number("child_vtable"));
        if(!image_base && number("jumping")) image_base=vt-player_state_contract::normal_foot_states[2].vtable;
        // The captured executable's image base is 0x140000000; the first Falling
        // observation independently checks it before replay assertions below.
        const auto stage_base=image_base ? image_base:std::uintptr_t{0x140000000};
        JumpStage stage=JumpStage::unknown;
        if(number("mode")==2) stage=JumpStage::board;
        else {
            if(vt==stage_base+player_state_contract::normal_foot_states[0].vtable || vt==stage_base+player_state_contract::normal_foot_states[1].vtable) stage=JumpStage::ground;
            if(vt==stage_base+player_state_contract::normal_foot_states[2].vtable) stage=JumpStage::falling;
            if(vt==stage_base+jump_state_contract::trajectory.vtable) stage=JumpStage::trajectory;
            if(vt==stage_base+jump_state_contract::simulated_ragdoll.vtable) stage=JumpStage::simulated_ragdoll;
        }
        auto e=evidence(stage);e.position={float(number("root_x")),float(number("root_y")),float(number("root_z"))};
        e.position_valid=number("head_valid")!=0;e.valid=number("state_valid") && number("animation_valid");
        e.bail=number("bail_latched") || number("wipeout_request") || number("animation_wipeout") || number("recovery");
        e.hippy.active=number("hippy_active")!=0;e.hippy.candidate=number("hippy_candidate")!=0;
        const auto prior_stage=previous_stage;previous_stage=stage;
        const auto episode=tracker.update(e,number("time_ms")/1000.);
        if(number("disable_foot") || !number("lock_mount_jump") || !number("lock_hippy_jump") || e.bail) continue;
        if((stage==JumpStage::trajectory || stage==JumpStage::simulated_ragdoll) && row[columns.at("attachment_reason")]=="none" && !e.hippy.candidate) {
            // This captured hippy departure precedes the controller's candidate
            // update by one camera observation. It must not use the ordinary switch.
            if(stage==JumpStage::trajectory && prior_stage==JumpStage::board && !episode.ordinary) { ++pending_hippy;continue; }
            if(!episode.ordinary) throw std::runtime_error("Replay gap at "+row[columns.at("time_ms")]+" stage="+jump_stage_name(stage)+
                " type="+episode.type+" reset="+episode.reset_reason);
            ++covered;
            if(stage==JumpStage::trajectory) ++trajectory;else ++simulated;
        }
    }
    check(image_base && covered+pending_hippy>=57 && simulated>=51,"replay did not cover the reproduced gaps");
    std::cout<<"Gameplay replay covers "<<trajectory<<" trajectory and "<<simulated<<" simulated-ragdoll gaps; "<<pending_hippy<<" hippy departure awaits confirmation\n";
}
int run(int argc,char** argv) {
    if(argc==2) { replay(argv[1]);return 0; }
    contract_test();
    for(bool foot:{false,true}) for(bool jump:{false,true}) for(bool hippy:{false,true}) {
        Config config;config.disable_spring_on_foot=foot;config.stick.enabled=true;
        config.stick.lock_mount_jump=jump;config.stick.lock_hippy_jump=hippy;
        ComfortSnapshot owner;owner.state_valid=owner.animation_valid=true;
        JumpEpisodeTracker tracker;tracker.update(evidence(JumpStage::ground),0);
        BailView view;HeadPose head;head.valid=true;head.root=head.head=pose(0);
        view.apply(pose(.1f),head,false,true,0);view.remember(pose(.1f),pose(.1f),head,true);
        CameraTransition transition;Settings handoff;handoff.enabled=true;
        transition.apply(pose(.1f),handoff,1,1,false,0);
        double time=.016;float x=.2f;
        for(auto stage:{JumpStage::trajectory,JumpStage::falling,JumpStage::simulated_ragdoll,JumpStage::falling,JumpStage::trajectory}) {
            const auto episode=tracker.update(evidence(stage,x),time);check(episode.ordinary,"ordinary episode lost a verified stage");
            check(tracker.update(evidence(stage,x),time).ordinary,"duplicate observation changed episode");
            CameraMovementSample movement;movement.mode=1;movement.custom_mode=stage==JumpStage::falling ? 1:0;
            movement.non_bail=true;movement.jump_episode=episode;movement.jumping=stage==JumpStage::falling;
            check(head_position_attached(config,owner,movement)==jump,"jump switch lost ownership through episode");
            head.root=head.head=pose(x);const auto native=pose(x+.3f);auto output=view.apply(native,head,false,false,time);
            if(head_position_attached(config,owner,movement)) output=view.attach(output,head);
            check(jump || output[12]==native[12],"disabled attachment modified native position");
            const bool eligible=camera_look_allowed(config,owner,movement);
            output=transition.apply(output,handoff,1,1,false,time,{std::uintptr_t(stage),0,0,0},&config.spring,nullptr,jump,eligible,!eligible,false);
            check(!jump || std::abs(output[12]-(x+.1f))<.00001f,"final camera failed to follow calibrated head through transition");
            auto off=config;off.stick.lock_mount_jump=false;
            check(!head_position_attached(off,owner,movement),"live jump switch off retained attachment");
            movement.hippy.active=true;
            check(head_position_attached(config,owner,movement)==hippy,"hippy ownership used ordinary jump switch");
            movement.mounting=true;check(head_position_attached(config,owner,movement)==hippy,"mounting overrode hippy switch");
            movement.hippy.active=false;movement.hippy.candidate=true;
            check(!head_position_attached(config,owner,movement),"unconfirmed hippy acquired attachment");
            auto compat=config;compat.look_only=true;
            check(!head_position_attached(compat,owner,movement),"compatibility mode acquired episode attachment");
            time+=.016;x+=.2f;
        }
        check(!tracker.update(evidence(JumpStage::ground),time).ordinary,"landing retained episode");
        check(!tracker.update(evidence(JumpStage::simulated_ragdoll),time+.016).ordinary,"standalone ragdoll started episode");
    }
    for(int reason=0;reason<9;++reason) {
        JumpEpisodeTracker tracker;tracker.update(evidence(JumpStage::ground),0);
        check(tracker.update(evidence(JumpStage::trajectory),.016).ordinary,"episode setup failed");
        auto e=evidence(JumpStage::simulated_ragdoll);double time=.032;
        if(reason==0) e.bail=true;
        if(reason==1) e.stage=JumpStage::board;
        if(reason==2) e.stage=JumpStage::unknown;
        if(reason==3) e.valid=false;
        if(reason==4) e.owner[2]=99;
        if(reason==5) time=.3;
        if(reason==6) e.position[0]=6;
        if(reason==7) e.position_valid=false;
        if(reason==8) time=-.1;
        const auto state=tracker.update(e,time);
        check(!state.ordinary && std::string_view(state.reset_reason)!="none","lifecycle reset lost reason or retained episode");
    }
    JumpEpisodeTracker hippy_tracker;hippy_tracker.update(evidence(JumpStage::board),0);
    auto e=evidence(JumpStage::trajectory);e.hippy.candidate=true;
    check(std::string_view(hippy_tracker.update(e,.016).type)=="hippy_candidate","candidate episode missing");
    e.stage=JumpStage::falling;e.hippy.candidate=false;e.hippy.active=true;
    check(!hippy_tracker.update(e,.032).ordinary,"confirmed hippy armed ordinary episode");
    e.stage=JumpStage::simulated_ragdoll;
    check(std::string_view(hippy_tracker.update(e,.048).type)=="hippy_jump","hippy episode lost ragdoll stage");
    std::cout<<"Jump contract rejection, full-stage attachment, independent switches and lifecycle resets passed\n";
    return 0;
}
int main(int argc,char** argv) {
    try { return run(argc,argv); }
    catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
