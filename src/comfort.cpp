#include "comfort_policy.h"
#include "game_camera_ownership.h"
#include "player_state_contract.h"
#include "native.h"
#include "settings.h"
#include "stock_bridge.h"
#include "owned_hooks.h"
#include "stick_runtime.h"
#include <mutex>
#include <atomic>
namespace jishsfirstpersonmod {
namespace {
std::mutex mutex;
ComfortPolicy policy;
BailLatch bail_latch;
RecoveryCameraLatch recovery_camera;
bool recovery_suppressed(const Config& config) { return config.third_person_recovery && !config.third_person_bailing && !config.look_only && recovery_camera.active; }
PlayerState player;
using ChooseState=std::uint32_t(*)(std::uintptr_t,std::uint32_t);
ChooseState original_choose{};
std::atomic<bool> animation_contract{},offboard_contract{},observer_installed{};
std::atomic<bool> suppression_owned{};
std::mutex selector_mutex;
std::uintptr_t watched_selector{};
ULONGLONG watched_until{},sample_time{};
std::uint32_t chosen_current{},chosen_next{};
std::uint64_t wipeout_events{},reported_wipeouts{};
std::uint32_t choose_hook(std::uintptr_t selector,std::uint32_t current) {
    const auto next=original_choose(selector,current);
    const auto after=GetLastError();
    {
        std::lock_guard lock(selector_mutex);
        const auto now=GetTickCount64();
        if(selector==watched_selector && now<watched_until) {
            chosen_current=current;chosen_next=next;sample_time=now;
            if(next==player_state_contract::wipeout) ++wipeout_events;
        }
    }
    SetLastError(after);
    return next;
}
PlayerState read_player(std::uintptr_t client,std::uintptr_t base) {
    using namespace dingosdk;
    using namespace dingosdk::client_source::detail;
    namespace pc=player_state_contract;
    SourceReader r;
    source_require(r.pointer(client)==base+addr::engine::client_vtable,"Client identity changed");
    const auto world=r.value<std::uint32_t>(client,0xc4);
    source_require(world==13 || world==21,"Outside gameplay");
    const auto context=r.pointer(client,8);
    const auto offset=r.value<std::uint32_t>(base,addr::engine::context_player_manager_offset);
    source_require(offset<=0x1000000,"Player manager offset invalid");
    const auto manager=r.pointer(context,offset);
    const auto begin=r.pointer(manager,0x4c8),end=r.pointer(manager,0x4d0);
    source_require(r.pointer(manager)==base+addr::engine::local_player_manager_vtable && begin && end==begin+8,"Local player manager invalid");
    const auto local=r.pointer(begin),entity=r.pointer(local,0xb8);
    source_require(r.pointer(local)==base+addr::engine::local_player_vtable &&
        r.pointer(local,0x78)==context && r.value<std::uint8_t>(local,0x45)==1 &&
        r.value<std::uint8_t>(local,0x44)==0 && r.pointer(entity)==base+addr::engine::skater_entity_vtable &&
        r.pointer(entity,0x20)==context && r.pointer(entity,0xf8)==local &&
        r.value<std::uint8_t>(entity,0x7e0)==0,"Local skater unavailable or teleporting");
    const auto handle=r.pointer(local,0xb0),collection=r.pointer(entity,0x70);
    const auto component=r.pointer(entity,pc::component_offset),core=r.pointer(component,pc::core_offset);
    const auto physics=r.pointer(core,pc::physics_context_offset),selector=r.pointer(core,pc::selector_offset);
    const auto causes=r.pointer(core,0x428),rig=r.pointer(core,0x438);
    source_require(r.pointer(handle)==entity+8 && r.pointer(collection)==entity &&
        r.pointer(component)==base+addr::engine::skater_component_vtable && r.pointer(component,0x18)==collection &&
        r.pointer(core)==base+pc::core_vtable && physics && selector &&
        r.pointer(selector,8)==physics && r.pointer(causes,0x20)==physics &&
        r.pointer(rig)==physics && r.pointer(rig,0x4630)==core,"Physics ownership changed");
    const auto state=r.value<std::uint32_t>(physics,pc::physics_state_offset); r.verify();
    const auto riding=pc::riding(state);
    PlayerState result{core,true,pc::board_verified && riding.has_value(),false,riding.value_or(false),false,state};
    result.client=client;result.entity=entity;result.context=physics;result.rig=rig;
    if(offboard_contract.load()) {
        SourceReader foot;
        try {
            const auto parent=foot.pointer(core,pc::movement_parent_offset),board=foot.pointer(core,0x430);
            if(foot.pointer(parent)==base+pc::offboard_vtable) {
                source_require(foot.pointer(parent,8)==physics && foot.pointer(parent,0x10)==board &&
                    foot.pointer(parent,0x18)==rig,"Offboard ownership changed");
                const auto active=foot.pointer(parent,pc::active_substate_offset);
                for(const auto& substate:pc::normal_foot_states) {
                    if(active==parent+substate.offset && foot.pointer(active)==base+substate.vtable) result.normal_foot=true;
                }
                foot.verify();
                // Native Offboard ownership is a stronger board signal than a
                // generic state family, including animation/ragdoll children.
                result.board_verified=true;result.riding=false;
            }
        } catch(...) { result.normal_foot=false; }
    }
    if(animation_contract.load()) {
        // Request flags trigger the latch; documented movement substates clear it.
        SourceReader animation;
        try {
            const auto output=animation.pointer(animation.pointer(core,pc::animation_owner),pc::animation_output);
            const auto request=animation.value<std::uint8_t>(output,pc::wipeout_request_offset);
            const auto bail=animation.value<std::uint8_t>(output,pc::animation_wipeout_offset);
            const auto recovery=animation.value<std::uint8_t>(output,pc::recovery_offset);
            source_require(request<=1 && bail<=1 && recovery<=1,"Animation flag invalid");animation.verify();
            result.animation_valid=true;result.wipeout_request=request!=0;
            result.animation_wipeout=bail!=0;result.recovery=recovery!=0;
        } catch(...) {}
    }
    {
        std::lock_guard lock(selector_mutex);
        const auto now=GetTickCount64();
        if(watched_selector!=selector || now>=watched_until) { sample_time=0;reported_wipeouts=wipeout_events; }
        watched_selector=selector;watched_until=now+500;
        result.selector_valid=observer_installed.load() && sample_time && now-sample_time<500;
        result.selector_current=chosen_current;result.selector_next=chosen_next;
        if(wipeout_events!=reported_wipeouts) {
            result.wipeout_event=true;
            reported_wipeouts=wipeout_events;
        }
    }
    return result;
}
}
ComfortSnapshot comfort_snapshot() {
    std::lock_guard lock(mutex);
    const auto config=menu_snapshot().config;
    const auto desired=policy.desired(player,config.third_person_bailing,(config.on_board_only || config.native.board_only),recovery_suppressed(config));
    return {policy.master,policy.initialized,player_state_contract::board_verified,animation_contract.load() && offboard_contract.load(),
        player.valid,policy.master && desired && !*desired,player.physics,
        player.animation_valid,player.wipeout_request,player.animation_wipeout,player.recovery,
        player.selector_valid,player.selector_current,player.selector_next,
        player.bailing,player.riding,policy.master && (config.third_person_camera_switching() || (config.on_board_only || config.native.board_only)) && !desired,
        player.normal_foot,player.identity,player.client,player.entity,player.context,player.rig};
}
void comfort_user_request(bool enabled) {
    const auto config=menu_snapshot().config;
    std::lock_guard lock(mutex);policy.request(enabled);
    const auto desired=policy.desired(player,config.third_person_bailing,(config.on_board_only || config.native.board_only),recovery_suppressed(config));
    suppression_owned=enabled && desired && !*desired;
}
bool comfort_requested_view(bool master) {
    const auto config=menu_snapshot().config;
    std::lock_guard lock(mutex);
    auto requested=policy;requested.request(master);
    return !game_camera_suspended() && requested.desired(player,config.third_person_bailing,(config.on_board_only || config.native.board_only),recovery_suppressed(config)).value_or(stock_first_person_requested());
}
void comfort_observe_native(bool active) { std::lock_guard lock(mutex); policy.observe_native(active); }
void clear_comfort_state() {
    { std::lock_guard lock(mutex); player={};bail_latch={};recovery_camera={}; }
    { std::lock_guard lock(selector_mutex); watched_selector=0;watched_until=sample_time=0; }
}
void update_comfort(std::uintptr_t client,std::uintptr_t base) {
    PlayerState next;
    try { next=read_player(client,base); } catch(...) {}
    if(!next.valid) {
        std::lock_guard lock(selector_mutex);watched_selector=0;watched_until=sample_time=0;
    }
    std::optional<bool> desired;
    bool master{},sample_recovery{};
    const auto config=menu_snapshot().config;
    {
        std::lock_guard lock(mutex);
        const bool positive=next.wipeout_event || next.physics==player_state_contract::wipeout ||
            (next.selector_valid && next.selector_next==player_state_contract::wipeout) ||
            (next.animation_valid && (next.wipeout_request || next.animation_wipeout));
        const auto bail=bail_latch.update({next.identity,next.valid,next.board_verified && next.riding,
            next.normal_foot,next.animation_valid,positive,next.recovery});
        next.bail_verified=animation_contract.load() && offboard_contract.load() && bail.has_value();
        next.bailing=bail.value_or(false);
        if(player.bailing!=next.bailing || player.valid!=next.valid) {
            log(!next.valid ? "Comfort switching suspended: local player unavailable.":
                next.bailing ? (config.third_person_bailing ? "Comfort: wipeout latched; third person until native movement resumes.":
                    "Comfort: wipeout latched; first-person camera follows head; look resumes on verified native control."):
                "Comfort: native movement restored; bail restriction cleared.");
        }
        player=next;
        desired=policy.desired(player,config.third_person_bailing,(config.on_board_only || config.native.board_only));
        master=policy.master;
        sample_recovery=config.third_person_recovery || recovery_camera.active;
    }
    // Publish this tick's player before the existing movement reader takes its
    // snapshot. Never call it while holding the comfort mutex.
    if(sample_recovery) {
        const auto movement=live_camera_movement();
        std::lock_guard lock(mutex);
        const bool previous=recovery_camera.active;
        recovery_camera.update(config.third_person_recovery && !config.third_person_bailing && !config.look_only,
            next.identity,next.valid && movement.mode && movement.recovery_verified,next.bailing,
            movement.getting_up,movement.recovery_controllable,
            !next.bailing && (next.riding || next.normal_foot),next.wipeout_event);
        if(previous!=recovery_camera.active) log(recovery_camera.active ?
            "Comfort: getting up; third person until normal control returns.":"Comfort: getting-up camera released.");
        desired=policy.desired(player,config.third_person_bailing,(config.on_board_only || config.native.board_only),recovery_suppressed(config));
    }
    const bool ownership=game_camera_suspended();
    const bool ownership_managed=game_camera_managing();
    if(ownership) desired=false;
    acknowledge_game_camera(master,stock_first_person_active());
    // Disabled options leave the existing native activation path untouched.
    static ULONGLONG next_retry{};
    static std::optional<bool> retry_target;
    if(!master) suppression_owned=false;
    const auto now=GetTickCount64();
    if((config.third_person_camera_switching() || (config.on_board_only || config.native.board_only) || suppression_owned || ownership_managed) && (next.valid || ownership) && desired && (ownership ? stock_bridge_ready():stock_camera_available()) &&
       *desired!=stock_first_person_requested() && (retry_target!=desired || now>=next_retry)) {
        retry_target=desired;
        next_retry=now+1000;
        if(request_stock_first_person(*desired)) {
            suppression_owned=master && !*desired;
            log(*desired ? "Comfort restored First Person.":"Comfort temporarily selected third person.");
        }
    }
}
void install_comfort_observer(std::uintptr_t base) {
    using namespace dingosdk;
    namespace pc=player_state_contract;
    std::array<unsigned char,32> bytes{};
    std::array<unsigned char,18> recovery{};
    bool foot_ok=memory::read(base+pc::offboard_owner_site,bytes) && bytes==pc::offboard_owner_prefix &&
        memory::read(base+pc::offboard_constructor_site,bytes) && bytes==pc::offboard_constructor_prefix;
    for(const auto& substate:pc::normal_foot_states) {
        std::uintptr_t update{};
        foot_ok=foot_ok && memory::read(base+substate.vtable+0x18,update) && update==base+substate.update;
    }
    offboard_contract=foot_ok;
    animation_contract=memory::read(base+pc::animation_request_site,bytes) && bytes==pc::animation_request_prefix &&
        memory::read(base+pc::animation_bail_site,bytes) && bytes==pc::animation_bail_prefix &&
        memory::read(base+pc::recovery_site,recovery) && recovery==pc::recovery_prefix;
    log(animation_contract && foot_ok ? "Bail comfort contracts verified: wipeout requests and native movement substates.":
        "ERROR: bail comfort unavailable: native animation/movement contract differs.");
    if(!memory::read(base+pc::caller_rva,bytes) || bytes!=pc::caller_prefix) {
        log("Pre-remap selector observer unavailable: caller contract differs.");return;
    }
    const auto handler=resolve_stock_handler(base+pc::choose_state_rva);
    if(!handler || !stock_bridge_ready()) {
        log("Pre-remap selector observer unavailable: no verified stock selector hook to chain.");return;
    }
    void* forwarding{};
    auto result=owned_hook_prepare(reinterpret_cast<void*>(handler),reinterpret_cast<void*>(&choose_hook),&forwarding);
    if(result==HookOk) {
        original_choose=reinterpret_cast<ChooseState>(forwarding);
        result=hook_queue_enable(reinterpret_cast<void*>(handler));
        if(result==HookOk) result=apply_owned_hooks();
    }
    observer_installed=result==HookOk;
    log(observer_installed ? "Pre-remap selector observer chained; original state and return value preserved.":
        "Pre-remap selector observer attach failed: "+std::to_string(result));
}
}
