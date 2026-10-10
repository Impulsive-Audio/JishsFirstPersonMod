#include "settings.h"
#include "stick_input_filter.h"
#include "camera_transition.h"
#include "camera_movement.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path module_directory(){return std::filesystem::current_path()/"stick-test-data";}
std::filesystem::path data_directory(){return module_directory();}
}
void check(bool ok,const char* message) { if(!ok) { std::cerr<<message<<std::endl; std::exit(1); } }
int main() {
    using namespace jishsfirstpersonmod;using namespace dingosdk::first_person;
    constexpr std::uintptr_t game=0x140000000,parent=0x100000;
    for(const auto& child:player_state_contract::normal_foot_states) {
        check(camera_movement_mode(true,true,parent,parent+child.offset,game+child.vtable,game,504)==1,"normal movement lost custom camera");
        check(camera_movement_mode(true,true,parent,parent+child.offset,game+child.vtable+8,game,504)==0,"changed child vtable allowed custom camera");
    }
    check(camera_movement_mode(true,true,parent,parent+0x900,game+0x1234,game,100)==0,"special offboard animation mistaken for board camera");
    check(camera_movement_mode(false,false,0,0,0,game,100)==0,"invalid state allowed custom camera");
    check(camera_movement_mode(true,false,0,0,0,game,504)==0,"offboard physics alone proved normal walking");
    check(camera_movement_mode(true,false,0,0,0,game,100)==2,"board camera denied");
    check(camera_movement_mode(true,false,0,0,0,game,999)==0,"unknown state allowed custom camera");
    check(normalize_axis(-32768)==-1 && normalize_axis(32767)==1,"axis extremes incorrect");
    auto dead=radial_stick({.1f,.1f},.15f);check(dead.x==0 && dead.y==0,"radial deadzone drift");
    auto diagonal=radial_stick({1,1},.15f);check(std::abs(std::hypot(diagonal.x,diagonal.y)-1)<.00001f,"diagonal faster than straight");
    StickSettings settings;settings.enabled=true;Settings spring;
    StickLookState state;
    check(!state.update(settings,{1,0},true,1,0,0,0,spring).active,"held stick armed without neutral");
    check(state.update(settings,{},true,1,.016,0,0,spring).active,"neutral did not arm");
    auto aim=state.update(settings,{1,0},true,1,.116,0,0,spring);
    check(std::abs(aim.heading+12)<.0001f,"yaw rate incorrect");
    check(!state.update(settings,{1,0},false,1,.132,0,0,spring).capture,"mounting retained stick ownership");
    check(!state.update(settings,{1,0},true,1,.148,0,0,spring).active,"return from board skipped neutral");
    for(int fps:{30,60,120}) {
        StickLookState run;run.update(settings,{},true,1,0,0,0,spring);
        for(int i=1;i<=fps;++i) aim=run.update(settings,{1,0},true,1,double(i)/fps,0,0,spring);
        check(std::abs(aim.heading+120)<.001f,"frame-rate dependent sensitivity");
    }
    state.reset();state.update(settings,{},true,1,0,0,0,spring);
    for(int i=1;i<=120;++i) aim=state.update(settings,{0,1},true,1,i/60.,0,0,spring);
    check(aim.pitch==85,"pitch not clamped");
    auto held=state.update(settings,{},true,1,2.016,0,0,spring);check(held.pitch==85,"release recentered analog view");
    settings.turn_character=true;
    check(!state.update(settings,{1,0},true,1,2.032,0,0,spring).active,"mode switch skipped neutral");
    state.update(settings,{},true,1,2.048,30,10,spring);
    aim=state.update(settings,{1,0},true,1,2.148,30,10,spring);
    check(std::abs(aim.body_heading+2)<.001f && std::abs(aim.heading-18)<.001f,"camera/body heading bias doubled rotation");
    check(!state.update(settings,{},true,2,2.164,0,0,spring).capture ||
        state.update(settings,{},true,2,2.164,0,0,spring).heading==0,"device change retained old view");
    settings.invert_y=true;state.reset();state.update(settings,{},true,1,0,0,0,spring);
    aim=state.update(settings,{0,1},true,1,.1,0,0,spring);check(aim.pitch<0,"invert Y ignored");
    settings.invert_y=false;spring.enabled=true;
    state.reset();state.update(settings,{},true,1,0,0,0,spring);
    aim=state.update(settings,{1,0},true,1,.1,0,0,spring);check(aim.heading<0 && aim.heading>-12,"spring did not smooth analog turn");
    Matrix pose{1,0,0,7,0,1,0,8,0,0,1,9,3,4,5,1};
    StickAim view{true,true,70,15,70};auto camera=aim_camera(pose,view);
    check(std::abs(matrix_heading(camera)-70)<.001f,"absolute camera yaw incorrect");
    auto turned=target(pose,{}, {0,-40});auto same=aim_camera(turned,view);
    check(std::abs(matrix_heading(same)-70)<.001f,"native facing applied twice to view");
    Matrix tilted=pose;
    write(tilted,multiply(orientation(turned),{0,0,std::sin(.4f),std::cos(.4f)}),{3,4,5});
    tilted=target(tilted,{}, {35,0});
    check(aim_camera(tilted,{})==tilted,"inactive stick changed native camera");
    for(float pitch:{-85.f,0.f,35.f,85.f}) for(float yaw:{-170.f,-90.f,0.f,70.f,170.f}) {
        const auto level=aim_camera(tilted,{true,true,yaw,pitch,20});
        check(std::abs(wrap_heading(matrix_heading(level)-yaw))<.001f,"tilted head changed world yaw");
        check(std::abs(level[1])<.00001f,"horizontal look introduced roll");
        check(std::abs(level[9]+std::sin(pitch*std::numbers::pi_v<float>/180.f))<.00001f,"yaw changed viewing pitch");
        check(level[12]==3 && level[13]==4 && level[14]==5 && level[3]==7 && level[7]==8 && level[11]==9,"stick look changed native position/metadata");
        for(unsigned axis:{0u,4u,8u}) check(std::abs(length({level[axis],level[axis+1],level[axis+2]})-1)<.00001f,"camera basis lost normalization");
    }
    auto body=face_heading(pose,90);check(body[12]==3 && body[13]==4 && body[14]==5 && body[3]==7 && body[7]==8 && body[11]==9,"native motion position/metadata changed");
    CameraTransition handoff;
    Settings transition_settings;transition_settings.enabled=true;
    transition_settings.up=transition_settings.down=transition_settings.left=transition_settings.right=30;
    const auto foot=aim_camera(pose,{true,true,90,25,0});
    handoff.apply(foot,transition_settings,1,1,true,0);
    auto board=pose;board[12]+=1;
    auto start=handoff.apply(board,transition_settings,1,2,false,.016);
    check(std::abs(matrix_heading(start)-90)<.001f && std::abs(start[12]-foot[12])<.00001f,"mount handoff snapped");
    auto repeated=handoff.apply(board,transition_settings,1,2,false,.016);
    check(repeated==start,"same-frame callback integrated twice");
    auto progress=handoff.apply(board,transition_settings,1,2,false,.032);
    check(matrix_heading(progress)<90 && matrix_heading(progress)>0,"handoff failed to ease");
    auto reversed=handoff.apply(foot,transition_settings,1,1,true,.048);
    check(std::abs(matrix_heading(reversed)-matrix_heading(progress))<.001f,"reversed transition snapped");
    for(int i=4;i<240;++i) reversed=handoff.apply(foot,transition_settings,1,1,true,i*.016);
    check(reversed==foot,"handoff failed to finish exactly");
    check(handoff.apply(board,transition_settings,1,0,false,4)==board,"bail retained blending");
    auto native_animation=target(pose,{}, {40,120});
    check(handoff.apply(native_animation,transition_settings,1,0,false,4.001)==native_animation,"special animation retained custom transition");
    state.reset();state.update(settings,{},true,1,0,20,20,spring);
    state.update(settings,{1,0},true,1,.1,20,20,spring);
    check(!state.update(settings,{1,0},false,1,.116,20,20,spring).capture,"animation retained stick ownership");
    check(!state.update(settings,{1,0},true,1,.132,80,80,spring).active,"animation exit skipped neutral");
    const auto resumed=state.update(settings,{},true,1,.148,80,80,spring);
    check(resumed.active && resumed.heading==80,"animation exit retained old heading");
    handoff.apply(foot,transition_settings,1,1,true,4.016);
    check(handoff.apply(board,transition_settings,2,2,false,4.032)==board,"new player retained handoff");
    transition_settings.enabled=false;
    check(handoff.apply(foot,transition_settings,2,1,true,4.048)==foot,"disabled spring blended");
    transition_settings.enabled=true;
    handoff.reset();handoff.apply(foot,transition_settings,1,1,true,0);
    handoff.apply(board,transition_settings,1,2,false,.016);
    auto moving=board;moving[12]+=.2f;
    auto followed=handoff.apply(moving,transition_settings,1,2,false,.016);
    check(std::abs(followed[12]-start[12]-.2f)<.00001f,"handoff added lag to native motion");
    const auto transition_at=[&](float strength) {
        CameraTransition blend;auto cfg=transition_settings;cfg.up=cfg.down=cfg.left=cfg.right=strength;
        blend.apply(foot,cfg,1,1,true,0);blend.apply(board,cfg,1,2,false,.016);
        return blend.apply(board,cfg,1,2,false,.116);
    };
    check(matrix_heading(transition_at(70))>matrix_heading(transition_at(35)),"higher handoff strength did not soften mount");
    check(transition_at(0)==board,"zero handoff strength did not switch immediately");
    check(std::abs(matrix_heading(body)-90)<.001f,"native facing target incorrect");
    for(auto kind:{dingosdk::PlayStationPad::dualshock4,dingosdk::PlayStationPad::dualsense}) {
        std::array<std::uint8_t,64> report{};report[0]=1;report[1]=170;report[2]=180;report[3]=220;report[4]=30;
        report[5]=0x20;report[8]=40;report[9]=60;
        const auto before=dingosdk::parse_playstation_report(kind,false,report.data(),report.size());
        check(neutral_sony_stick(kind,report),"Sony report rejected");
        const auto after=dingosdk::parse_playstation_report(kind,false,report.data(),report.size());
        check(before && after && after->right_x==0 && std::abs(after->right_y)<=258 && before->buttons==after->buttons &&
            before->left_x==after->left_x && before->left_y==after->left_y && before->left_trigger==after->left_trigger,
            "Sony stick capture altered unrelated input");
    }
    Config config;config.stick=settings;config.stick.deadzone=.22f;
    config.stick.mount_smoothing=82;config.stick.dismount_smoothing=35;config.stick.smooth_transitions=false;
    auto path=data_directory()/"settings.ini";write_config(path,config);const auto reloaded=read_config(path);
    check(reloaded.stick.enabled && reloaded.stick.turn_character && reloaded.stick.deadzone==.22f,"stick preferences did not persist");
    check(!reloaded.stick.smooth_transitions && reloaded.stick.mount_smoothing==82 && reloaded.stick.dismount_smoothing==35,"independent handoff controls did not persist");
    config.stick.mount_smoothing=101;bool rejected=false;try {validate_config(config);} catch(...) {rejected=true;}
    check(rejected,"invalid handoff strength accepted");
    std::cout<<"Stick axes, timing, neutral handoff, smoothing, native facing and persistence passed\n";
}


