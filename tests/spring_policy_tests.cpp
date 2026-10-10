#include "spring_policy.h"
#include "camera_policy.h"
#include "settings.h"
#include "stock_contract.h"
#include <iostream>
#include <stdexcept>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path module_directory() { return std::filesystem::current_path()/"spring-policy-test-data"; }
std::filesystem::path data_directory() { return module_directory(); }
}
void check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
int main() {
    using namespace jishsfirstpersonmod;
    Config saved;saved.spring.enabled=true;saved.spring.up=76;saved.spring.offset={.1f,.2f,.3f};
    check(!saved.disable_spring_on_foot,"old profiles opt in unexpectedly");
    check(camera_config_for_state(saved,true,1).spring.enabled,"disabled option changed spring");
    saved.disable_spring_on_foot=true;
    const auto foot=camera_config_for_state(saved,true,1);
    check(!foot.spring.enabled && foot.spring.up==76 && foot.spring.offset==saved.spring.offset,"foot override changed arm settings");
    check(saved.spring.enabled && camera_config_for_state(saved,true,2).spring.enabled,"board did not restore preference");
    check(camera_config_for_state(saved,false,1).spring.enabled,"inactive first person retained override");
    check(camera_config_for_state(saved,true,0).spring.enabled,"unverified movement forced spring off");
    check(!glance_settings(foot).enabled,"linked smoothing ignored override");
    saved.link_smoothing=false;saved.look_spring.enabled=true;
    check(glance_settings(camera_config_for_state(saved,true,1)).enabled,"independent look smoothing changed");
    saved.spring.enabled=false;
    check(!camera_config_for_state(saved,true,2).spring.enabled,"board enabled manually disabled spring");
    saved.spring.enabled=true;
    ComfortSnapshot owner;owner.state_valid=owner.animation_valid=true;owner.physics=100;
    CameraMovementSample movement;movement.mode=2;movement.custom_mode=2;movement.non_bail=true;
    check(camera_overrides_allowed(owner,movement),"normal movement lost overrides");
    auto bail_config=saved;bail_config.disable_spring_on_foot=false;bail_config.stick.enabled=true;
    owner.bailing=true;
    check(!effective_camera_config(bail_config,true,owner,movement).spring.enabled &&
        !camera_overrides_allowed(owner,movement),"latched bail retained spring or overrides");
    owner.bailing=false;owner.recovery=true;
    check(!effective_camera_config(bail_config,true,owner,movement).spring.enabled &&
        !camera_overrides_allowed(owner,movement),"recovery retained spring or overrides");
    owner.recovery=false;movement.bail_recovery=true;
    check(!effective_camera_config(bail_config,true,owner,movement).spring.enabled &&
        !camera_overrides_allowed(owner,movement),"live bail flags waited for tick latch");
    movement.bail_recovery=false;movement.mounting=true;
    check(action_camera_locked(bail_config,movement) && !effective_camera_config(bail_config,true,owner,movement).spring.enabled,
        "mount lock did not disable spring");
    movement.mounting=false;movement.mode=1;movement.jumping=true;
    check(action_camera_locked(bail_config,movement),"on-foot jump lock missing");
    bail_config.stick.lock_mount_jump=false;
    check(!action_camera_locked(bail_config,movement) && effective_camera_config(bail_config,true,owner,movement).spring.enabled,
        "mount/jump toggle could not restore normal behavior");
    bail_config.stick.lock_mount_jump=true;movement.jumping=false;movement.mode=2;movement.state[0]=200;
    check(!action_camera_locked(bail_config,movement),"board jump was locked");
    movement.jumping=true;bail_config.stick.enabled=false;
    check(!action_camera_locked(bail_config,movement),"mount/jump lock required no stick feature");
    owner.bailing=true;
    check(!effective_camera_config(bail_config,true,owner,movement).spring.enabled,"bail lock depended on stick feature");
    check(effective_camera_config(bail_config,false,owner,movement).spring.enabled,"inactive bail changed saved spring");
    StickLookState stick;StickSettings control;control.enabled=true;
    dingosdk::first_person::Settings immediate;
    stick.update(control,{},true,1,0,0,0,immediate);
    check(stick.update(control,{1,0},true,1,.016,0,0,immediate).active,"stick setup did not activate");
    check(!stick.update(control,{1,0},false,1,.032,0,0,immediate).active,"camera lock retained stick aim");
    check(!stick.update(control,{1,0},true,1,.048,90,0,immediate).active,"held stick resumed before neutral");
    stick.update(control,{},true,1,.064,90,0,immediate);
    check(stick.update(control,{1,0},true,1,.080,90,0,immediate).active,"neutral did not release camera lock");
    movement={};movement.mode=1;movement.jumping=true;movement.hippy.active=true;
    bail_config.stick.enabled=true;bail_config.stick.lock_mount_jump=true;owner.bailing=false;
    check(!action_camera_locked(bail_config,movement),"on-foot toggle captured a confirmed hippy jump");
    bail_config.stick.lock_hippy_jump=true;
    check(action_camera_locked(bail_config,movement),"hippy toggle did not lock confirmed jump");
    bail_config.stick.enabled=false;
    check(!action_camera_locked(bail_config,movement),"hippy lock ignored right-stick requirement");
    movement.view_hold=true;
    check(camera_overrides_allowed(owner,movement) || !movement.non_bail,"reconciliation blocked eligible input");
    RuntimeSpringSwitch sync;
    check(!sync.update(false,true,false,0),"disabled policy issued native write");
    check(sync.update(true,false,true,1)==false,"offboard did not queue spring off");sync.queued(false,true);
    check(!sync.update(true,false,true,50),"pending queue flooded");
    check(sync.update(true,true,true,60)==true,"rapid mount did not replace pending disable");sync.queued(true,true);
    check(!sync.update(true,true,true,70),"acknowledged native write repeated");
    check(sync.update(true,false,true,80)==false,"second dismount failed");sync.queued(false,true);
    check(!sync.update(true,false,false,90),"offboard acknowledgement failed");
    check(sync.protects_preference(false),"runtime override lost preference protection");
    check(sync.update(false,true,false,100)==true,"clearing option did not restore spring");sync.queued(true,true);
    check(sync.protects_preference(false),"pending restoration exposed native bit");
    check(!sync.update(false,true,true,110) && !sync.protects_preference(false),"restoration did not release ownership");
    RuntimeSpringSwitch failed;
    check(failed.update(true,false,true,0)==false,"initial native request missing");failed.queued(false,false);
    check(!failed.update(true,false,true,500),"rejected native request retried too soon");
    check(failed.update(true,false,true,1000)==false,"rejected request was not retried");failed.queued(false,true);
    check(!failed.update(true,false,true,1500),"unacknowledged request flooded");
    check(failed.update(true,false,true,2000)==false,"unacknowledged request was not retried");
    check(failed.update(true,true,false,2001)==true,"opposite target was rate limited");
    // Use the same mirror normalization as the bridge, beyond its pending window.
    stock_contract::CameraMirror mirror;auto native=saved;
    mirror.reconcile(native,saved,0);
    native.spring.enabled=false;native.spring.offset[1]=.7f;
    if(sync.protects_preference(true)) native.spring.enabled=saved.spring.enabled;
    auto merged=mirror.reconcile(native,saved,3000);
    check(merged.spring.enabled && merged.spring.offset[1]==.7f,"automatic bit overwrote preference or masked unrelated stock edit");
    const auto path=data_directory()/"settings.ini";
    SettingsStore store(path,saved);Command command;command.kind=CommandKind::spring_on_foot;command.enabled=true;
    check(store.submit(command,0),"option command rejected");store.flush_due(0);
    const auto loaded=read_config(path);
    check(loaded.stick.lock_mount_jump && loaded.disable_spring_on_foot && loaded.spring.enabled && loaded.spring.up==76,"option/preference did not persist");
    command.enabled=false;check(store.submit(command,1),"clearing option rejected");store.flush_due(1);
    check(!read_config(path).disable_spring_on_foot && read_config(path).spring.enabled,"clearing option changed saved spring");
    Command stick_command;stick_command.kind=CommandKind::stick_settings;stick_command.stick=saved.stick;
    check(!loaded.stick.look_during_parkour,"parkour look default changed");
    stick_command.stick.look_during_parkour=true;stick_command.stick.lock_mount_jump=false;stick_command.stick.lock_hippy_jump=true;
    check(store.submit(stick_command,2),"mount/jump toggle rejected");store.flush_due(752);
    check(!read_config(path).stick.lock_mount_jump && read_config(path).stick.lock_hippy_jump && read_config(path).stick.look_during_parkour,"mount/jump toggle did not persist");
    std::cout<<"State-based Spring policy, native queue restoration and preference persistence passed\n";
}
