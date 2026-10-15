#pragma once
#include "Engine/Game/Skater/first_person_spring.h"
#include <algorithm>
#include <cmath>
namespace jishsfirstpersonmod {
struct StickSettings {
    bool enabled{},invert_y{};
    float yaw_speed=120,pitch_speed=90,deadzone=.15f,follow_tolerance=35;
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
struct StickAim { bool capture{},active{}; float heading{},pitch{},body_heading{};bool manual{};};
class StickLookState {
    bool ready_{},seen_{},center_facing_{};
    unsigned device_{};
    double time_{};
    float yaw_{},pitch_{},smooth_yaw_{},smooth_pitch_{},yaw_velocity_{},pitch_velocity_{},body_bias_{},center_offset_{};
    StickAxes prev_right_{},prev_move_{};
    bool move_authority_{},align_request_{};
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
        // The newest active input steers the follow servo. Look authority runs
        // the plain tolerance; move authority aligns the character with the
        // view on straight-forward walking and drags the view toward movement
        // that leaves the tolerance. Held sticks keep their authority until
        // the other stick delivers new input.
        const auto move_stick=radial_stick(move,settings.deadzone);
        const bool right_was=prev_right_.x!=0 || prev_right_.y!=0;
        // Authority: the newest stick to change its input steers the follow;
        // steady holds never re-claim, so analog noise cannot flap control.
        if(std::hypot(input.x-prev_right_.x,input.y-prev_right_.y)>.15f) move_authority_=false;
        if(std::hypot(move_stick.x-prev_move_.x,move_stick.y-prev_move_.y)>.15f) move_authority_=true;
        prev_right_=input;prev_move_=move_stick;
        const bool moving=move_stick.x!=0 || move_stick.y!=0;
        const float stick_angle=moving ? -std::atan2(move_stick.x,move_stick.y)*(180.f/std::numbers::pi_v<float>):0.f;
        // Releasing the look stick while moving hands control to the movement
        // with a full alignment request; a new look input cancels it.
        if(right_was && !input.x && !input.y && moving) align_request_=true;
        if(input.x || input.y) align_request_=false;
        float limit=settings.follow_tolerance;
        bool engaged{};
        if(align_request_ && moving) {
            limit=0.f;engaged=true;
        } else if(move_authority_ && moving) {
            if(std::abs(wrap_heading(stick_angle))<30.f) { limit=0.f;engaged=true; }
            else {
                const float move_vs_view=wrap_heading(body_heading+stick_angle-yaw_-180.f);
                const float excess=std::abs(move_vs_view)-limit;
                if(excess>0.f) {
                    engaged=true;
                    // Proportional drag: the rate ramps with the excess so the
                    // engage is smooth instead of toggling at the tolerance edge.
                    const float rate=settings.yaw_speed*std::min(1.f,.25f+excess/45.f);
                    const float drag=wrap_heading(body_heading+stick_angle-180.f-yaw_);
                    yaw_=wrap_heading(yaw_+std::clamp(drag,-rate*dt,rate*dt));
                }
            }
        }
        if(!spring.enabled) { smooth_yaw_=yaw_;smooth_pitch_=pitch_;yaw_velocity_=pitch_velocity_=0; }
        else {
            float yaw_error=wrap_heading(smooth_yaw_-yaw_),pitch_error=smooth_pitch_-pitch_;
            dingosdk::first_person::damp(yaw_error,yaw_velocity_,yaw_error>0 ? spring.right:spring.left,dt);
            dingosdk::first_person::damp(pitch_error,pitch_velocity_,pitch_error>0 ? spring.down:spring.up,dt);
            smooth_yaw_=wrap_heading(yaw_+yaw_error);smooth_pitch_=pitch_+pitch_error;
        }
        // The follow servo keeps the character's facing within the tolerance of
        // the view direction; the native turn animation provides the rate. Zero
        // deviation keeps the current facing, past it the body target rides the
        // tolerance boundary until the look re-centers or the tolerance expands.
        const float deviation=wrap_heading(body_heading-smooth_yaw_-180.f);
        if(align_request_ && moving && std::abs(deviation)<3.f) align_request_=false;
        body_bias_=wrap_heading(180.f+std::clamp(deviation,-limit,limit));
        return {true,true,smooth_yaw_,smooth_pitch_,wrap_heading(smooth_yaw_+body_bias_),input.x!=0 || input.y!=0 || engaged};
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
