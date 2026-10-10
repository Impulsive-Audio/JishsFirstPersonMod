#include "bail_camera_trace.h"
#include "stock_bridge.h"
#include "Engine/Core/Platform/memory.h"
#include "Extension/Skater/free_flight.h"

#include <deque>
#include <vector>
#include <mutex>
#include <atomic>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "rotating_log.h"
namespace jishsfirstpersonmod {
namespace {
using namespace dingosdk::first_person;
struct Sample {
    std::uint64_t time{},event{};
    int phase{},native_spring{};
    ComfortSnapshot state;CameraMovementSample movement;
    Matrix native_pose{},final_pose{},facing{},root{},head{};
    bool lock_hippy_jump{},facing_valid{},head_valid{},saved_spring{},effective_spring{},requested{},active{},
        stick_enabled{},stick_active{},turn_character{},disable_foot{},lock_mount_jump{};
    float pitch{},yaw{};BailViewInfo view;
};
std::mutex trace_mutex;
std::deque<Sample> pre;
std::vector<Sample> pending;
std::uint64_t event{},start{},until{},next_sample{};
std::uintptr_t owner{};
bool previous_bail{};
std::atomic<unsigned> dropped{};
void matrix_fields(std::ostream& out,const Matrix& pose) {
    for(unsigned i=12;i<15;++i) out<<','<<pose[i];
    for(const auto value:orientation(pose)) out<<','<<value;
}
}
void record_bail_camera(const Matrix& native_pose,const Matrix& final_pose,const ComfortSnapshot& state,
    const CameraMovementSample& movement,const Config& config,const StickAim& aim,float pitch,float yaw,const BailViewInfo& view,const HeadPose& head) noexcept {
    try {
        std::unique_lock lock(trace_mutex,std::try_to_lock);if(!lock.owns_lock()) { ++dropped;return; }
        const auto now=GetTickCount64();
        const bool bail=bail_camera_locked(state,movement) || view.blocked || movement.hippy.active || movement.hippy.candidate;
        if(owner!=state.core) { owner=state.core;pre.clear();previous_bail=false;start=until=next_sample=0; }
        const bool changed=bail!=previous_bail;
        if(now<next_sample && !changed) return;
        next_sample=now+50;
        if(bail && !previous_bail) {
            ++event;start=now;
            for(auto sample:pre) { sample.event=event;sample.phase=0;if(pending.size()<1024) pending.push_back(sample);else ++dropped; }
            pre.clear();
        }
        if(bail) until=now+2000;
        previous_bail=bail;
        Sample sample;sample.time=now;sample.event=event;sample.phase=bail ? 1:2;
        sample.state=state;sample.movement=movement;sample.view=view;
        sample.head_valid=head.valid;sample.root=head.root;sample.head=head.head;sample.native_pose=native_pose;sample.final_pose=final_pose;
        sample.saved_spring=config.spring.enabled;sample.requested=stock_first_person_requested();sample.active=stock_first_person_active();
        sample.effective_spring=effective_camera_config(config,sample.requested,state,movement).spring.enabled;
        sample.native_spring=stock_camera_spring_enabled();sample.pitch=pitch;sample.yaw=yaw;
        sample.stick_enabled=config.stick.enabled;sample.stick_active=aim.active;sample.turn_character=config.stick.turn_character;
        sample.lock_hippy_jump=config.stick.lock_hippy_jump;sample.disable_foot=config.disable_spring_on_foot;sample.lock_mount_jump=config.stick.lock_mount_jump;
        if(state.state_valid && state.rig) {
            sample.facing_valid=dingosdk::memory::read(state.rig+0x4720,sample.facing) && dingosdk::valid_flight_transform(sample.facing);
        }
        if(start && now<until && now-start<=15000) {
            if(pending.size()<1024) pending.push_back(sample);else ++dropped;
        } else if(!bail) { pre.push_back(sample);while(pre.size()>20) pre.pop_front(); }
    } catch(...) { ++dropped; }
}
void flush_bail_camera_trace() noexcept {
    try {
        std::vector<Sample> batch;
        { std::lock_guard lock(trace_mutex);batch.swap(pending); }
        if(batch.empty()) return;
        const auto path=data_directory()/"bail-camera-trace-v3.csv";
        std::filesystem::create_directories(path.parent_path());
        const std::string_view header="time_ms,event,phase,mode,custom_mode,physics,active_child,child_vtable,state_valid,animation_valid,wipeout_request,animation_wipeout,recovery,bail_latched,selector_valid,selector_current,selector_next,requested,active,saved_spring,effective_spring,native_spring,disable_foot,lock_mount_jump,mounting,jumping,stick_enabled,stick_active,turn_character,look_pitch,look_yaw,native_x,native_y,native_z,native_qx,native_qy,native_qz,native_qw,final_x,final_y,final_z,final_qx,final_qy,final_qz,final_qw,facing_valid,facing_x,facing_y,facing_z,facing_qx,facing_qy,facing_qz,facing_qw,head_valid,root_x,root_y,root_z,root_qx,root_qy,root_qz,root_qw,head_x,head_y,head_z,head_qx,head_qy,head_qz,head_qw,camera_head_distance_m,custom_position_delta_m,custom_angle_delta_deg,dropped,hold_phase,view_blocked,head_attached,offset_calibrated,offset_x,offset_y,offset_z,lock_hippy_jump,hippy_candidate,hippy_active,hippy_rising_samples,hippy_up_speed,reconciling,reconcile_remaining_deg,manual_takeover,facing_eligible\n";
        std::ostringstream out;
        out<<std::setprecision(9);
        for(const auto& s:batch) {
            const auto& o=s.state;const auto& m=s.movement;
            out<<s.time<<','<<s.event<<','<<s.phase<<','<<m.mode<<','<<m.custom_mode<<','<<m.state[0]<<','<<m.state[2]<<','<<m.state[3]
                <<','<<o.state_valid<<','<<o.animation_valid<<','<<o.wipeout_request<<','<<o.animation_wipeout<<','<<o.recovery<<','<<o.bailing
                <<','<<o.selector_valid<<','<<o.selector_current<<','<<o.selector_next<<','<<s.requested<<','<<s.active
                <<','<<s.saved_spring<<','<<s.effective_spring<<','<<s.native_spring<<','<<s.disable_foot<<','<<s.lock_mount_jump
                <<','<<m.mounting<<','<<m.jumping<<','<<s.stick_enabled<<','<<s.stick_active<<','<<s.turn_character<<','<<s.pitch<<','<<s.yaw;
            matrix_fields(out,s.native_pose);matrix_fields(out,s.final_pose);
            out<<','<<s.facing_valid;matrix_fields(out,s.facing);
            out<<','<<s.head_valid;matrix_fields(out,s.root);matrix_fields(out,s.head);
            const Vec3 native{s.native_pose[12],s.native_pose[13],s.native_pose[14]},final{s.final_pose[12],s.final_pose[13],s.final_pose[14]};
            out<<','<<(s.head_valid ? length(subtract(final,{s.head[12],s.head[13],s.head[14]})):-1.f)
                <<','<<length(subtract(final,native))<<','<<length(rotation_vector(multiply(conjugate(orientation(s.native_pose)),orientation(s.final_pose))))*180/std::numbers::pi_v<float>
                <<','<<dropped.load()<<','<<s.view.phase<<','<<s.view.blocked<<','<<s.view.attached<<','<<s.view.calibrated
                <<','<<s.view.offset[0]<<','<<s.view.offset[1]<<','<<s.view.offset[2]<<','<<s.lock_hippy_jump
                <<','<<m.hippy.candidate<<','<<m.hippy.active<<','<<m.hippy.rising_samples<<','<<m.hippy.up_speed
                <<",0,0,0"
                <<','<<character_facing_allowed(o,m)<<'\n';
        }
        if(!append_rotating_log(path,out.str(),trace_log_limit,log_archives,header))
            log("Could not write bail-camera-trace-v3.csv; later batches will retry.");
    } catch(...) { log("Bail camera trace flush failed."); }
}
}
