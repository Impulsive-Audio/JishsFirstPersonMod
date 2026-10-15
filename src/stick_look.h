#pragma once
#include "Engine/Game/Skater/first_person_spring.h"
#include <algorithm>
#include <cmath>
namespace jishsfirstpersonmod {
struct StickSettings {
    bool enabled{},invert_y{};
    float yaw_speed=120,pitch_speed=90,deadzone=.15f,head_motion=100;
    bool smooth_transitions=true;
    bool lock_mount_jump=true;
    bool lock_hippy_jump=false;
    bool look_during_parkour=false;
    bool lean_look_down=true;
    float lean_forward=0.086f,lean_down=0.077f;
    float mount_smoothing=70,dismount_smoothing=35;
};
struct StickAxes { float x{},y{}; };
inline float normalize_axis(short value) { return value<0 ? value/32768.f:value/32767.f; }
inline float wrap_heading(float heading) { return std::remainder(heading,360.f); }
inline StickAxes radial_stick(StickAxes input,float deadzone) {
    if(!std::isfinite(input.x) || !std::isfinite(input.y)) return {};
    const auto length=std::hypot(input.x,input.y);
    if(length<=deadzone) return {};
    const float scale=(std::min(length,1.f)-deadzone)/((1-deadzone)*length);
    return {input.x*scale,input.y*scale};
}
inline dingosdk::first_person::Vec3 look_down_lean(const dingosdk::first_person::Matrix& pose,float pitch,float forward_max,float down_max) {
    // Lean at full look-down: forward_max metres ahead, down_max metres down,
    // scaled linearly from zero at level view. Position-only, orientation untouched.
    const float fraction=std::max(0.f,-pitch)/85.f;
    if(fraction<=0.f) return {};
    const float x=pose[8],z=pose[10];
    const float length=std::hypot(x,z);
    if(length<1e-4f) return {};
    const float forward=forward_max*fraction/length,down=down_max*fraction;
    return {-x*forward,-down,-z*forward};
}
inline dingosdk::first_person::Vec3 yaw_anchored_position(const dingosdk::first_person::Matrix& head,
    const dingosdk::first_person::Vec3& offset) {
    // Camera position anchored to the head with the offset rotated by the
    // head's yaw only: head pitch and roll must not swing the calibrated
    // offset around the head during steep looks.
    const float yaw=std::atan2(head[8],head[10]);
    const float half=yaw*0.5f;
    return dingosdk::first_person::add({head[12],head[13],head[14]},
        dingosdk::first_person::rotate(dingosdk::first_person::Quat{0,std::sin(half),0,std::cos(half)},offset));
}
inline float rotation_angle(const dingosdk::first_person::Quat& q) { return 2.f*std::acos(std::clamp(std::fabs(q[3]),0.f,1.f)); }
inline dingosdk::first_person::Quat scale_rotation(const dingosdk::first_person::Quat& rotation,float strength) {
    using namespace dingosdk::first_person;
    return normalized(Quat{rotation[0]*strength,rotation[1]*strength,rotation[2]*strength,1.f+(rotation[3]-1.f)*strength});
}
struct HeadWobbleState {
    dingosdk::first_person::Quat average_delta{0,0,0,1},wobble{0,0,0,1},previous{0,0,0,1};
    double last{};
};
// Extracts the native head camera's high-frequency wobble: per-frame deltas
// are compared against their own slow average so sustained turns cancel, and
// only bob-rate deltas accumulate (with a short leak and a hard angle cap).
inline dingosdk::first_person::Quat track_head_wobble(HeadWobbleState& state,
    const dingosdk::first_person::Quat& orientation,double now) {
    using namespace dingosdk::first_person;
    if(state.last==0 || now-state.last>0.1) {
        state.previous=orientation;state.average_delta={0,0,0,1};state.wobble={0,0,0,1};state.last=now;
        return {0,0,0,1};
    }
    const float dt=std::clamp(static_cast<float>(now-state.last),1e-4f,.1f);state.last=now;
    auto delta=multiply(conjugate(state.previous),orientation);
    if(delta[3]<0.f) delta={-delta[0],-delta[1],-delta[2],-delta[3]};
    state.previous=orientation;
    const auto blend=[&](const Quat& a,const Quat& b,float t) {
        const float flip=a[0]*b[0]+a[1]*b[1]+a[2]*b[2]+a[3]*b[3]<0.f ? -1.f:1.f;
        return normalized(Quat{a[0]+(b[0]*flip-a[0])*t,a[1]+(b[1]*flip-a[1])*t,
                                a[2]+(b[2]*flip-a[2])*t,a[3]+(b[3]*flip-a[3])*t});
    };
    state.average_delta=blend(state.average_delta,delta,std::clamp(dt/0.4f,0.f,1.f));
    state.wobble=blend(state.wobble,{0,0,0,1},std::clamp(dt/0.3f,0.f,1.f));
    if(rotation_angle(delta)<1.2f*dt) {
        state.wobble=normalized(multiply(state.wobble,multiply(conjugate(state.average_delta),delta)));
        if(const float angle=rotation_angle(state.wobble);angle>0.07f) state.wobble=scale_rotation(state.wobble,0.07f/angle);
    }
    return state.wobble;
}
// The native camera rotation expressed in the skater body's frame: the real
// head deviation from the torso (bob and sway), bounded by the neck, so a
// turn or look sweep cannot leak into the wobble.
inline dingosdk::first_person::Quat body_referenced(const dingosdk::first_person::Matrix& body,
    const dingosdk::first_person::Matrix& camera) {
    using namespace dingosdk::first_person;
    const auto base=multiply(orientation(body),Quat{0,1,0,0});
    return normalized(multiply(conjugate(base),orientation(camera)));
}
struct StickAim { bool capture{},active{}; float heading{},pitch{},body_heading{};bool manual{};};
class StickLookState {
    bool ready_{},seen_{},center_facing_{};
    unsigned device_{};
    double time_{};
    float yaw_{},pitch_{},smooth_yaw_{},smooth_pitch_{},yaw_velocity_{},pitch_velocity_{},center_offset_{};
public:
    void reset() { *this={}; }
    void recenter_facing(float camera_offset=180.f) { reset();center_facing_=true;center_offset_=camera_offset; }
    void seed(const StickSettings& settings,unsigned device,double now,float yaw,float pitch,float body_heading) {
        reset();ready_=seen_=true;device_=device;time_=now;
        yaw_=smooth_yaw_=yaw;pitch_=smooth_pitch_=std::clamp(pitch,-85.f,85.f);
    }
    StickAim update(const StickSettings& settings,StickAxes axes,bool eligible,unsigned device,
                    double now,float camera_heading,float body_heading,
                    const dingosdk::first_person::Settings& spring,float camera_pitch=0,StickAxes move={}) {
        if(!settings.enabled || !eligible || !device || !std::isfinite(now)) { reset();return {}; }
        if(!seen_ || device_!=device || now<time_ || now-time_>.25) {
            const bool center=center_facing_;const float offset=center_offset_;
            reset();center_facing_=center;center_offset_=offset;seen_=true;device_=device;time_=now;
        }
        const auto input=radial_stick(axes,settings.deadzone);
        if(!ready_) {
            time_=now;
            if(input.x!=0 || input.y!=0) return {true,false};
            if(center_facing_) {camera_heading=wrap_heading(body_heading+center_offset_);camera_pitch=0;}
            center_facing_=false;
            ready_=true;yaw_=smooth_yaw_=camera_heading;pitch_=smooth_pitch_=std::clamp(camera_pitch,-85.f,85.f);
        }
        const float dt=static_cast<float>(now-time_);time_=now;
        // Match the proven glance conventions: +Y looks up, +X looks right.
        yaw_=wrap_heading(yaw_-input.x*settings.yaw_speed*dt);
        pitch_=std::clamp(pitch_+input.y*settings.pitch_speed*dt*(settings.invert_y ? -1.f:1.f),-85.f,85.f);
        // The left stick is walk input only: moving follows the view, so the
        // character always faces the camera heading and the native drive
        // supplies the speed. The look stick owns the camera.
        const auto move_stick=radial_stick(move,settings.deadzone);
        const bool moving=move_stick.x!=0 || move_stick.y!=0;
        if(!spring.enabled) { smooth_yaw_=yaw_;smooth_pitch_=pitch_;yaw_velocity_=pitch_velocity_=0; }
        else {
            float yaw_error=wrap_heading(smooth_yaw_-yaw_),pitch_error=smooth_pitch_-pitch_;
            dingosdk::first_person::damp(yaw_error,yaw_velocity_,yaw_error>0 ? spring.right:spring.left,dt);
            dingosdk::first_person::damp(pitch_error,pitch_velocity_,pitch_error>0 ? spring.down:spring.up,dt);
            smooth_yaw_=wrap_heading(yaw_+yaw_error);smooth_pitch_=pitch_+pitch_error;
        }
        // The character faces the view; the left stick only supplies movement.
        return {true,true,smooth_yaw_,smooth_pitch_,wrap_heading(smooth_yaw_+180.f),input.x!=0 || input.y!=0 || moving};
    }
};
inline float matrix_heading(const dingosdk::first_person::Matrix& matrix) {
    return std::atan2(matrix[8],matrix[10])*180.f/std::numbers::pi_v<float>;
}
inline dingosdk::first_person::Matrix aim_head_relative(dingosdk::first_person::Matrix pose,const StickAim& aim) {
    // StickAim heading uses mathematical yaw; LookAngles uses its inverse.
    return aim.active ? dingosdk::first_person::target(pose,{}, {aim.pitch,-aim.heading}):pose;
}
inline bool right_handed(const dingosdk::first_person::Matrix& m) {
    // Rejects mirrored frames: valid_flight_transform checks orthonormality only.
    const float determinant=m[0]*(m[5]*m[10]-m[6]*m[9])+m[1]*(m[6]*m[8]-m[4]*m[10])+m[2]*(m[4]*m[9]-m[5]*m[8]);
    return determinant>0.f;
}
struct FakieMirror {
    bool fakie{};float mirror{1.f};double start{};float from{1.f};bool easing{};
};
inline dingosdk::first_person::Matrix aim_board_body_local(dingosdk::first_person::Matrix pose,
    const StickAim& aim,const dingosdk::first_person::Matrix& body,bool body_valid,
    float stance_blend,bool stance_valid,FakieMirror& stance,double frame) {
    // Stick look rotates in the skater body's own axes while the native
    // head-driven camera passes through untouched: the controls anchor to the
    // player as a whole instead of the animated head, and capture stays seamless.
    // The body matrix is a facing frame (Z row forward, X row left); the half
    // turn re-bases it to the camera's right/up/backward axes so positive
    // stick pitch looks up. Yaw shares the up axis and keeps its sense.
    // The skater state carries a stance-signed blend (positive regular,
    // negative fakie); in fakie the body's nose opposes the view, so the
    // pitch axis mirrors. Stance is tracked even while the aim is idle so
    // the mirror settles before the next capture, and while looking the
    // mirror eases over 200 ms so a stance flip swings through level
    // instead of snapping. An unreadable blend holds the last known stance.
    using namespace dingosdk::first_person;
    if(stance_valid) stance.fakie=stance_blend<0.f;
    const float target=stance.fakie ? -1.f:1.f;
    if(!aim.active || !body_valid) { stance.mirror=target;stance.easing=false;return pose; }
    if(!stance.easing && stance.mirror!=target) { stance.from=stance.mirror;stance.start=frame;stance.easing=true; }
    if(stance.easing) {
        const double t=std::clamp((frame-stance.start)/0.2,0.0,1.0);
        stance.mirror=stance.from+(target-stance.from)*static_cast<float>(t*t*(3.0-2.0*t));
        if(frame-stance.start>=0.2) { stance.mirror=target;stance.easing=false; }
    }
    const auto base=multiply(orientation(body),Quat{0,1,0,0});
    const float yaw=aim.heading*std::numbers::pi_v<float>/360.f;
    const float pitch=stance.mirror*std::clamp(aim.pitch,-85.f,85.f)*std::numbers::pi_v<float>/360.f;
    const auto look=multiply({0,std::sin(yaw),0,std::cos(yaw)},
                             {std::sin(pitch),0,0,std::cos(pitch)});
    write(pose,normalized(multiply(multiply(multiply(base,look),conjugate(base)),orientation(pose))),
        {pose[12],pose[13],pose[14]});
    return pose;
}
inline dingosdk::first_person::Matrix aim_camera(dingosdk::first_person::Matrix pose,const StickAim& aim) {
    if(!aim.active) return pose;
    using namespace dingosdk::first_person;
    // Absolute world-up yaw followed by pitch about the horizontal right axis.
    // Native head tilt must not turn horizontal stick movement into camera roll.
    const float yaw=wrap_heading(aim.heading)*std::numbers::pi_v<float>/360.f;
    const float pitch=std::clamp(aim.pitch,-85.f,85.f)*std::numbers::pi_v<float>/360.f;
    const auto rotation=multiply({0,std::sin(yaw),0,std::cos(yaw)},
                                 {std::sin(pitch),0,0,std::cos(pitch)});
    write(pose,rotation,{pose[12],pose[13],pose[14]});
    return pose;
}
inline dingosdk::first_person::Matrix face_heading(dingosdk::first_person::Matrix pose,float heading) {
    // Keep the native surface-up axis, target position and SIMD metadata.
    using namespace dingosdk::first_person;
    Vec3 up{pose[4],pose[5],pose[6]};const auto n=length(up);
    if(!std::isfinite(n) || n<.5f) return pose;
    for(auto& value:up) value/=n;
    const float radians=heading*std::numbers::pi_v<float>/180;
    Vec3 forward{std::sin(radians),0,std::cos(radians)};
    const float dot=forward[0]*up[0]+forward[1]*up[1]+forward[2]*up[2];
    for(unsigned i=0;i<3;++i) forward[i]-=dot*up[i];
    const auto f=length(forward);if(f<.01f) return pose;
    for(auto& value:forward) value/=f;
    const Vec3 right{up[1]*forward[2]-up[2]*forward[1],up[2]*forward[0]-up[0]*forward[2],up[0]*forward[1]-up[1]*forward[0]};
    for(unsigned i=0;i<3;++i) { pose[i]=right[i];pose[4+i]=up[i];pose[8+i]=forward[i]; }
    return pose;
}
}
