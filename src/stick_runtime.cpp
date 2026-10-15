#include "stick_runtime.h"
#include "menu.h"
#include "stock_bridge.h"
#include "native.h"
#include "game_camera_ownership.h"
#include "player_state_contract.h"
#include "camera_movement.h"
#include "jump_state_contract.h"
#include "bail_recovery_contract.h"
#include "getup_contract.h"
#include "settings.h"
#include "camera_policy.h"
#include "head_attachment.h"
#include "body_flip_runtime.h"
#include "Extension/Skater/free_flight.h"
#include "owned_hooks.h"
#include "bridge_resolver.h"
#include <chrono>
#include <mutex>
#include <atomic>
#include <fstream>
#include <cstdio>
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
std::mutex jump_mutex;
JumpEpisodeTracker jump_tracker;
std::atomic<bool> jump_contract{};
std::atomic<bool> recovery_contract{};
std::atomic<bool> getup_verified{};
std::mutex mutex;
StickLookState look;
BoardLookState board_look;
bool recenter_handoff{};
struct Request { ComfortSnapshot owner;StickAim aim;bool turn{};unsigned device{};double time{};float body_heading{};bool head_valid{},recovery{},board{};StickAxes native_axes{}; } request;
float native_heading{},displayed_pitch{};double heading_time{};
double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
// Temporary stance discovery probe: dumps candidate u32 words around the rig
// body matrix and the physics context at ~10 Hz so a stance field can be found
// by differential analysis against the nose-vs-view angle. Log-only; remove
// once the stance offset is found and audited.
void stance_scan_tick(const ComfortSnapshot& owner) {
    static double last_scan{};
    static std::ofstream scan_file;
    static bool scan_header{};
    const double time=now();
    if(time-last_scan<.2) return;
    last_scan=time;
    if(!owner.state_valid || !owner.rig || !owner.context || !owner.core) return;
    try {
        Matrix body{};
        const bool body_ok=dingosdk::memory::read(owner.rig+0x4720,body) && dingosdk::valid_flight_transform(body);
        std::uint32_t physics{};
        const bool physics_ok=dingosdk::memory::read(owner.context+player_state_contract::physics_state_offset,physics);
        std::array<unsigned char,0x800> rig_low{};
        std::array<unsigned char,0x1000> rig_main{};
        std::array<unsigned char,0x800> rig_high{};
        std::array<unsigned char,0x800> context_mid{};
        std::array<unsigned char,0x800> context_high{};
        std::array<unsigned char,0x400> selector_window{};
        std::array<unsigned char,0x100> animation_window{};
        const bool rig_low_ok=dingosdk::memory::read_bytes(owner.rig+0x3800,rig_low.data(),rig_low.size());
        const bool rig_ok=dingosdk::memory::read_bytes(owner.rig+0x4000,rig_main.data(),rig_main.size());
        const bool rig_high_ok=dingosdk::memory::read_bytes(owner.rig+0x5000,rig_high.data(),rig_high.size());
        const bool context_ok=dingosdk::memory::read_bytes(owner.context+0x1000,context_mid.data(),context_mid.size());
        const bool context_high_ok=dingosdk::memory::read_bytes(owner.context+0x1800,context_high.data(),context_high.size());
        std::uintptr_t selector{};
        const bool selector_ptr_ok=dingosdk::memory::read(owner.core+player_state_contract::selector_offset,selector) && selector;
        const bool selector_ok=selector_ptr_ok && dingosdk::memory::read_bytes(selector,selector_window.data(),selector_window.size());
        std::uintptr_t animation_owner_obj{},animation_out{};
        const bool animation_ptr_ok=dingosdk::memory::read(owner.core+player_state_contract::animation_owner,animation_owner_obj) && animation_owner_obj
            && dingosdk::memory::read(animation_owner_obj+player_state_contract::animation_output,animation_out) && animation_out;
        const bool animation_ok=animation_ptr_ok && dingosdk::memory::read_bytes(animation_out,animation_window.data(),animation_window.size());
        if(!scan_file.is_open()) {
            scan_file.open(data_directory()/"logs/stance-scan.csv",std::ios::app);
            if(!scan_file.is_open()) return;
        }
        if(!scan_header) {
            scan_header=true;
            scan_file<<"time_ms,nose_ok,nose_heading,native_heading,native_age,physics"
                <<",rd_ok,r_ok,ru_ok,c1_ok,c2_ok,s_ok,a_ok";
            const auto labels=[&](const char* prefix,std::size_t bytes,std::uintptr_t base) {
                char label[24];
                for(std::size_t i=0;i<bytes/4;++i) { std::snprintf(label,sizeof(label),",%s%x",prefix,base+i*4);scan_file<<label; }
            };
            labels("RD",rig_low.size(),0x3800);labels("R",rig_main.size(),0x4000);labels("RU",rig_high.size(),0x5000);
            labels("C1",context_mid.size(),0x1000);labels("C2",context_high.size(),0x1800);
            labels("S",selector_window.size(),0);labels("A",animation_window.size(),0);
            scan_file<<"\n";
        }
        scan_file<<static_cast<std::uint64_t>(time*1000.0)<<','<<(body_ok?1:0)<<','
            <<(body_ok ? matrix_heading(body):0.f)<<','<<native_heading<<','<<(time-heading_time)<<',';
        if(physics_ok) scan_file<<physics; else scan_file<<'x';
        scan_file<<','<<(rig_low_ok?1:0)<<','<<(rig_ok?1:0)<<','<<(rig_high_ok?1:0)
            <<','<<(context_ok?1:0)<<','<<(context_high_ok?1:0)<<','<<(selector_ok?1:0)<<','<<(animation_ok?1:0);
        const auto dump=[&](const unsigned char* data,bool ok,std::size_t bytes) {
            char word[16];
            for(std::size_t i=0;i<bytes/4;++i) {
                if(!ok) { scan_file<<",x"; continue; }
                std::uint32_t value{};std::memcpy(&value,data+i*4,4);
                std::snprintf(word,sizeof(word),",%x",value);scan_file<<word;
            }
        };
        dump(rig_low.data(),rig_low_ok,rig_low.size());
        dump(rig_main.data(),rig_ok,rig_main.size());
        dump(rig_high.data(),rig_high_ok,rig_high.size());
        dump(context_mid.data(),context_ok,context_mid.size());
        dump(context_high.data(),context_high_ok,context_high.size());
        dump(selector_window.data(),selector_ok,selector_window.size());
        dump(animation_window.data(),animation_ok,animation_window.size());
        scan_file<<'\n';
        scan_file.flush();
    } catch(...) {}
}
bool live_foot(const Request& value) {
    if(game_camera_suspended() || panel_gameplay_blocked()) return false;
    if(!compatibility::enabled()) return false;
    if(!(value.board ? value.aim.active:value.aim.capture) || !value.head_valid || now()-value.time>.1 || !stock_first_person_active() || !stock_first_person_requested()) return false;
    bool visible{};if(!overlay_available(visible) || visible) return false;
    const auto foreground=GetForegroundWindow();DWORD pid{};
    if(!foreground || !GetWindowThreadProcessId(foreground,&pid) || pid!=GetCurrentProcessId()) return false;
    const auto current=comfort_snapshot();const auto movement=live_camera_movement();
    return current.core==value.owner.core && current.entity==value.owner.entity && current.context==value.owner.context &&
        current.rig==value.owner.rig && movement.mode==(value.board ? 2:1) &&
        (!value.board || board_free_look_allowed(menu_snapshot().config,current,movement)) &&
        (!value.recovery || !movement.view_hold) &&
        value.recovery==(!menu_snapshot().config.look_only && bail_camera_locked(current,movement)) &&
        camera_look_allowed(menu_snapshot().config,current,movement);
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
    // Serialize the observation and history together: sampler, controller and
    // transform callbacks may all inspect the same native state in one frame.
    std::lock_guard jump_lock(jump_mutex);
    const auto unavailable=[&]() {
        CameraMovementSample result;result.jump_episode=jump_tracker.update({},now());return result;
    };
    const auto o=comfort_snapshot();
    if(!o.state_valid || !o.core || !game_base) return unavailable();
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
        bool non_bail{},bail_recovery=physics==pc::wipeout,positive_wipeout=physics==pc::wipeout;
        if(o.animation_valid) {
            const auto output=r.pointer(r.pointer(o.core,pc::animation_owner),pc::animation_output);
            const auto wipeout_request=r.value<std::uint8_t>(output,pc::wipeout_request_offset);
            const auto bail=r.value<std::uint8_t>(output,pc::animation_wipeout_offset);
            const auto recovery=r.value<std::uint8_t>(output,pc::recovery_offset);
            source_require(wipeout_request<=1 && bail<=1 && recovery<=1,"Animation flags invalid");
            bail_recovery=bail_recovery || wipeout_request || bail || recovery;
            positive_wipeout=positive_wipeout || wipeout_request || bail;
            non_bail=wipeout_request==0 && bail==0 && recovery==0 && physics!=pc::wipeout;
        }
        const bool mounting=(r.value<std::uint32_t>(o.context,0x13d4)&0x80)!=0;
        const auto stage=jump_state_contract::stage(offboard,parent,active,vtable,game_base,jump_contract.load());
        const auto ground=pc::normal_foot_states[0];
        const bool ground_child=offboard && active==parent+ground.offset && vtable==game_base+ground.vtable;
        const bool verified_control=recovery_contract.load() && o.animation_valid;
        const auto control_flags=verified_control && offboard ? r.value<std::uint32_t>(o.context,bail_recovery_contract::control_flags_offset):0u;
        const auto motion_override=verified_control && ground_child ? r.value<std::uint8_t>(active,bail_recovery_contract::ground_override_offset):0u;
        source_require(motion_override<=1,"Ground motion override invalid");
        Matrix body{};
        const bool body_valid=memory::read(o.rig+0x4720,body) && valid_flight_transform(body);
        r.verify();
        CameraMovementSample result;
        result.mode=offboard ? 1:2;result.custom_mode=camera_movement_mode(true,offboard,parent,active,vtable,game_base,physics);
        result.position_valid=body_valid;result.position={body[12],body[13],body[14]};
        result.body=body;result.body_matrix_valid=body_valid&&right_handed(body);
        // rig+0x4a38 holds a stance-signed blend: positive regular, negative fakie.
        float stance{};
        result.stance_valid=memory::read(o.rig+0x4a38,stance) && std::isfinite(stance) && std::abs(stance)<=1.001f;
        result.stance_blend=result.stance_valid ? stance:0.f;
        result.non_bail=non_bail;result.bail_recovery=bail_recovery;result.mounting=mounting;
        result.recovery_verified=verified_control;result.recovery_flags=control_flags;result.recovery_override=motion_override!=0;
        result.recovery_controllable=verified_control && bail_recovery_contract::controllable(ground_child,control_flags,
            motion_override!=0,mounting,positive_wipeout);
        result.getting_up=offboard && getup_verified.load() && verified_control &&
            getup_contract::requested(parent,active,vtable,game_base,control_flags);
        result.jumping=stage==JumpStage::falling;
        result.view_hold=bail_view_suspended.load();result.state={physics,parent,active,vtable};
        { std::lock_guard lock(hippy_mutex);
          if(hippy_owner==std::array<std::uintptr_t,4>{o.core,o.entity,o.context,o.rig}) result.hippy=hippy_detector.state(); }
        // Do not expose stale controller confirmation after a live landing/remount.
        // The detector itself is still advanced only by the controller tick.
        if(stage==JumpStage::ground || stage==JumpStage::board || stage==JumpStage::unknown || bail_camera_locked(o,result)) result.hippy={};
        JumpEpisodeEvidence evidence;
        evidence.owner={o.core,o.entity,o.context,o.rig};evidence.valid=non_bail || bail_recovery;
        evidence.bail=bail_recovery || bail_camera_locked(o,result);evidence.stage=stage;
        evidence.position_valid=body_valid;evidence.position={body[12],body[13],body[14]};evidence.hippy=result.hippy;
        result.jump_episode=jump_tracker.update(evidence,now());
        const auto flip=body_flip_status();
        if(flip.owner==std::array<std::uintptr_t,4>{o.core,o.entity,o.context,o.rig}) {
            result.body_flip_head_lock=flip.owns_head();result.body_flip_camera_owned=flip.owns_camera();
            result.body_flip_tilt=flip.tilt;result.body_flip_rate=flip.rate;result.body_flip_phase=static_cast<unsigned>(flip.phase);
        }
        return result;
    } catch(...) { return unavailable(); }
}
void set_bail_view_suspended(bool suspended) { bail_view_suspended=suspended; }
void tick_hippy_jump(const Config& config,const ComfortSnapshot& owner,CameraMovementSample& movement,double time) {
    HippyJumpEvidence evidence;
    evidence.owner=owner.core;evidence.physics=static_cast<std::uint32_t>(movement.state[0]);
    evidence.bail=bail_camera_locked(owner,movement);evidence.falling=movement.jump_episode.stage==JumpStage::falling;
    evidence.on_board=movement.mode==2;
    evidence.grounded=movement.jump_episode.stage==JumpStage::ground;
    if(config.stick.enabled && stock_first_person_requested() && owner.state_valid && movement.mode && movement.non_bail &&
        movement.jump_episode.stage!=JumpStage::unknown) {
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
void stop_stick_look() { std::lock_guard lock(mutex);look.reset();board_look.reset();request={}; }
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
StickInputPolicy stick_input_policy(unsigned slot) {
    const auto value=snapshot();
    if(slot!=~0u && (slot>=4 || value.device>15 || !(value.device&(1u<<slot)))) return {};
    if(!value.aim.capture || !live_foot(value)) return {};
    if(!value.board) return {~0u,{}};
    const auto settings=menu_snapshot().config.board;
    return settings.enabled ? StickInputPolicy{settings.modifier,value.native_axes}:StickInputPolicy{};
}
std::uint32_t stick_capture_mask(unsigned slot) { return stick_input_policy(slot).mask; }
bool stick_captures_input(unsigned slot,std::uint32_t buttons) {
    const auto mask=stick_capture_mask(slot);
    return mask && (mask==~0u || (buttons&mask)==mask);
}
void tick_stick_look(const Config& config,const GameplayInput& input,const ComfortSnapshot& owner,bool inhibited) {
    stance_scan_tick(owner);
    const double time=now();const auto movement=live_camera_movement();
    const auto head=read_animation_head(owner);
    Matrix body{};const bool body_valid=owner.rig && dingosdk::memory::read(owner.rig+0x4720,body) && dingosdk::valid_flight_transform(body);
    const auto effective=effective_camera_config(config,stock_first_person_requested(),owner,movement);
    const bool eligible=ready && !inhibited && movement.mode==1 && camera_look_allowed(config,owner,movement) &&
        (!bail_camera_locked(owner,movement) || !movement.view_hold) && head.valid && body_valid;
    std::lock_guard lock(mutex);
    if(bail_camera_locked(owner,movement) || movement.mode==2) recenter_handoff=true;
    if(movement.mode==2) {
        look.reset();
        const bool board_eligible=ready && !inhibited && board_free_look_allowed(config,owner,movement) && head.valid && body_valid;
        if(request.owner.core && (!request.board || request.owner.core!=owner.core || request.owner.entity!=owner.entity ||
            request.owner.context!=owner.context || request.owner.rig!=owner.rig)) board_look.reset();
        const auto aim=board_look.update(config.board,{input.right_x,input.right_y},board_eligible,input,time);
        request={owner,aim,false,input.device,time,0,head.valid,false,true,board_look.native_axes(config.board)};
        return;
    }
    board_look.reset();
    if(request.board) look.reset();
    if(inhibited || !config.stick.enabled || !input.available || !input.device) {
        look.reset();board_look.reset();request={};return;
    }
    if(!eligible || time-heading_time>=.5) { look.reset();board_look.reset();request={};return; }
    const float body_heading=matrix_heading(body);
    if(request.owner.core && std::array<std::uintptr_t,4>{request.owner.core,request.owner.entity,request.owner.context,request.owner.rig}!=std::array<std::uintptr_t,4>{owner.core,owner.entity,owner.context,owner.rig}) { look.reset(); }
    const bool turn=!config.look_only && config.stick.turn_character && character_facing_allowed(owner,movement);
    const bool recovery=!config.look_only && bail_camera_locked(owner,movement);
    if(request.owner.core && request.recovery!=recovery) look.reset();
    if(turn && !request.turn) {
        if(recenter_handoff) {
            // Camera +Z points backward; after dismount/bail, face away from the body.
            look.recenter_facing();recenter_handoff=false;
            log("On-foot Camera and character look: recentering after board/bail handoff; waiting for neutral stick.");
        }
        else look.rebase_facing(body_heading);
    }
    auto stick=config.stick;if(recovery) stick.turn_character=false;
    auto aim=look.update(stick,{input.right_x,input.right_y},true,input.device,time,recovery ? 0:native_heading,
        recovery ? 0:body_heading,effective.stick_spring,recovery ? 0:displayed_pitch);
    request={owner,aim,turn,input.device,time,body_heading,head.valid,recovery};
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
    log("On-foot stick look ready: native motion/facing chained; on-board stick input captured only while the configured modifier is held.");return true;
}
bool install_stick_runtime(std::uintptr_t base) {
    getup_verified=getup_contract::verified([](std::uintptr_t address,void* data,std::size_t size) {
        return dingosdk::memory::read_bytes(address,data,size);
    },base);
    log(getup_verified ? "Get-up camera lock verified: native ragdoll animation request, camera-only hold.":
        "ERROR: get-up animation contract differs; get-up camera lock unavailable.");
    recovery_contract=bail_recovery_contract::verified([](std::uintptr_t address,void* data,std::size_t size) {
        return dingosdk::memory::read_bytes(address,data,size);
    },base);
    log(recovery_contract ? "Bail recovery control verified: owned Ground child, native selection flags and unoverridden input path.":
        "ERROR: bail recovery input contract differs; bail look remains blocked.");
    jump_contract=jump_state_contract::verified([](std::uintptr_t address,void* data,std::size_t size) {
        return dingosdk::memory::read_bytes(address,data,size);
    },base);
    log(jump_contract ? "Jump episode contracts verified: Trajectory/Falling/SimulatedRagdoll; ground/board/bail terminate attachment.":
        "ERROR: extended jump episode contract differs; Trajectory/SimulatedRagdoll recognition disabled.");
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
