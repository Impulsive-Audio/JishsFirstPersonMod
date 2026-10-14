#include "native_camera_interop.h"
#include "native_recording.h"
#include "bail_view.h"
#include "stick_look.h"
#include "native_controls.h"
#include "camera_cycle_override.h"
#include <iostream>
#include <stdexcept>
using namespace jishsfirstpersonmod;
using namespace dingosdk::first_person;
namespace jishsfirstpersonmod { thread_local bool raw_controller_read{}; }
namespace {
void check(bool ok,const char* why) { if(!ok) throw std::runtime_error(why); }
Matrix pose(float yaw,float x=0) {
    Matrix m{1,0,0,0,0,1,0,0,0,0,1,0,x,0,0,1};
    const auto a=yaw*std::numbers::pi_v<float>/360;
    write(m,{0,std::sin(a),0,std::cos(a)},{x,0,0});return m;
}
}
int main() {
    check(native_camera_cycle_choice(true,true,false,true,true,false,false,false) &&
        native_camera_cycle_choice(true,true,true,false,true,false,false,false),"native camera cycle entry/exit lost manual intent");
    check(!native_camera_cycle_choice(true,true,true,false,true,true,false,false) &&
        !native_camera_cycle_choice(true,true,true,false,true,false,true,false) &&
        !native_camera_cycle_choice(true,true,true,false,true,false,false,true) &&
        !native_camera_cycle_choice(false,true,true,false,true,false,false,false),"comfort/game-owned transition interpreted as camera cycle choice");
    CameraCycleRequests requests;
    check(!requests.settled(true,false) && requests.attempt(false,0),"enabled native camera cycle was not disabled");
    requests.accepted(false);
    check(!requests.settled(true,true),"queued disable was mistaken for completed restoration");
    check(requests.attempt(true,1),"rapid comfort toggle did not queue restoration immediately");
    requests.accepted(true);
    check(!requests.settled(false,true),"restoration completed before native acknowledgement");
    check(!requests.attempt(true,500) && requests.attempt(true,1001),"delayed/rejected request retry flooded or stopped");
    check(requests.settled(true,true),"native restoration was not acknowledged");
    check(requests.settled(false,false),"already disabled preference unnecessarily changed");
    const auto recorded=read_native_recording([](NativeRecordingInput* input,bool allow_menu) {
        check(allow_menu,"recording did not request stock menu input bypass");
        check(raw_controller_read,"menu recording was neutralized by the gameplay filter");
        input->available=true;input->buttons=0x301;input->device=3;
        input->style=dingosdk::ControllerStyle::dualsense;
        input->keys.fill(~std::uint64_t{}); // Native reader writes the full 48-byte structure.
    });
    check(recorded.available && recorded.buttons==0x301 && recorded.device==3 && recorded.style==dingosdk::ControllerStyle::dualsense,
        "native recording ABI lost buttons/device/controller style");
    check(!raw_controller_read,"recording left gameplay filtering bypassed");
    {
        RecordingInputAccess outer;
        read_native_recording([](NativeRecordingInput*,bool) {check(raw_controller_read,"nested reader lost bypass");});
        check(raw_controller_read,"nested reader cleared outer bypass");
    }
    check(!raw_controller_read,"nested recording left gameplay filtering bypassed");
    const auto unfocused=read_native_recording([](NativeRecordingInput* input,bool) { *input={}; });
    check(!unfocused.available && !unfocused.buttons,"native focus/disconnect gate was bypassed");
    check(native_first_person_toggle("firstperson toggle") &&
          native_first_person_toggle(" \tdebug.first_person \t toggle\n"),"native toggle command not routed");
    check(!native_first_person_toggle("firstperson 1") && !native_first_person_toggle("firstperson 0") &&
          !native_first_person_toggle("firstperson toggle extra") && !native_first_person_toggle("freecam toggle"),"unrelated console command captured");
    check(native_keyboard_binding(0x80000046) && native_keyboard_binding(0x40000000 | 46 | (50<<6)) &&
          !native_keyboard_binding(0x40000000 | 63),"native keyboard profile binding validation failed");
    NativeFirstPersonSettings saved;
    saved.enabled=saved.stabilize=saved.follow_flips=saved.board_only=saved.camera_cycle=true;
    saved.offset={.1f,.2f,.3f};saved.rotation={10,20,30};
    saved.up=12;saved.down=23;saved.left=34;saved.right=45;
    saved.smoothing=60;saved.head_pitch=86;saved.head_roll=5;saved.bob=25;
    const auto normal=effective_native_sample(saved,false);
    check(normal.enabled && normal.stabilize && normal.follow_flips,"normal native features disabled");
    const auto locked=effective_native_sample(saved,true);
    check(!locked.enabled && !locked.stabilize && !locked.follow_flips,"native features fight head lock");
    check(locked.board_only && locked.camera_cycle && normal.camera_cycle && locked.offset==saved.offset && locked.rotation==saved.rotation &&
          locked.up==saved.up && locked.down==saved.down && locked.left==saved.left && locked.right==saved.right &&
          locked.smoothing==60 && locked.head_pitch==86 && locked.head_roll==5 && locked.bob==25,"unrelated native settings changed");
    check(saved.enabled && saved.stabilize && saved.follow_flips,"temporary override changed saved input");
    saved.stabilize=saved.follow_flips=false;
    const auto resumed=effective_native_sample(saved,false);
    check(!resumed.stabilize && !resumed.follow_flips,"restore ignored latest native choice");

    // These protect the actual 1.15 paths, including the behavior the 1.17
    // tandem tests deliberately replaced. Native rotation must not move a hold.
    const auto identity=pose(0);HeadPose head;head.valid=true;head.head=identity;
    BailView bail;bail.remember(identity,identity,head,true);
    bail.apply(identity,head,true,false,1);
    const auto held=bail.apply(pose(90),head,true,false,1.01);
    check(std::abs(matrix_heading(held))<.01,"native rotation leaked into bail hold");
    head.head=pose(0,.2f);
    check(std::abs(bail.attach(identity,head)[12]-.2f)<.001,"attachment no longer follows animated head");
    StickAim aim{true,true,30,0,0};
    check(std::abs(matrix_heading(aim_camera(pose(90),aim))-30)<.01,"camera-only aim accumulated native heading");
    aim.heading=40;
    check(std::abs(matrix_heading(aim_camera(pose(10),aim))-40)<.01,"resumed look uses stale composition anchor");
    std::cout<<"Native sample overrides preserve settings and legacy bail/head/stick behavior passed\n";
}
