#include "settings.h"
#include "comfort_policy.h"
#include "player_state_contract.h"
#include "look_smoothing.h"
#include <iostream>
#include <stdexcept>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path module_directory() { return std::filesystem::current_path()/"comfort-test-data"; }
std::filesystem::path data_directory() { return module_directory(); }
}
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
int main() {
    using namespace jishsfirstpersonmod;
    Config initial; initial.spring.enabled=true; initial.spring.up=76; initial.spring.offset={.1f,.2f,.3f};
    auto path=data_directory()/"settings.ini";
    SettingsStore store(path,initial);
    Command command; command.kind=CommandKind::link_smoothing;command.enabled=false;
    check(!initial.third_person_recovery,"Recovery option unexpectedly enabled by default");
    RecoveryCameraLatch recovery;
    check(!recovery.update(true,1,true,true,false,false,false,false),"Third person started during the fall");
    check(recovery.update(true,1,true,true,true,false,false,false),"Getting up did not select third person");
    check(recovery.update(true,1,true,true,false,false,false,false),"Cleared getting-up flag released camera early");
    check(!recovery.update(true,1,true,true,false,true,false,false),"Normal control did not restore first person");
    recovery.update(true,1,true,true,true,false,false,false);
    check(!recovery.update(true,1,true,true,true,false,false,true),"Repeated fall retained third-person recovery");
    recovery.update(true,1,true,true,true,false,false,false);
    check(!recovery.update(true,2,true,true,false,false,false,false),"New character inherited recovery suppression");
    recovery.update(true,2,true,true,true,false,false,false);
    check(!recovery.update(false,2,true,true,true,false,false,false),"Disabled option retained suppression");
    recovery.update(true,2,true,true,true,false,false,false);
    check(!recovery.update(true,2,false,true,true,false,false,false),"Invalid state retained suppression");
    command.kind=CommandKind::third_person_recovery;command.enabled=true;
    check(store.submit(command,0),"Recovery setting command failed");store.flush_due(0);
    check(read_config(path).third_person_recovery,"Recovery setting did not persist");
    command.kind=CommandKind::link_smoothing;command.enabled=false;
    check(store.submit(command,0),"unlink failed");
    auto config=store.snapshot().config;
    check(!config.link_smoothing && config.look_smoothing_initialized && config.look_spring.up==76 &&
        config.look_spring.offset==dingosdk::first_person::Vec3{},"first unlink did not copy smoothing only");
    command.kind=CommandKind::look_settings;command.spring=config.look_spring;command.spring.up=23;
    check(store.submit(command,1),"look settings failed");
    config=store.snapshot().config;
    check(config.spring.up==76 && glance_settings(config).up==23,"independent values leaked to stock settings");
    command.kind=CommandKind::link_smoothing;command.enabled=true;store.submit(command,2);
    check(glance_settings(store.snapshot().config).up==76,"linked settings ignored native spring");
    command.enabled=false;store.submit(command,3);
    check(store.snapshot().config.look_spring.up==23,"relink discarded independent values");
    command.kind=CommandKind::comfort_options;command.third_person_bailing=true;command.on_board_only=true;
    check(store.submit(command,4),"comfort persistence request failed");store.flush_due(4);
    config=read_config(path);
    check(config.third_person_bailing && config.on_board_only && !config.link_smoothing && config.look_spring.up==23,
        "settings did not survive reload");
    command.kind=CommandKind::set_enabled;command.enabled=true;store.submit(command,5);
    store.mark_enabled_queued(true);store.publish_runtime(false,true,"Temporarily third person");
    store.acknowledge_master(true);
    check(!store.snapshot().requested_enabled && !store.snapshot().active,"suppressed master acknowledgement changed native camera state");
    command.kind=CommandKind::set_enabled;command.enabled=true;
    check(store.submit(command,6),"pending activation setup failed");
    command.kind=CommandKind::look_only;command.enabled=true;
    check(store.submit(command,6),"compatibility switch rejected");store.flush_due(6);
    check(!store.take_enabled() && !store.snapshot().requested_enabled,"compatibility retained pending activation");
    config=read_config(path);
    check(config.look_only && config.third_person_bailing && config.spring.offset==initial.spring.offset,
        "compatibility persistence discarded saved camera settings");
    command.kind=CommandKind::comfort_options;command.third_person_bailing=false;command.on_board_only=false;
    check(store.submit(command,7) && store.snapshot().config.look_only && !store.snapshot().config.third_person_bailing && !store.snapshot().config.on_board_only,
        "compatibility mode blocked comfort edits");
    command.third_person_bailing=true;command.on_board_only=true;
    check(store.submit(command,7),"compatibility comfort restore failed");store.flush_due(7);
    config=read_config(path);
    check(config.look_only && config.third_person_bailing && config.on_board_only,"compatibility comfort options did not persist");
    command.kind=CommandKind::set_enabled;
    check(store.submit(command,7) && store.take_enabled()==true,"compatibility mode blocked manual activation");
    command.enabled=false;
    check(store.submit(command,7) && store.take_enabled()==false,"compatibility mode blocked manual deactivation");
    command.kind=CommandKind::look_only;command.enabled=false;
    check(store.submit(command,8) && !store.snapshot().config.look_only && store.snapshot().config.third_person_bailing,
        "leaving compatibility lost saved settings");
    // Preserve the native preference when ReSkate has saved the effective off
    // value, including a game restart and a recommended preset change.
    command.kind=CommandKind::cycle_restore;command.action=1;command.enabled=true;
    check(store.submit(command,9),"native camera-cycle preference capture failed");
    store.flush_due(9);
    auto retained=read_config(path);
    check(retained.cycle_restore_pending && retained.cycle_restore_enabled,"camera-cycle preference lost on restart");
    SettingsStore restarted(path,retained);
    command.kind=CommandKind::recommended_preset;command.preset=Config{};
    check(restarted.submit(command,10),"preset failed during camera-cycle override");
    retained=restarted.snapshot().config;
    check(retained.cycle_restore_pending && retained.cycle_restore_enabled,"preset erased camera-cycle restoration");
    command.kind=CommandKind::cycle_restore;command.action=1;command.enabled=false;
    check(restarted.submit(command,11),"native preference edit rejected during override");
    restarted.flush_due(11);retained=read_config(path);
    check(retained.cycle_restore_pending && !retained.cycle_restore_enabled,"latest native preference not retained");
    command.action=0;
    check(restarted.submit(command,12),"camera-cycle restoration acknowledgement failed");
    restarted.flush_due(12);
    check(!read_config(path).cycle_restore_pending,"completed restoration survived as pending");
    ComfortPolicy policy;PlayerState riding{1,true,true,true,true,false,100};
    check(!policy.desired(riding,true,true),"uninitialized policy guessed master mode");
    policy.observe_native(true);
    check(policy.desired(riding,true,true)==true,"riding not first person");
    auto bail=riding;bail.bailing=true;bail.physics=300;
    check(policy.desired(bail,true,false)==false && policy.desired(bail,false,true)==true &&
        policy.desired(bail,true,true)==false,"comfort options not independent");
    auto foot=riding;foot.riding=false;foot.physics=504;
    check(policy.desired(foot,false,true)==false && policy.desired(foot,true,false)==true,"walking policy incorrect");
    policy.observe_native(false);
    check(policy.desired(riding,false,false,true)==false && policy.master,"Recovery suppression erased first-person preference");
    check(policy.master && policy.desired(riding,true,true)==true,"temporary native suppression erased master intent");
    check(!policy.desired({},true,true),"invalid lifecycle state guessed a view");
    auto uncertain=foot;uncertain.bail_verified=false;
    check(!policy.desired(uncertain,true,false),"ambiguous offboard state inferred bail recovery");
    policy.request(false);check(policy.desired(riding,true,true)==false,"manual off restored first person");
    check(policy.desired(riding,false,false,true)==false && !policy.master,"Recovery overrode manual first-person off");
    policy.request(true);check(policy.desired(riding,true,true)==true,"re-arming failed");
    check(player_state_contract::board_verified && player_state_contract::bail_verified,"documented comfort contracts disabled");
    for(auto state:{100u,101u,103u,200u,201u,400u,499u}) check(player_state_contract::riding(state)==true,"known board family rejected");
    check(player_state_contract::riding(504)==false && player_state_contract::riding(300)==false,"offboard or bail classified riding");
    check(!player_state_contract::riding(0) && !player_state_contract::riding(503) && !player_state_contract::riding(999),"unknown board state guessed");
    check(std::string(player_state_contract::state_name(504))=="Off-board / remapped wipeout","504 lost its documented ambiguity");
    check(std::string(player_state_contract::state_name(103))=="Crouch" &&
        std::string(player_state_contract::state_name(200))=="Pop","documented trick state names incorrect");
    BailLatch latch;
    BailEvidence walking{1,true,false,true,true,false,false};
    check(latch.update(walking)==false,"ordinary walking became a bail");
    auto stumble=walking;stumble.recovery=true;
    check(latch.update(stumble)==false,"runout alone started a wipeout");
    auto falling=walking;falling.wipeout=true;
    check(latch.update(falling)==true,"positive wipeout failed to latch");
    auto ragdoll=walking;ragdoll.normal_foot=false;
    check(latch.update(ragdoll)==true,"transient wipeout flag clearing ended ragdoll too early");
    check(latch.update(stumble)==true,"recovery/runout ended active bail too early");
    check(latch.update(walking)==true,"single transitional normal sample cleared a bail");
    check(latch.update(walking)==false,"normal on-foot movement did not restore first person");
    latch.update(falling);
    auto mounted=walking;mounted.riding=true;mounted.normal_foot=false;
    check(latch.update(mounted)==true && latch.update(mounted)==false,"riding after recovery retained bail");
    latch.update(falling);
    check(!latch.update({}),"invalid player state retained a usable bail decision");
    check(latch.update(walking)==false,"respawn retained old bail latch");
    latch.update(falling);walking.identity=2;
    check(latch.update(walking)==false,"different physics owner inherited a bail");
    auto unmapped=walking;unmapped.normal_foot=false;
    check(!latch.update(unmapped),"unknown animation state guessed recovery");
    using namespace dingosdk::first_person;
    Matrix pose{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};Settings smooth;smooth.enabled=true;
    LookSmoothing look;look.apply(pose,smooth,{},0,true);
    auto before=look.apply(pose,smooth,{0,90},.016,true);
    Settings immediate;
    auto after=look.apply(pose,immediate,{0,90},.016,false);
    check(before==after,"linkage switch snapped held look");
    for(int i=2;i<500;++i) after=look.apply(pose,immediate,{},i*.016,false);
    auto snap=look.apply(pose,immediate,{0,90},8,false);
    check(std::abs(snap[0])<.00001f,"unlinked disabled spring not applied after neutral");
    std::cout<<"Independent smoothing, persistence, linkage continuity and comfort policy checks passed\n";
}
