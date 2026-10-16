#include "settings.h"
#include "stick_input_filter.h"
#include "camera_transition.h"
#include "camera_movement.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "look_smoothing.h"
#include "spring_policy.h"
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
    StickLookState recentered;recentered.recenter_facing();
    check(!recentered.update(settings,{1,0},true,1,0,45,90,spring,20).active,"dismount recenter skipped neutral stick");
    const auto centered=recentered.update(settings,{},true,1,.016,45,90,spring,20);
    check(centered.active && centered.heading==-90 && centered.body_heading==90 && centered.pitch==0,
        "dismount/recovery recenter faces into the body");
    const auto facing=recentered.update(settings,{1,0},true,1,.116,45,90,spring);
    check(std::abs(wrap_heading(facing.body_heading-facing.heading-180.f))<.001f,"looking did not turn the character with the view");
    recentered.recenter_facing();
    const auto camera_only=recentered.update(settings,{},true,1,.132,45,90,spring,20);
    check(camera_only.heading==-90 && camera_only.pitch==0,"recenter applied without the mode selector");
    for(const float heading:{-179.f,-90.f,0.f,90.f,179.f}) {
        StickLookState dismounted;dismounted.recenter_facing();
        const auto outward=dismounted.update(settings,{},true,1,0,heading,heading,spring);
        const auto camera=aim_camera(Matrix{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},outward);
        const auto body=face_heading(Matrix{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},heading);
        // Viewing direction is camera -Z; character facing is body +Z.
        const float dot=-(camera[8]*body[8]+camera[9]*body[9]+camera[10]*body[10]);
        check(dot>.999f && std::abs(wrap_heading(outward.body_heading-heading))<.001f,
            "dismount view did not face outward without turning the character");
    }
    StickLookState bail_center;bail_center.recenter_facing(180.f);
    check(!bail_center.update(settings,{1,0},true,1,0,45,90,spring,20).active,"bail recenter skipped neutral stick");
    const auto bail_centered=bail_center.update(settings,{},true,1,.016,45,90,spring,20);
    check(bail_centered.active && bail_centered.heading==-90 && bail_centered.body_heading==90 && bail_centered.pitch==0,
        "bail recenter did not reverse camera while preserving character facing");
    const auto bail_turned=bail_center.update(settings,{1,0},true,1,.116,45,90,spring);
    check(std::abs(wrap_heading(bail_turned.body_heading-bail_turned.heading-180.f))<.001f,
        "bail recenter did not follow the view");
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
    state.reset();state.update(settings,{},true,1,2.048,30,10,spring);
    aim=state.update(settings,{1,0},true,1,2.148,30,10,spring);
    check(std::abs(aim.heading-18)<.001f && std::abs(wrap_heading(aim.body_heading-aim.heading-180.f))<.001f,
        "character did not follow the view while looking");
    // The left stick is walk input only: moving follows the view, and no stick
    // direction can steer the character.
    StickLookState walker;
    const auto walk=walker.update(settings,{},true,1,0,45,10,spring,0,{0,1});
    check(walk.active && walk.manual && std::abs(wrap_heading(walk.body_heading-225.f))<.001f,
        "walking did not follow the view");
    const auto sidestep=walker.update(settings,{},true,1,.016,45,10,spring,0,{-1,0});
    check(std::abs(wrap_heading(sidestep.body_heading-225.f))<.001f,
        "left stick direction changed the character facing");
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

    // On-board look rotates in the skater body's axes over the native camera.
    // Skater body matrices are facing frames: Z row forward, X row left.
    const auto board_pose=[&](float heading,float pitch,float roll) {
        constexpr float half=std::numbers::pi_v<float>/360.f;
        Matrix result=pose;
        write(result,multiply(multiply(multiply({0,std::sin(heading*half),0,std::cos(heading*half)},
            {std::sin(pitch*half),0,0,std::cos(pitch*half)}),{0,0,std::sin(roll*half),std::cos(roll*half)}),{0,1,0,0}),{3,4,5});
        return result;
    };
    const auto close=[](const dingosdk::first_person::Matrix& a,const dingosdk::first_person::Matrix& b) {
        for(unsigned i=0;i<16;++i) if(std::abs(a[i]-b[i])>=.00001f) return false;
        return true;
    };
    const auto spec_board=[&](const Matrix& view,const Matrix& body,float heading,float pitch,bool flipped) {
        const float yaw=heading*std::numbers::pi_v<float>/360.f;
        const float clamped=(flipped ? -1.f:1.f)*std::clamp(pitch,-85.f,85.f)*std::numbers::pi_v<float>/360.f;
        const auto base=multiply(orientation(body),Quat{0,1,0,0});
        const auto look=multiply({0,std::sin(yaw),0,std::cos(yaw)},{std::sin(clamped),0,0,std::cos(clamped)});
        Matrix result=view;
        write(result,normalized(multiply(multiply(multiply(base,look),conjugate(base)),orientation(view))),
            {view[12],view[13],view[14]});
        return result;
    };
    const auto crouch=board_pose(25,30,20);
    const auto view_pose=[&](float heading,float pitch,float roll) {
        constexpr float half=std::numbers::pi_v<float>/360.f;
        Matrix result=pose;
        write(result,multiply(multiply({0,std::sin(heading*half),0,std::cos(heading*half)},
            {std::sin(pitch*half),0,0,std::cos(pitch*half)}),{0,0,std::sin(roll*half),std::cos(roll*half)}),{3,4,5});
        return result;
    };
    const auto head_view=view_pose(-40,15,-5);
    check(right_handed(pose) && right_handed(crouch),"proper frame reported mirrored");
    Matrix mirrored=pose;mirrored[0]=-mirrored[0];mirrored[1]=-mirrored[1];mirrored[2]=-mirrored[2];
    check(!right_handed(mirrored),"mirrored frame passed the handedness check");
    FakieMirror mirror_state{};
    const std::array<dingosdk::first_person::Matrix,2> bodies{pose,crouch};
    for(const auto& body:bodies) for(float pitch:{-85.f,0.f,35.f,85.f})
        for(float heading:{-170.f,-90.f,0.f,70.f,170.f})
            check(aim_board_body_local(head_view,{true,true,heading,pitch,0},body,true,0.75f,true,mirror_state,10.0)==
                spec_board(head_view,body,heading,pitch,false),"board look composition changed");
    const auto wall=board_pose(0,80,10);
    check(aim_board_body_local(head_view,{true,true,70,35,0},wall,true,0.75f,true,mirror_state,10.0)==
        spec_board(head_view,wall,70,35,false),"steep body look composition changed");
    for(float pitch:{-120.f,120.f})
        check(aim_board_body_local(head_view,{true,true,0,pitch,0},crouch,true,0.75f,true,mirror_state,10.0)==
            spec_board(head_view,crouch,0,pitch,false),"board pitch did not clamp");
    const auto capture=aim_board_body_local(head_view,{true,true,0,0,0},crouch,true,0.75f,true,mirror_state,10.0);
    check(close(capture,head_view),"body look moved the camera at capture");
    check(capture[12]==3 && capture[13]==4 && capture[14]==5 && capture[3]==7 && capture[7]==8 && capture[11]==9,
        "body look changed native position/metadata");
    const auto body25=aim_board_body_local(head_view,{true,true,-15,0,0},board_pose(25,30,20),true,0.75f,true,mirror_state,10.0);
    constexpr float half_turn=40.f*std::numbers::pi_v<float>/360.f;
    Matrix moved_view=head_view;
    write(moved_view,multiply({0,std::sin(half_turn),0,std::cos(half_turn)},orientation(head_view)),
        {head_view[12],head_view[13],head_view[14]});
    const auto body65=aim_board_body_local(moved_view,{true,true,-15,0,0},board_pose(65,30,20),true,0.75f,true,mirror_state,10.0);
    check(std::abs(wrap_heading(matrix_heading(body65)-matrix_heading(body25))-40)<.001f,
        "body look did not follow the body's heading");
    const auto rolled=aim_board_body_local(pose,{true,true,0,35,0},board_pose(0,0,90),true,0.75f,true,mirror_state,10.0);
    check(std::abs(wrap_heading(matrix_heading(rolled))-35)<.001f && std::abs(rolled[9])<.00001f,
        "board pitch ignored the body's rolled right axis");
    const auto aligned_view=view_pose(0,0,0);
    const auto pitch_up=aim_board_body_local(aligned_view,{true,true,0,35,0},board_pose(0,0,0),true,0.75f,true,mirror_state,10.0);
    check(!mirror_state.fakie && !mirror_state.easing && std::abs(pitch_up[9]+std::sin(35*std::numbers::pi_v<float>/180.f))<.00001f,
        "regular stance pitch inverted");
    const auto fakie_start=aim_board_body_local(aligned_view,{true,true,0,35,0},board_pose(180,0,0),true,-0.6f,true,mirror_state,20.0);
    check(mirror_state.fakie && mirror_state.easing && std::abs(fakie_start[9]-std::sin(35*std::numbers::pi_v<float>/180.f))<.00001f,
        "fakie ease did not start from the pre-flip view");
    const auto fakie_mid=aim_board_body_local(aligned_view,{true,true,0,35,0},board_pose(180,0,0),true,-0.6f,true,mirror_state,20.1);
    check(std::abs(fakie_mid[9])<.00001f,"fakie ease did not pass through level");
    const auto fakie_end=aim_board_body_local(aligned_view,{true,true,0,35,0},board_pose(180,0,0),true,-0.6f,true,mirror_state,20.25);
    check(!mirror_state.easing && fakie_end==spec_board(aligned_view,board_pose(180,0,0),0,35,true),
        "fakie ease did not settle mirrored");
    const auto hold_state=aim_board_body_local(aligned_view,{true,true,0,35,0},board_pose(0,0,0),true,0.f,false,mirror_state,20.3);
    check(mirror_state.fakie && hold_state==spec_board(aligned_view,board_pose(0,0,0),0,35,true),
        "unreadable stance snapped the pitch axis");
    aim_board_body_local(aligned_view,{},board_pose(0,0,0),true,0.75f,true,mirror_state,20.4);
    check(!mirror_state.fakie && !mirror_state.easing && mirror_state.mirror==1.f,
        "idle stance flip did not settle the mirror");
    const auto yaw_center=aim_board_body_local(aligned_view,{true,true,0,0,0},board_pose(0,0,0),true,0.75f,true,mirror_state,10.0);
    const auto yaw_right=aim_board_body_local(aligned_view,{true,true,-15,0,0},board_pose(0,0,0),true,0.75f,true,mirror_state,10.0);
    check(std::abs(wrap_heading(matrix_heading(yaw_right)-matrix_heading(yaw_center))+15)<.001f,
        "board yaw turned the wrong way");
    check(aim_board_body_local(head_view,{true,true,-15,0,0},crouch,false,0.75f,true,mirror_state,10.0)==head_view,
        "invalid body frame changed the camera");
    check(aim_board_body_local(head_view,{},{},false,0.f,false,mirror_state,10.0)==head_view,"inactive board look changed the camera");
    // On-foot look-down lean: user-calibrated 0.086 m forward and 0.077 m down at full look-down.
    check(look_down_lean(pose,30.f,.086f,.077f)[0]==0 && look_down_lean(pose,30.f,.086f,.077f)[1]==0 && look_down_lean(pose,30.f,.086f,.077f)[2]==0,
        "look-down lean applied while looking up");
    const auto lean_full=look_down_lean(pose,-85.f,.086f,.077f);
    check(std::abs(lean_full[0])<1e-6f && std::abs(lean_full[1]+0.077f)<1e-6f && std::abs(lean_full[2]+0.086f)<1e-6f,
        "look-down lean offsets wrong at full down");
    const auto lean_mid=look_down_lean(pose,-42.5f,.086f,.077f);
    check(std::abs(lean_mid[1]+0.0385f)<1e-6f && std::abs(lean_mid[2]+0.043f)<1e-6f,
        "look-down lean did not scale with pitch");
    // The yaw-anchored head position keeps the calibrated offset level-relative:
    // a pitched head and a level head with the same yaw must anchor identically.
    const Matrix pitched_head=board_pose(0,-80,0);
    const Matrix level_head=board_pose(0,0,0);
    const dingosdk::first_person::Vec3 anchor_offset{0.f,0.05f,-0.084f};
    check(yaw_anchored_position(pitched_head,anchor_offset)==yaw_anchored_position(level_head,anchor_offset),
        "yaw anchor swung the offset with head pitch");
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
    config.board_offset[0]=.11f;config.board_offset[2]=-.13f;config.foot_offset[1]=.22f;config.stick.head_motion=70;
    auto path=data_directory()/"settings.ini";write_config(path,config);const auto reloaded=read_config(path);
    check(reloaded.stick.enabled && reloaded.stick.deadzone==.22f && reloaded.stick.head_motion==70,"stick preferences did not persist");
    check(reloaded.board_offset[0]==.11f && reloaded.board_offset[2]==-.13f && reloaded.foot_offset[1]==.22f,
        "per-mode camera offsets did not persist");
    // Per-mode camera offsets swap between on-foot and on-board memories.
    Config modes;modes.spring.offset={.1f,.2f,.3f};modes.board_offset={.4f,.5f,.6f};modes.foot_offset={.7f,.8f,.9f};
    adopt_mode_offsets(modes,1,2);
    check(modes.spring.offset[0]==.7f && modes.board_offset[2]==.3f,"foot mode did not adopt its stored offsets");
    adopt_mode_offsets(modes,2,1);
    check(modes.spring.offset[0]==.1f && modes.foot_offset[0]==.7f,"board mode did not restore its stored offsets");
    Config fresh;fresh.spring.offset={.1f,.2f,.3f};fresh.board_offset={.4f,.5f,.6f};fresh.foot_offset={.1f,.2f,.3f};
    adopt_mode_offsets(fresh,1,0);
    check(fresh.spring.offset[0]==.1f && fresh.board_offset[0]==.4f,"first mode overwrote the other mode's offsets");
    // The on-foot look stays absolute while scaled, damped head motion rides on top.
    const auto yaw_quat=Quat{0,std::sin(.125f),0,std::cos(.125f)};
    check(rotation_angle(swing_from_level(yaw_quat))<.001f,"world yaw leaked into the head swing");
    const auto tilt_quat=multiply(Quat{std::sin(.1f),0,0,std::cos(.1f)},yaw_quat);
    check(std::abs(rotation_angle(swing_from_level(tilt_quat))-.2f)<.01f,"head tilt missing from the swing");
    StickAim riding;riding.active=true;riding.heading=40;riding.pitch=-10;
    HeadMotionState motion_state;
    Matrix steering_pose=pose;
    write(steering_pose,yaw_quat,{0,0,0});
    const auto level_view=aim_camera_native(steering_pose,riding,false,0.f,0.f,motion_state,0);
    const auto alive_view=aim_camera_native(steering_pose,riding,false,1.f,0.f,motion_state,.016);
    check(std::abs(wrap_heading(matrix_heading(alive_view)-matrix_heading(level_view)))<.001f,
        "native steering yaw doubled into the on-foot view");
    Matrix rolling_pose=pose;
    write(rolling_pose,Quat{0,0,std::sin(.35f),std::cos(.35f)},{0,0,0});
    const auto flat_view=aim_camera_native(rolling_pose,riding,false,0.f,0.f,motion_state,.032);
    const auto rolled_view=aim_camera_native(rolling_pose,riding,false,1.f,0.f,motion_state,.048);
    const auto roll_difference=multiply(conjugate(orientation(flat_view)),orientation(rolled_view));
    check(std::abs(rotation_angle(roll_difference)-.7f)<.01f,"native roll did not pass into the on-foot view");
    const auto locked_view=aim_camera_native(rolling_pose,riding,true,1.f,0.f,motion_state,.064);
    check(rotation_angle(multiply(conjugate(orientation(flat_view)),orientation(locked_view)))<.001f,
        "world lock did not discard the head motion");
    const auto half_view=aim_camera_native(rolling_pose,riding,false,.5f,0.f,motion_state,.08);
    check(std::abs(rotation_angle(multiply(conjugate(orientation(flat_view)),orientation(half_view)))-.35f)<.01f,
        "head motion scale did not halve the roll");
    HeadMotionState damp_state;
    aim_camera_native(steering_pose,riding,false,1.f,0.f,damp_state,.1);
    const auto damped_view=aim_camera_native(rolling_pose,riding,false,1.f,80.f,damp_state,.116);
    check(rotation_angle(multiply(conjugate(orientation(flat_view)),orientation(damped_view)))<.1f,
        "head motion smoothing did not damp the roll");
    check(!reloaded.stick.smooth_transitions && reloaded.stick.mount_smoothing==82 && reloaded.stick.dismount_smoothing==35,"independent handoff controls did not persist");
    config.stick.mount_smoothing=101;bool rejected=false;try {validate_config(config);} catch(...) {rejected=true;}
    check(rejected,"invalid handoff strength accepted");

    // Legacy migration preserves the effective on-foot behavior, then becomes independent.
    const auto migration_path=module_directory()/"migration.ini";
    for(bool linked:{false,true}) for(bool foot_off:{false,true}) {
        { std::ofstream old(migration_path); old<<"[JishsFirstPersonMod]\nspring=1\nstrength_up=12\nstrength_down=23\nstrength_left=34\nstrength_right=45\nlink_smoothing="<<linked
            <<"\nlook_smoothing_initialized=1\nlook_spring=1\nlook_strength_up=21\nlook_strength_down=32\nlook_strength_left=43\nlook_strength_right=54\ndisable_spring_on_foot="<<foot_off<<"\nbald_first_person=1\n"; }
        auto migrated=read_config(migration_path);
        check(migrated.settings_migrated && migrated.stick_smoothing_initialized,"legacy migration not detected");
        check(migrated.stick_spring.enabled==!(linked && foot_off),"legacy on-foot switch changed");
        check(migrated.stick_spring.up==(linked ? 12:21) && migrated.stick_spring.right==(linked ? 45:54),"legacy strengths changed");
        write_config(migration_path,migrated);
        auto roundtrip=read_config(migration_path);
        check(!roundtrip.settings_migrated && roundtrip.stick_spring.up==migrated.stick_spring.up,"migration repeated or lost");
        std::ifstream saved(migration_path);std::stringstream text;text<<saved.rdbuf();
        check(text.str().find("bald_first_person")!=std::string::npos,"restored option not saved");
        roundtrip.look_spring.up=99;roundtrip.spring.up=98;
        check(roundtrip.stick_spring.up==migrated.stick_spring.up,"stick strengths linked after migration");
    }
    for(bool glance_enabled:{false,true}) for(bool stick_enabled:{false,true}) {
        Config independent;independent.link_smoothing=false;independent.look_smoothing_initialized=true;
        independent.look_spring.enabled=glance_enabled;independent.stick_spring.enabled=stick_enabled;independent.stick_smoothing_initialized=true;
        independent.look_spring.up=independent.look_spring.down=independent.look_spring.left=independent.look_spring.right=70;
        independent.stick_spring=independent.look_spring;independent.stick_spring.enabled=stick_enabled;
        LookSmoothing glance;
        glance.apply(pose,glance_settings(independent),{},0,false);
        const auto glancing=glance.apply(pose,glance_settings(independent),{30,0},.1,false);
        check((glancing==target(pose,{}, {30,0}))==!glance_enabled,"glance toggle affected incorrectly");
        for(StickAxes axis: {StickAxes{1,0},StickAxes{-1,0},StickAxes{0,1},StickAxes{0,-1}}) {
            StickLookState a,b;auto direct=independent.stick_spring;direct.enabled=false;
            a.update(settings,{},true,1,0,0,0,independent.stick_spring);b.update(settings,{},true,1,0,0,0,direct);
            auto smoothed=a.update(settings,axis,true,1,.1,0,0,independent.stick_spring);
            auto immediate=b.update(settings,axis,true,1,.1,0,0,direct);
            check((std::abs(smoothed.heading-immediate.heading)<.0001f && std::abs(smoothed.pitch-immediate.pitch)<.0001f)==!stick_enabled,"stick directional smoothing toggle failed");
        }
        SettingsStore independent_store(migration_path,independent);Command smoothing;smoothing.kind=CommandKind::stick_smoothing;
        smoothing.spring=independent.stick_spring;smoothing.spring.up=36;check(independent_store.submit(smoothing,0),"stick command rejected");
        check(independent_store.snapshot().config.look_spring.up==70,"stick command changed glance settings");
        independent_store.flush_due(1000);auto saved=read_config(migration_path);
        check(saved.stick_spring.up==36 && saved.look_spring.enabled==glance_enabled && saved.stick_spring.enabled==stick_enabled,"independent settings roundtrip failed");
        auto foot_config=camera_config_for_state(saved,true,1);
        check(foot_config.stick_spring.up==36,"on-foot spring policy changed stick smoothing");
        smoothing.kind=CommandKind::apply_preset;smoothing.preset=saved;
        check(independent_store.submit(smoothing,1000) && independent_store.snapshot().config.stick_spring.up==36,"preset lost independent smoothing");
    }
    {auto invalid=Config{};invalid.stick_spring.up=101;bool failed=false;try{validate_config(invalid);}catch(...){failed=true;}check(failed,"invalid stick smoothing accepted");}

    std::cout<<"Stick axes, timing, neutral handoff, smoothing, native facing and persistence passed\n";
}


