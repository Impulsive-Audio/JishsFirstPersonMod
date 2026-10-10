#pragma once
#include "head_attachment.h"
#include <cstdint>
namespace jishsfirstpersonmod {
using CameraMatrix=dingosdk::first_person::Matrix;
struct BailViewInfo { int phase{};bool blocked{},attached{},calibrated{};dingosdk::first_person::Vec3 offset{};};
class BailView {
    using Quat=dingosdk::first_person::Quat;using Vec3=dingosdk::first_person::Vec3;
    CameraMatrix last_native_{};
    Quat displayed_{0,0,0,1},held_{0,0,0,1};Vec3 offset_{};
    bool ready_{},display_ready_{},calibrated_{};
    double time_{};
    int phase_{};BailViewInfo info_{};
    static Vec3 position(const CameraMatrix& pose) { return {pose[12],pose[13],pose[14]}; }
    static Quat level(Quat rotation) {
        using namespace dingosdk::first_person;
        const auto back=rotate(rotation,{0,0,1});
        const float yaw=std::atan2(back[0],back[2])/2;
        const float pitch=std::clamp(std::asin(std::clamp(-back[1],-1.f,1.f)),
            -85*std::numbers::pi_v<float>/180,85*std::numbers::pi_v<float>/180)/2;
        return multiply({0,std::sin(yaw),0,std::cos(yaw)},{std::sin(pitch),0,0,std::cos(pitch)});
    }
public:
    void reset() { *this={}; }
    BailViewInfo info() const { return info_; }
    void remember(const CameraMatrix& displayed,const CameraMatrix& native,const HeadPose& head,bool normal) {
        using namespace dingosdk::first_person;
        displayed_=orientation(displayed);display_ready_=true;
        if(normal && !phase_ && head.valid) {
            const auto delta=subtract(position(native),position(head.head));
            if(length(delta)<=.4f) { offset_=rotate(conjugate(orientation(head.head)),delta);calibrated_=true; }
        }
        info_.calibrated=calibrated_;info_.offset=offset_;
    }
    CameraMatrix attach(CameraMatrix target,const HeadPose& head) {
        using namespace dingosdk::first_person;
        info_.attached=head.valid && calibrated_;
        if(info_.attached) {
            const auto where=add(position(head.head),rotate(orientation(head.head),offset_));
            write(target,orientation(target),where);
        }
        info_.calibrated=calibrated_;info_.offset=offset_;return target;
    }
    CameraMatrix apply(CameraMatrix native,const HeadPose& head,bool bail,bool /*normal*/,double now) {
        using namespace dingosdk::first_person;
        if(!std::isfinite(now)) { reset();return native; }
        const bool discontinuity=ready_ && (now<time_ || now-time_>.25 || length(subtract(position(native),position(last_native_)))>5);
        if(discontinuity) reset();
        ready_=true;time_=now;last_native_=native;
        if(bail && !phase_) { held_=level(display_ready_ ? displayed_:orientation(native));phase_=1; }
        // No settle timer, angular-rate gate or release blend after bail clears.
        if(!bail) phase_=0;
        if(phase_) {
            const auto output=held_;
            auto where=position(native);
            const bool attached=head.valid && calibrated_;
            if(attached) where=add(position(head.head),rotate(orientation(head.head),offset_));
            write(native,output,where);displayed_=output;display_ready_=true;
            info_={1,true,attached,calibrated_,offset_};
        } else info_={0,false,false,calibrated_,offset_};
        return native;
    }
};
}
