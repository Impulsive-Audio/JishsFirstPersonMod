#include "camera_policy.h"
#include "bail_view.h"
#include <iostream>
#include <stdexcept>
using namespace jishsfirstpersonmod;using namespace dingosdk::first_person;
void check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
Matrix pose(float yaw=0,float x=0) { Matrix value{1,0,0,0,0,1,0,0,0,0,1,0,x,0,0,1};return target(value,{}, {0,yaw}); }
float angle(const Matrix& a,const Matrix& b) { return length(rotation_vector(multiply(conjugate(orientation(a)),orientation(b)))); }
int main() {
    // Exercise the actual attachment decision with both fallback paths active,
    // independently of each switch, and apply it to a moving anatomical head.
    for(bool disable_foot:{false,true}) for(bool special:{false,true})
        for(bool jump_lock:{false,true}) for(bool hippy_lock:{false,true}) {
        Config cfg;cfg.stick.enabled=true;cfg.disable_spring_on_foot=disable_foot;
        cfg.stick.lock_mount_jump=jump_lock;cfg.stick.lock_hippy_jump=hippy_lock;
        cfg.stick.look_during_parkour=true;
        ComfortSnapshot state;state.state_valid=state.animation_valid=true;
        CameraMovementSample action;action.mode=1;action.custom_mode=special ? 0:1;action.non_bail=true;
        for(int kind=0;kind<3;++kind) {
            action.mounting=kind==0;action.jumping=kind!=0;
            action.hippy.active=kind==2;
            const bool expected=kind==2 ? hippy_lock:jump_lock;
            check(head_position_attached(cfg,state,action)==expected,"off-board fallback overrode a jump switch");
            check(!effective_camera_config(cfg,true,state,action).spring.enabled || (!disable_foot && !expected),
                "attachment switch enabled Spring while foot suppression was active");
            check(kind==0 || camera_look_allowed(cfg,state,action),"attachment switch disrupted enabled parkour look");
            BailView view;HeadPose head;head.valid=true;head.root=head.head=pose();
            view.apply(pose(0,.1f),head,false,true,0);view.remember(pose(0,.1f),pose(0,.1f),head,true);
            head.root=head.head=pose(0,2);
            auto native=pose(30,3);auto output=view.apply(native,head,false,false,.016);
            if(head_position_attached(cfg,state,action)) output=view.attach(output,head);
            check(std::abs(output[12]-(expected ? 2.1f:3.f))<.00001f,"jump switch did not change final head position");
            check(angle(output,native)<.00001f,"head attachment changed camera orientation");
            // A live toggle off must release attachment on the next valid camera sample.
            cfg.stick.lock_mount_jump=cfg.stick.lock_hippy_jump=false;
            output=view.apply(native,head,false,false,.032);
            if(head_position_attached(cfg,state,action)) output=view.attach(output,head);
            check(output[12]==native[12],"live switch off retained head attachment");
            cfg.stick.lock_mount_jump=jump_lock;cfg.stick.lock_hippy_jump=hippy_lock;
        }
        action.mounting=action.jumping=true;action.hippy.active=true;
        check(head_position_attached(cfg,state,action)==hippy_lock,"mounting flag overrode the hippy switch");
        action.hippy.active=false;action.hippy.candidate=true;
        check(!head_position_attached(cfg,state,action),"unconfirmed hippy candidate used a fallback attachment");
        action={};action.mode=1;action.custom_mode=1;action.non_bail=true;
        check(head_position_attached(cfg,state,action)==disable_foot,"landing changed ground attachment policy");
        action.custom_mode=0;
        for(const auto child:{std::uintptr_t{0x65e8d40},std::uintptr_t{0x65e8ed0}}) {
            action.state={504,0,0,child};
            check(head_position_attached(cfg,state,action)==disable_foot,
                "special animation/ragdoll child forced attachment without an enabled option");
        }
        action.mode=action.custom_mode=2;
        check(!head_position_attached(cfg,state,action),"ordinary board airtime became head-attached");
        action.bail_recovery=true;
        check(head_position_attached(cfg,state,action),"jump switches disabled bail recovery attachment");
        cfg.look_only=true;
        check(!head_position_attached(cfg,state,action),"compatibility mode retained recovery attachment");
    }
    Config config;config.stick.enabled=config.stick.turn_character=true;
    ComfortSnapshot owner;owner.state_valid=owner.animation_valid=true;owner.normal_foot=true;
    CameraMovementSample movement;movement.mode=movement.custom_mode=1;movement.non_bail=true;
    check(camera_overrides_allowed(owner,movement) && character_facing_allowed(owner,movement),"normal-ground eligibility failed");
    movement.custom_mode=0;owner.normal_foot=false;owner.suspended=true;
    check(!camera_look_allowed(config,owner,movement),"default did not retain native climbing look");
    config.stick.look_during_parkour=true;
    check(camera_look_allowed(config,owner,movement) && !head_position_attached(config,owner,movement) &&
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
    config.look_only=true;config.disable_spring_on_foot=true;
    config.stick.lock_mount_jump=config.stick.lock_hippy_jump=true;
    movement.jumping=true;movement.hippy.active=true;
    check(!head_position_attached(config,owner,movement) && !action_camera_locked(config,movement),
        "compatibility mode retained head attachment");
    config.stick.look_during_parkour=true;
    check(camera_look_allowed(config,owner,movement),"compatibility mode disabled eligible look controls");
    owner.bailing=true;
    check(!head_position_attached(config,owner,movement) && !camera_look_allowed(config,owner,movement),
        "compatibility mode attached bail camera or captured bail input");
    owner.bailing=false;config.look_only=false;movement.jumping=false;movement.hippy.active=false;
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
