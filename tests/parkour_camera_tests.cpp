#include "camera_policy.h"
#include "bail_view.h"
#include <iostream>
#include <stdexcept>
using namespace jishsfirstpersonmod;using namespace dingosdk::first_person;
void check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
Matrix pose(float yaw=0,float x=0) { Matrix value{1,0,0,0,0,1,0,0,0,0,1,0,x,0,0,1};return target(value,{}, {0,yaw}); }
float angle(const Matrix& a,const Matrix& b) { return length(rotation_vector(multiply(conjugate(orientation(a)),orientation(b)))); }
int main() {
    Config config;config.stick.enabled=config.stick.turn_character=true;
    ComfortSnapshot owner;owner.state_valid=owner.animation_valid=true;owner.normal_foot=true;
    CameraMovementSample movement;movement.mode=movement.custom_mode=1;movement.non_bail=true;
    check(camera_overrides_allowed(owner,movement) && character_facing_allowed(owner,movement),"normal-ground eligibility failed");
    movement.custom_mode=0;owner.normal_foot=false;owner.suspended=true;
    check(!camera_look_allowed(config,owner,movement),"default did not retain native climbing look");
    config.stick.look_during_parkour=true;
    check(camera_look_allowed(config,owner,movement) && head_position_attached(config,owner,movement) &&
        !character_facing_allowed(owner,movement),"validated climbing could not look camera-only");
    movement.custom_mode=1;movement.jumping=true;
    config.stick.look_during_parkour=false;
    check(!camera_look_allowed(config,owner,movement) && head_position_attached(config,owner,movement),
        "disabled parkour look changed jump attachment or captured input");
    config.stick.look_during_parkour=true;
    check(camera_look_allowed(config,owner,movement),"enabled parkour look blocked jumping");
    check(camera_overrides_allowed(owner,movement) && head_position_attached(config,owner,movement) &&
        !character_facing_allowed(owner,movement),"jump position attachment disabled look or turned body");
    movement.hippy.active=true;config.stick.lock_hippy_jump=true;
    check(camera_overrides_allowed(owner,movement) && head_position_attached(config,owner,movement),"hippy attachment disabled camera look");
    config.stick.look_during_parkour=false;
    check(!camera_look_allowed(config,owner,movement),"disabled parkour look captured hippy input");
    config.stick.look_during_parkour=true;
    check(camera_look_allowed(config,owner,movement),"enabled parkour look blocked hippy input");
    config.stick.lock_hippy_jump=false;movement.hippy.candidate=false;
    check(!action_camera_locked(config,movement),"hippy toggle was not independent");
    movement={};movement.mode=movement.custom_mode=2;movement.non_bail=true;
    check(camera_overrides_allowed(owner,movement) && !character_facing_allowed(owner,movement) &&
        !action_camera_locked(config,movement),"board airtime took character or jump ownership");
    movement.mode=0;check(!camera_overrides_allowed(owner,movement),"invalid ownership allowed camera control");
    movement.mode=1;movement.bail_recovery=true;
    check(!camera_overrides_allowed(owner,movement),"active bail allowed camera controls");
    movement.bail_recovery=false;movement.view_hold=true;
    check(camera_overrides_allowed(owner,movement),"stale hold marker delayed input restoration");
    movement.mode=movement.custom_mode=1;movement.non_bail=true;movement.mounting=true;
    for(bool parkour:{false,true}) for(bool lock:{false,true}) {
        config.stick.look_during_parkour=parkour;config.stick.lock_mount_jump=lock;
        check(!camera_look_allowed(config,owner,movement) && !character_facing_allowed(owner,movement),
            "native mounting flag failed to release custom look/facing");
        movement.mounting=false;
        check(camera_look_allowed(config,owner,movement),"mount flag clearing left a custom mounting latch");
        movement.mounting=true;
    }
    movement.mounting=false;
    StickLookState stick;Settings immediate;StickSettings settings;settings.enabled=settings.turn_character=true;
    stick.seed(settings,1,0,40,15,100);
    auto before=stick.update(settings,{1,0},true,1,.016,40,100,immediate);
    // Eligibility remains true across ground, climbing and jumping. User mode stays unchanged.
    auto climb=stick.update(settings,{1,0},true,1,.032,40,140,immediate);
    check(before.active && climb.active && climb.heading!=before.heading,"parkour transition required neutral or reset aim");
    stick.rebase_facing(170);
    auto grounded=stick.update(settings,{},true,1,.032,40,170,immediate);
    check(std::abs(wrap_heading(grounded.body_heading-170))<.00001f,"ground re-entry used stale character bias");
    stick.seed(settings,1,.048,80,20,170);
    auto resumed=stick.update(settings,{1,0},true,1,.048,0,0,immediate);
    check(resumed.active && resumed.manual && resumed.heading==80 && resumed.pitch==20,"held input could not resume from shown view");
    stick.update(settings,{1,0},false,1,.064,80,0,immediate);
    check(!stick.update(settings,{1,0},true,2,.080,80,0,immediate).active,"device/focus reset bypassed neutral");
    StickLookState reentry;
    check(!reentry.update(settings,{1,0},true,1,0,65,100,immediate,27).active,
        "resumed analog look bypassed neutral entry");
    const auto from_display=reentry.update(settings,{},true,1,.016,65,100,immediate,27);
    check(from_display.active && from_display.heading==65 && from_display.pitch==27,
        "analog re-entry recentered displayed pitch");
    std::cout<<"Parkour look toggle, attachment separation, facing ownership and neutral entry passed\n";
}
