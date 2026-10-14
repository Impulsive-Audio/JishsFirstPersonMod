#include "body_flip_runtime.h"
#include "camera_policy.h"
#include "head_attachment.h"
#include "Engine/Core/Platform/memory.h"
#include "Extension/Skater/free_flight.h"
#include <mutex>
#include <chrono>
namespace jishsfirstpersonmod {
namespace { std::mutex flip_mutex;body_flip::Detector detector; }
body_flip::Status body_flip_status() { std::lock_guard lock(flip_mutex);auto status=detector.status();const auto now=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();if(status.sampled && now-status.sampled>.250) { detector.reset();status=detector.status(); }return status; }
void reset_body_flip() { std::lock_guard lock(flip_mutex);detector.reset(); }
void tick_body_flip(const Config& config,const ComfortSnapshot& owner,const CameraMovementSample& movement,double now,bool interrupted) {
    body_flip::Evidence e;e.owner={owner.core,owner.entity,owner.context,owner.rig};
    dingosdk::first_person::Matrix body{};
    e.valid=!interrupted && owner.state_valid && owner.animation_valid && !bail_camera_locked(owner,movement) &&
        movement.position_valid && owner.rig && dingosdk::memory::read(owner.rig+0x4720,body) && dingosdk::valid_flight_transform(body) && read_animation_head(owner).valid;
    e.enabled=config.lock_body_flips && !config.look_only;
    e.airborne=movement.mode==2 && movement.non_bail && (movement.state[0]==200 || movement.state[0]==201) &&
        !movement.mounting && !movement.hippy.active && !movement.hippy.candidate;
    e.up={body[4],body[5],body[6]};e.position=movement.position;
    std::lock_guard lock(flip_mutex);const auto before=detector.status();const auto after=detector.update(e,now);
    if(before.phase!=after.phase) log(std::string("Body flip: ")+body_flip::phase_name(after.phase)+" tilt="+std::to_string(after.tilt)+" rate="+std::to_string(after.rate));
}
}
