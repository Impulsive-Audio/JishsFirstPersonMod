#pragma once
#include "Engine/Game/Skater/first_person_spring.h"
#include <array>
#include <algorithm>
#include <numbers>
#include <cstdint>
namespace jishsfirstpersonmod {
namespace body_flip {
using namespace dingosdk::first_person;
inline Vec3 position(const Matrix& m) { return {m[12],m[13],m[14]}; }
inline constexpr float entry_tilt=80, rearm_tilt=60, entry_rate=30, quiet_rate=20;
inline constexpr double confirmation=.060, quiet_period=.120, handoff=.250, maximum_gap=.250;
enum class Phase { idle, entering, locked, leaving };
inline const char* phase_name(Phase p) { switch(p) { case Phase::entering:return "blending_in";case Phase::locked:return "locked";case Phase::leaving:return "blending_out";default:return "idle"; } }
struct Evidence {
    std::array<std::uintptr_t,4> owner{};
    bool valid{},enabled{},airborne{};
    Vec3 up{},position{};
};
struct Status {
    Phase phase=Phase::idle;double started{},sampled{};
    std::array<std::uintptr_t,4> owner{};
    float tilt{},rate{};std::uint64_t epoch{};
    bool owns_head() const { return phase==Phase::entering || phase==Phase::locked; }
    bool owns_camera() const { return phase!=Phase::idle; }
};
class Detector {
    Status state_;
    bool ready_{},airborne_{},armed_=true;
    double time_{},rising_{},quiet_{};
    Vec3 up_{},position_{};
    std::array<std::uintptr_t,4> owner_{};
public:
    void reset() { auto epoch=state_.epoch+1;*this={};state_.epoch=epoch; }
    Status status() const { return state_; }
    Status update(const Evidence& e,double now) {
        const float magnitude=length(e.up);
        if(!e.valid || !e.owner[0] || !std::isfinite(now) || !std::isfinite(magnitude) || magnitude<.5f || magnitude>1.5f) { reset();return state_; }
        for(float v:e.position) if(!std::isfinite(v)) { reset();return state_; }
        const Vec3 up{e.up[0]/magnitude,e.up[1]/magnitude,e.up[2]/magnitude};
        if(ready_ && (owner_!=e.owner || now<time_ || now-time_>maximum_gap || length(subtract(e.position,position_))>5)) reset();
        const float tilt=std::acos(std::clamp(up[1],-1.f,1.f))*180/std::numbers::pi_v<float>;
        if(!ready_) { ready_=true;owner_=e.owner;state_.owner=e.owner;state_.sampled=now;up_=up;position_=e.position;time_=now;airborne_=e.airborne;state_.tilt=tilt;return state_; }
        const double dt=now-time_;
        if(dt<=.000001) return state_;
        const float dot=up_[0]*up[0]+up_[1]*up[1]+up_[2]*up[2];
        state_.rate=std::acos(std::clamp(dot,-1.f,1.f))*180/std::numbers::pi_v<float>/static_cast<float>(dt);
        const float rising_rate=(tilt-state_.tilt)/static_cast<float>(dt);
        state_.tilt=tilt;
        if(!airborne_ && e.airborne) armed_=true;
        if(tilt<rearm_tilt) armed_=true;
        const bool allowed=e.enabled && e.airborne;
        if(allowed && rising_rate>entry_rate) rising_+=dt;else rising_=0;
        if(state_.owns_head()) {
            quiet_=state_.rate<quiet_rate ? quiet_+dt:0;
            if(!allowed || quiet_+1e-9>=quiet_period) { state_.phase=Phase::leaving;state_.started=now;armed_=false;rising_=quiet_=0; }
            else if(state_.phase==Phase::entering && now-state_.started+1e-9>=handoff) state_.phase=Phase::locked;
        } else if(state_.phase==Phase::leaving) {
            if(now-state_.started+1e-9>=handoff) state_.phase=Phase::idle;
        } else if(allowed && armed_ && tilt>=entry_tilt && rising_+1e-9>=confirmation) {
            state_.phase=Phase::entering;state_.started=now;quiet_=0;armed_=false;
        }
        if(!allowed) rising_=0;
        state_.sampled=now;time_=now;up_=up;position_=e.position;airborne_=e.airborne;
        return state_;
    }
};
inline Quat interpolate(Quat a,Quat b,float t) {
    a=normalized(a);b=normalized(b);
    float dot=0;for(unsigned i=0;i<4;++i) dot+=a[i]*b[i];
    if(dot<0) { for(auto& v:b) v=-v;dot=-dot; }
    dot=std::clamp(dot,0.f,1.f);
    float wa=1-t,wb=t;
    if(dot<.9995f) { const float angle=std::acos(dot),s=std::sin(angle);wa=std::sin((1-t)*angle)/s;wb=std::sin(t*angle)/s; }
    Quat q{};for(unsigned i=0;i<4;++i) q[i]=wa*a[i]+wb*b[i];return normalized(q);
}
// Use explicit camera offsets, never a delta captured from a lagging Spring view.
inline Matrix head_target(const Matrix& head,const Settings& settings) {
    return dingosdk::first_person::target(head,settings);
}
class Camera {
    Matrix last_{},from_{},destination_{};
    Phase phase_=Phase::idle;
    std::uint64_t epoch_{};
    bool ready_{};
public:
    void reset() { *this={}; }
    Matrix apply(const Matrix& normal,const Matrix& head,const Status& state,double now) {
        if(!ready_ || epoch_!=state.epoch) { last_=normal;from_=normal;phase_=Phase::idle;ready_=true;epoch_=state.epoch; }
        if(state.phase!=phase_) {
            from_=last_;phase_=state.phase;
            destination_=state.phase==Phase::entering ? head:normal;
        }
        auto output=normal;
        if(state.phase==Phase::locked) output=head;
        else if(state.phase==Phase::entering || state.phase==Phase::leaving) {
            const auto& target=state.phase==Phase::entering ? head:normal;
            const float t=std::clamp(static_cast<float>((now-state.started)/handoff),0.f,1.f),w=t*t*(3-2*t);
            // Carry the handoff with its moving destination instead of anchoring
            // the camera in world space while a fast flip pulls the head away.
            const auto initial_rotation=orientation(destination_);
            const Quat inverse{-initial_rotation[0],-initial_rotation[1],-initial_rotation[2],initial_rotation[3]};
            const auto offset=rotate(inverse,subtract(position(from_),position(destination_)));
            Vec3 residual{};for(unsigned i=0;i<3;++i) residual[i]=offset[i]*(1-w);
            const auto target_rotation=orientation(target);
            const auto rotation_offset=normalized(multiply(inverse,orientation(from_)));
            write(output,normalized(multiply(target_rotation,interpolate(rotation_offset,{0,0,0,1},w))),
                add(position(target),rotate(target_rotation,residual)));
        }
        last_=output;return output;
    }
};
}
}
