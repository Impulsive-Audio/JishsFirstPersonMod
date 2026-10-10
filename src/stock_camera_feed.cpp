#include "stock_bridge.h"
#include "game_camera_ownership.h"
#include "stock_contract.h"
#include "bridge_resolver.h"
#include "settings.h"
#include "look_smoothing.h"
#include "stick_runtime.h"
#include "camera_transition.h"
#include "camera_policy.h"
#include "bail_camera_trace.h"
#include "bail_view.h"
#include "owned_hooks.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Game/Build/20260929/client_source_spawn.h"
#include "Extension/Skater/free_flight.h"
#include <atomic>
#include <intrin.h>
#include <chrono>
#include <mutex>
namespace jishsfirstpersonmod {
namespace {
using Matrix=dingosdk::first_person::Matrix;
using Transform=void(*)(std::uintptr_t,const Matrix*);
using WriteFov=bool(*)(std::uintptr_t,float);
Transform original_transform{};WriteFov original_fov{};HMODULE stock_module{};
std::uintptr_t game_base{};
std::atomic<bool> feed_prepared{},transform_installed{};
std::atomic<std::uintptr_t> watched_camera{};
std::atomic<float> look_pitch{},look_yaw{},game_fov{90};
std::atomic<double> look_frame{};
std::mutex smoothing_mutex;
LookSmoothing smoothing;
CameraTransition transition;
BailView bail_view;
std::uintptr_t smoothing_camera{};
std::array<std::uintptr_t,5> transition_owner{};
bool from_stock(void* caller) {
    HMODULE owner{};return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(caller),&owner) && owner==stock_module;
}
void reset_camera_smoothing() {
    smoothing.reset();transition.reset();bail_view.reset();set_bail_view_suspended(false);smoothing_camera=0;
}
void transform_hook(std::uintptr_t camera,const Matrix* input) {
    if(game_camera_suspended()) {
        { std::lock_guard lock(smoothing_mutex);reset_camera_smoothing(); }
        original_transform(camera,input);return;
    }
    const auto watched=watched_camera.load();
    if(compatibility::enabled() && input && (!watched || camera==watched) && stock_first_person_requested() && from_stock(_ReturnAddress())) {
        watched_camera=camera;Matrix native{};
        if(dingosdk::memory::read(reinterpret_cast<std::uintptr_t>(input),native) && dingosdk::valid_flight_transform(native)) {
            auto matrix=native;auto movement=live_camera_movement();
            const auto state=comfort_snapshot();const auto config=menu_snapshot().config;
            if(config.look_only) {
                dingosdk::first_person::Quat glance{0,0,0,1};
                {
                    std::lock_guard lock(smoothing_mutex);
                    const std::array<std::uintptr_t,5> identity{state.core,state.client,state.entity,state.context,state.rig};
                    if(smoothing_camera!=camera || identity!=transition_owner) { reset_camera_smoothing();smoothing_camera=camera;transition_owner=identity; }
                    if(camera_look_allowed(config,state,movement)) {
                        const Matrix identity_pose{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
                        glance=dingosdk::first_person::orientation(smoothing.apply(identity_pose,glance_settings(config),{look_pitch.load(),look_yaw.load()},look_frame.load(),config.link_smoothing));
                        matrix=aim_camera(native,movement.mode==1 ? stick_camera_aim():StickAim{});
                        dingosdk::first_person::write(matrix,dingosdk::first_person::normalized(dingosdk::first_person::multiply(dingosdk::first_person::orientation(matrix),glance)),{native[12],native[13],native[14]});
                    } else { smoothing.reset();stop_stick_look(); }
                }
                note_camera_view(matrix,glance);
                original_transform(camera,&matrix);return;
            }
            const auto head=read_animation_head(state);const auto pitch=look_pitch.load(),yaw=look_yaw.load();
            bool suspended{},changed{};StickAim aim{};BailViewInfo view;
            dingosdk::first_person::Quat glance{0,0,0,1};
            const double frame=look_frame.load();
            {
                std::lock_guard lock(smoothing_mutex);
                const std::array<std::uintptr_t,5> identity{state.core,state.client,state.entity,state.context,state.rig};
                if(smoothing_camera!=camera || identity!=transition_owner) { reset_camera_smoothing();smoothing_camera=camera;transition_owner=identity; }
                if(!movement.mode || !state.state_valid) { reset_camera_smoothing();suspended=true; }
                else {
                    const bool bail=bail_camera_locked(state,movement);
                    const bool attached=head_position_attached(config,state,movement);
                    const bool normal=character_facing_allowed(state,movement) ||
                        (camera_overrides_allowed(state,movement) && movement.mode==2 && movement.custom_mode==2);
                    matrix=bail_view.apply(native,head,bail,normal,frame);view=bail_view.info();
                    movement.view_hold=view.blocked;set_bail_view_suspended(view.blocked);
                    const bool valid=camera_overrides_allowed(state,movement) && head.valid;
                    const bool eligible=valid && camera_look_allowed(config,state,movement);
                    if(bail || !valid) {
                        smoothing.reset();transition.reset();suspended=true;changed=view.blocked;
                        if(!bail) { bail_view.reset();view=bail_view.info(); }
                    } else {
                        if(!eligible) { smoothing.reset();suspended=true; }
                        else {
                            const auto effective=effective_camera_config(config,stock_first_person_requested(),state,movement);
                            const Matrix identity_pose{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
                            glance=dingosdk::first_person::orientation(smoothing.apply(identity_pose,glance_settings(effective),{pitch,yaw},frame,config.link_smoothing));
                            aim=movement.mode==1 ? stick_camera_aim():StickAim{};
                            matrix=aim_camera(matrix,aim);
                            dingosdk::first_person::write(matrix,dingosdk::first_person::normalized(dingosdk::first_person::multiply(dingosdk::first_person::orientation(matrix),glance)),{matrix[12],matrix[13],matrix[14]});
                        }
                        if(attached) matrix=bail_view.attach(matrix,head);
                        dingosdk::first_person::Settings handoff;handoff.enabled=config.stick.smooth_transitions;
                        const bool mount_handoff=config.stick.enabled && movement.mounting;
                        handoff.up=handoff.down=handoff.left=handoff.right=(mount_handoff || movement.mode==2) ? config.stick.mount_smoothing:config.stick.dismount_smoothing;
                        // Native attachment disables positional Spring, not saved handoff strengths.
                        const auto look_settings=glance_settings(config);
                        matrix=transition.apply(matrix,handoff,state.core,movement.mode,aim.active,frame,movement.state,
                            &config.spring,&look_settings,attached,eligible,!eligible,mount_handoff);
                        bail_view.remember(matrix,native,head,normal);view=bail_view.info();changed=true;
                    }
                }
            }
            if(suspended) { suspend_custom_camera();look_pitch=0;look_yaw=0; }
            note_camera_view(matrix,glance);
            record_bail_camera(native,matrix,state,movement,config,aim,pitch,yaw,view,head);
            original_transform(camera,changed ? &matrix:input);return;
        }
    }
    if(!stock_first_person_requested() || (camera==watched && from_stock(_ReturnAddress()))) { std::lock_guard lock(smoothing_mutex);reset_camera_smoothing(); }
    original_transform(camera,input);
}
bool fov_hook(std::uintptr_t camera,float value) {
    if(!menu_snapshot().config.look_only && !game_camera_suspended() && camera==watched_camera.load() && stock_first_person_requested() && menu_snapshot().config.fov==0 && from_stock(_ReturnAddress()))
        value=game_fov.load();
    return original_fov(camera,value);
}
bool activate_transform_feed() {
    if(!feed_prepared.load(std::memory_order_acquire)) return false;
    if(transform_installed) return true;
    if(!stock_module || !game_base || !original_transform) return false;
    using namespace dingosdk;
    const auto stock=reinterpret_cast<std::uintptr_t>(stock_module);
    std::uint8_t initialized{};std::uintptr_t base{},callback{};
    const auto& profile=stock_contract::contract();
    if(!memory::read(stock+profile.source_initialized_rva,initialized) || !initialized ||
       !memory::read(stock+profile.source_base_rva,base) || base!=game_base ||
       !memory::read(stock+profile.source_transform_rva,callback) || callback!=reinterpret_cast<std::uintptr_t>(original_transform)) return false;
    auto* slot=reinterpret_cast<void* volatile*>(stock+profile.source_transform_rva);
    MEMORY_BASIC_INFORMATION page{};
    if(!VirtualQuery(const_cast<void*>(reinterpret_cast<const volatile void*>(slot)),&page,sizeof(page)) || page.State!=MEM_COMMIT ||
        (page.Protect&0xff)!=PAGE_READWRITE) return false;
    if(InterlockedCompareExchangePointer(slot,reinterpret_cast<void*>(&transform_hook),reinterpret_cast<void*>(original_transform))!=reinterpret_cast<void*>(original_transform)) return false;
    transform_installed=true;
    log("Stock SourceTrial transform callback chained: look feed active; game code and camera ownership unchanged.");return true;
}
}
void set_stock_glance(std::uintptr_t camera,float fov,float pitch,float yaw) {
    if(compatibility::enabled()) activate_transform_feed();
    if(!stock_first_person_requested()) {
        std::lock_guard lock(smoothing_mutex);reset_camera_smoothing();
    }
    watched_camera=camera;look_pitch=pitch;look_yaw=yaw;
    look_frame=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if(std::isfinite(fov) && fov>1 && fov<175) game_fov=fov;
}
bool install_stock_camera_feed(std::uintptr_t base,HMODULE stock) {
    using namespace dingosdk;
    namespace addresses=game::build::v20260929::client_source_spawn;
    std::array<unsigned char,24> prefix{};
    if(!memory::read(base+addresses::camera_transform_setter,prefix) || prefix!=addresses::camera_transform_prefix) {
        log("ERROR: native camera transform prefix differs; look input feed not installed.");return false;
    }
    if(!stock_bridge_ready()) return false;
    stock_module=stock;game_base=base;
    original_transform=reinterpret_cast<Transform>(base+addresses::camera_transform_setter);
    feed_prepared.store(true,std::memory_order_release);
    set_background_flush(flush_bail_camera_trace);
    log("Bail camera trace ready: bounded pre/bail/recovery samples write to bail-camera-trace-v3.csv on the settings worker.");
    bool source_ready=false;
    for(unsigned tries=0;tries<6000;++tries) {
        std::uint8_t initialized{};std::uintptr_t live_base{},callback{};
        const auto image=reinterpret_cast<std::uintptr_t>(stock);
        const auto& profile=stock_contract::contract();
        if(!memory::read(image+profile.source_initialized_rva,initialized) ||
           !memory::read(image+profile.source_base_rva,live_base) ||
           !memory::read(image+profile.source_transform_rva,callback)) {
            log("ERROR: SourceTrial live camera slots unreadable.");return false;
        }
        const auto readiness=compatibility::camera_readiness(initialized,live_base,base,callback,reinterpret_cast<std::uintptr_t>(original_transform));
        if(readiness==compatibility::Readiness::incompatible) { log("ERROR: SourceTrial live camera slots changed.");return false; }
        if(readiness==compatibility::Readiness::ready) { source_ready=true;break; }
        Sleep(100);
    }
    if(!source_ready) { log("ERROR: SourceTrial readiness timed out; no compatibility conclusion.");return false; }
    if(!activate_transform_feed()) { log("ERROR: SourceTrial callback ownership or page protection changed.");return false; }
    void* fov{};
    const auto fov_target=reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(stock)+stock_contract::contract().write_fov_rva);
    const auto prepared=owned_hook_prepare(fov_target,reinterpret_cast<void*>(&fov_hook),&fov);
    if(prepared!=HookOk) { log("ERROR: stock FOV hook preparation failed: "+std::to_string(prepared));return false; }
    original_fov=reinterpret_cast<WriteFov>(fov);
    auto result=hook_queue_enable(fov_target);
    if(result==HookOk) result=apply_owned_hooks();
    if(result!=HookOk) { log("ERROR: stock FOV hook attach failed: "+std::to_string(result));return false; }
    log("Native camera input feed prepared without patching Skate camera code.");return true;
}
bool restore_stock_camera_feed() {
    feed_prepared=false;
    if(!transform_installed) return true;
    auto* slot=reinterpret_cast<void* volatile*>(reinterpret_cast<std::uintptr_t>(stock_module)+stock_contract::contract().source_transform_rva);
    const auto restored=dingosdk::restore_owned_slot(slot,reinterpret_cast<void*>(&transform_hook),reinterpret_cast<void*>(original_transform));
    // A replacement installed by someone else belongs to them; leave it untouched.
    transform_installed=false;return restored;
}
void clear_stock_camera_blends() {
    std::lock_guard lock(smoothing_mutex);
    reset_camera_smoothing();look_pitch=0;look_yaw=0;
}
}
