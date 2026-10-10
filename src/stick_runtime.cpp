#include "stick_runtime.h"
#include "stock_bridge.h"
#include "native.h"
#include "game_camera_ownership.h"
#include "player_state_contract.h"
#include "camera_movement.h"
#include "settings.h"
#include "camera_policy.h"
#include "head_attachment.h"
#include "Extension/Skater/free_flight.h"
#include "owned_hooks.h"
#include "bridge_resolver.h"
#include <chrono>
#include <mutex>
#include <atomic>
namespace jishsfirstpersonmod {
namespace {
using Matrix=dingosdk::first_person::Matrix;
using Motion=void(*)(std::uintptr_t,std::uintptr_t,const Matrix*,std::uint8_t);
Motion original_motion{};
std::uintptr_t game_base{};
std::atomic<bool> ready{};
std::atomic<bool> camera_suspended{},bail_view_suspended{};
std::mutex hippy_mutex;
HippyJumpDetector hippy_detector;
std::array<std::uintptr_t,4> hippy_owner{};
std::mutex mutex;
StickLookState look;
struct Request { ComfortSnapshot owner;StickAim aim;bool turn{};unsigned device{};double time{};float body_heading{};bool head_valid{}; } request;
float native_heading{},displayed_pitch{};double heading_time{};
double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
bool live_foot(const Request& value) {
    if(game_camera_suspended()) return false;
    if(!compatibility::enabled()) return false;
    if(!value.aim.capture || !value.head_valid || now()-value.time>.1 || !stock_first_person_active() || !stock_first_person_requested()) return false;
    bool visible{};if(!overlay_available(visible) || visible) return false;
    const auto foreground=GetForegroundWindow();DWORD pid{};
    if(!foreground || !GetWindowThreadProcessId(foreground,&pid) || pid!=GetCurrentProcessId()) return false;
    const auto current=comfort_snapshot();const auto movement=live_camera_movement();
    return current.core==value.owner.core && current.entity==value.owner.entity && current.context==value.owner.context &&
        current.rig==value.owner.rig && movement.mode==1 && camera_look_allowed(menu_snapshot().config,current,movement);
}
Request snapshot() { std::lock_guard lock(mutex);return request; }
void motion_hook(std::uintptr_t rig,std::uintptr_t context,const Matrix* supplied,std::uint8_t flags) {
    const auto error=GetLastError();const auto value=snapshot();
    if(menu_snapshot().config.look_only) { original_motion(rig,context,supplied,flags);return; }
    alignas(16) Matrix matrix{};
    if(value.turn && value.aim.active && character_facing_allowed(comfort_snapshot(),live_camera_movement()) && rig==value.owner.rig && context==value.owner.context && live_foot(value) && supplied &&
       dingosdk::memory::read(reinterpret_cast<std::uintptr_t>(supplied),matrix) && dingosdk::valid_flight_transform(matrix)) {
        matrix=face_heading(matrix,value.aim.body_heading);
        SetLastError(error);original_motion(rig,context,&matrix,flags);return;
    }
    SetLastError(error);original_motion(rig,context,supplied,flags);
}
}
bool stick_runtime_ready() { return ready.load(); }
CameraMovementSample live_camera_movement() {
    using namespace dingosdk;
    using namespace dingosdk::client_source::detail;
    namespace pc=player_state_contract;
    const auto o=comfort_snapshot();
    if(!o.state_valid || !o.core || !game_base) return {};
    try {
        SourceReader r;
        const auto world=r.value<std::uint32_t>(o.client,0xc4);
        source_require(world==13 || world==21,"Outside gameplay");
        source_require(r.pointer(o.core)==game_base+pc::core_vtable && r.pointer(o.core,0x438)==o.rig &&
            r.pointer(o.core,0x3c0)==o.context && r.pointer(o.rig)==o.context && r.pointer(o.rig,0x4630)==o.core &&
            r.pointer(r.pointer(o.entity,0x628),0x70)==o.core && r.value<std::uint8_t>(o.entity,0x7e0)==0,"Camera owner changed");
        const auto parent=r.pointer(o.core,pc::movement_parent_offset);
        const bool offboard=r.pointer(parent)==game_base+pc::offboard_vtable;
        std::uintptr_t active{},vtable{};
        if(offboard) {
            source_require(r.pointer(parent,8)==o.context && r.pointer(parent,0x18)==o.rig &&
                r.pointer(parent,0x10)==r.pointer(o.core,0x430),"Offboard camera owner changed");
            active=r.pointer(parent,pc::active_substate_offset);vtable=r.pointer(active);
            MEMORY_BASIC_INFORMATION page{};
            source_require(VirtualQuery(reinterpret_cast<void*>(vtable),&page,sizeof(page)) &&
                reinterpret_cast<std::uintptr_t>(page.AllocationBase)==game_base,"Offboard child type is not owned by Skate");
        }
        const auto physics=r.value<std::uint32_t>(o.context,pc::physics_state_offset);
        bool non_bail{},bail_recovery=physics==pc::wipeout;
        if(o.animation_valid) {
            const auto output=r.pointer(r.pointer(o.core,pc::animation_owner),pc::animation_output);
            const auto wipeout_request=r.value<std::uint8_t>(output,pc::wipeout_request_offset);
            const auto bail=r.value<std::uint8_t>(output,pc::animation_wipeout_offset);
            const auto recovery=r.value<std::uint8_t>(output,pc::recovery_offset);
            source_require(wipeout_request<=1 && bail<=1 && recovery<=1,"Animation flags invalid");
            bail_recovery=bail_recovery || wipeout_request || bail || recovery;
            non_bail=wipeout_request==0 && bail==0 && recovery==0 && physics!=pc::wipeout;
        }
        const bool mounting=(r.value<std::uint32_t>(o.context,0x13d4)&0x80)!=0;
        const bool jumping=offboard && active==parent+pc::normal_foot_states[2].offset &&
            vtable==game_base+pc::normal_foot_states[2].vtable;
        r.verify();
        CameraMovementSample result;
        result.mode=offboard ? 1:2;result.custom_mode=camera_movement_mode(true,offboard,parent,active,vtable,game_base,physics);
        result.non_bail=non_bail;result.bail_recovery=bail_recovery;result.mounting=mounting;result.jumping=jumping;
        result.view_hold=bail_view_suspended.load();result.state={physics,parent,active,vtable};
        { std::lock_guard lock(hippy_mutex);
          if(hippy_owner==std::array<std::uintptr_t,4>{o.core,o.entity,o.context,o.rig}) result.hippy=hippy_detector.state(); }
        return result;
    } catch(...) { return {}; }
}
void set_bail_view_suspended(bool suspended) { bail_view_suspended=suspended; }
void tick_hippy_jump(const Config& config,const ComfortSnapshot& owner,CameraMovementSample& movement,double time) {
    HippyJumpEvidence evidence;
    evidence.owner=owner.core;evidence.physics=static_cast<std::uint32_t>(movement.state[0]);
    evidence.bail=bail_camera_locked(owner,movement);evidence.falling=movement.jumping;
    evidence.on_board=movement.mode==2;
    evidence.grounded=movement.mode==1 && movement.custom_mode==1 && !movement.jumping;
    if(config.stick.enabled && stock_first_person_requested() && owner.state_valid && movement.mode && movement.non_bail) {
        try {
            dingosdk::overlay::DebugModel model;
            const auto pose=dingosdk::client_source::detail::debug_skater(game_base,owner.client,model);
            evidence.valid=model.skater_position_valid && model.skater_identity==owner.entity;
            evidence.position={pose[12],pose[13],pose[14]};
        } catch(...) {}
    }
    std::lock_guard lock(hippy_mutex);
    const std::array<std::uintptr_t,4> identity{owner.core,owner.entity,owner.context,owner.rig};
    if(identity!=hippy_owner) { hippy_detector.reset();hippy_owner=identity; }
    movement.hippy=hippy_detector.update(evidence,time);
}
int live_camera_movement_mode() { return live_camera_movement().custom_mode; }
void stop_stick_look() { std::lock_guard lock(mutex);look.reset();request={}; }
void suspend_custom_camera() { stop_stick_look();camera_suspended=true; }
bool take_camera_suspension() { return camera_suspended.exchange(false); }
void note_camera_view(const dingosdk::first_person::Matrix& pose,dingosdk::first_person::Quat glance) {
    using namespace dingosdk::first_person;
    const auto base=normalized(multiply(orientation(pose),conjugate(glance)));
    const auto back=rotate(base,{0,0,1});
    std::lock_guard lock(mutex);native_heading=std::atan2(back[0],back[2])*180/std::numbers::pi_v<float>;
    displayed_pitch=std::asin(std::clamp(-back[1],-1.f,1.f))*180/std::numbers::pi_v<float>;
    heading_time=now();
}
void note_native_camera_heading(const dingosdk::first_person::Matrix& pose) { note_camera_view(pose,{0,0,0,1}); }
StickAim stick_camera_aim() { const auto value=snapshot();return live_foot(value) ? value.aim:StickAim{}; }
bool stick_captures_input(unsigned slot) {
    const auto value=snapshot();
    if(slot!=~0u && (slot>=4 || value.device>15 || !(value.device&(1u<<slot)))) return false;
    return live_foot(value);
}
void tick_stick_look(const Config& config,const GameplayInput& input,const ComfortSnapshot& owner,bool inhibited) {
    const double time=now();const auto movement=live_camera_movement();
    const auto head=read_animation_head(owner);
    Matrix body{};const bool body_valid=owner.rig && dingosdk::memory::read(owner.rig+0x4720,body) && dingosdk::valid_flight_transform(body);
    const auto effective=effective_camera_config(config,stock_first_person_requested(),owner,movement);
    const bool eligible=ready && !inhibited && movement.mode==1 && camera_look_allowed(config,owner,movement) && head.valid && body_valid;
    std::lock_guard lock(mutex);
    if(inhibited || !config.stick.enabled || !input.available || !input.device) {
        look.reset();request={};return;
    }
    if(!eligible || time-heading_time>=.5) { look.reset();request={};return; }
    const float body_heading=matrix_heading(body);
    if(request.owner.core && std::array<std::uintptr_t,4>{request.owner.core,request.owner.entity,request.owner.context,request.owner.rig}!=std::array<std::uintptr_t,4>{owner.core,owner.entity,owner.context,owner.rig}) { look.reset(); }
    const bool turn=!config.look_only && config.stick.turn_character && character_facing_allowed(owner,movement);
    if(turn && !request.turn) look.rebase_facing(body_heading);
    auto aim=look.update(config.stick,{input.right_x,input.right_y},true,input.device,time,native_heading,body_heading,glance_settings(effective),displayed_pitch);
    request={owner,aim,turn,input.device,time,body_heading,head.valid};
}
bool attach_stick_runtime(std::uintptr_t base) {
    using namespace dingosdk;
    namespace spawn=game::build::v20260929::client_source_spawn;
    // ABI and consumer are already proven by custom.2's noclip motion hook.
    const auto target=resolve_stock_handler(base+spawn::skater_motion);
    if(!target || !stock_bridge_ready()) { log("ERROR: native on-foot facing hook unavailable: verified stock motion handler missing.");return false; }
    game_base=base;void* original{};
    auto status=owned_hook_prepare(reinterpret_cast<void*>(target),reinterpret_cast<void*>(&motion_hook),&original);
    if(status==HookOk) {
        original_motion=reinterpret_cast<Motion>(original);
        status=hook_queue_enable(reinterpret_cast<void*>(target));
        if(status==HookOk) status=apply_owned_hooks();
    }
    if(status!=HookOk) { log("ERROR: native facing hook attach failed: "+std::to_string(status));return false; }
    if(!install_stick_input_filter()) { log("ERROR: stick input filters unavailable; stick look remains disabled.");return false; }
    ready=true;
    log("On-foot stick look ready: native motion/facing chained; board input forwarded unchanged.");return true;
}
bool install_stick_runtime(std::uintptr_t base) {
    namespace spawn=dingosdk::game::build::v20260929::client_source_spawn;
    if(resolve_stock_handler(base+spawn::skater_motion)) return attach_stick_runtime(base);
    // Stock installs the motion bridge after its native source initializes.
    // Wait outside the game/loader thread instead of permanently disabling an
    // early-loaded DLL or attaching another hook directly to Skate.exe.
    log("Waiting for stock motion bridge on initialization worker.");
    for(unsigned attempt=0;attempt<3000;++attempt) {
        if(resolve_stock_handler(base+spawn::skater_motion)) return attach_stick_runtime(base);
        Sleep(100);
    }
    log("ERROR: stock motion bridge readiness timed out.");return false;
}
}
