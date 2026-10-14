#pragma once
#include "spectator_contract.h"
#include "body_flip.h"
namespace jishsfirstpersonmod::spectator {
using Matrix=dingosdk::first_person::Matrix;
class View {
    body_flip::Camera camera_;
    body_flip::Status state_;
    Identity identity_{};
    bool session_{},preferred_{},ready_{};
    std::uintptr_t context_{};
    double sampled_{};
    dingosdk::first_person::Vec3 position_{};
    float last_fov_{},from_fov_{};
public:
    void discard() { camera_.reset();state_={};identity_={};ready_=false;last_fov_=from_fov_=0; }
    void observe(bool watching,bool enabled,std::uintptr_t context,bool interrupted=false) {
        if(!watching || !enabled || !context || (session_ && context_!=context)) {
            session_=preferred_=false;discard();
        }
        if(watching && enabled && context) { session_=true;context_=context; }
        if(interrupted)discard();
    }
    bool toggle() { if(!session_)return false;preferred_=!preferred_;return true; }
    bool session() const { return session_; }
    bool preferred() const { return preferred_; }
    body_flip::Phase phase() const { return state_.phase; }
    bool owns_camera() const { return state_.owns_camera(); }
    const Identity& identity() const { return identity_; }
    Matrix apply(const Matrix& normal,const Matrix& head,const Identity& identity,double now) {
        using namespace dingosdk::first_person;
        const auto position=body_flip::position(head);
        if(!session_ || !std::isfinite(now)) { discard();return normal; }
        if(ready_ && (identity_!=identity || now<sampled_ || now-sampled_>body_flip::maximum_gap || length(subtract(position,position_))>5))discard();
        if(!ready_) { camera_.apply(normal,head,state_,now);ready_=true;identity_=identity; }
        const auto before=state_.phase;
        if(preferred_) {
            if(state_.phase==body_flip::Phase::idle || state_.phase==body_flip::Phase::leaving) { state_.phase=body_flip::Phase::entering;state_.started=now; }
            else if(state_.phase==body_flip::Phase::entering && now-state_.started+1e-9>=body_flip::handoff) state_.phase=body_flip::Phase::locked;
        } else {
            if(state_.owns_head()) { state_.phase=body_flip::Phase::leaving;state_.started=now; }
            else if(state_.phase==body_flip::Phase::leaving && now-state_.started+1e-9>=body_flip::handoff) state_.phase=body_flip::Phase::idle;
        }
        if(before!=state_.phase && (state_.phase==body_flip::Phase::entering || state_.phase==body_flip::Phase::leaving))from_fov_=last_fov_;
        sampled_=now;position_=position;
        return camera_.apply(normal,head,state_,now);
    }
    float fov(float normal,float configured,double now) {
        if(!std::isfinite(normal) || normal<=1 || normal>=175)return normal;
        const auto head=configured>=40 && configured<=120 ? configured:normal;
        if(!last_fov_)last_fov_=from_fov_=normal;
        auto output=normal;
        if(state_.phase==body_flip::Phase::locked)output=head;
        else if(state_.owns_camera()) {
            const float t=std::clamp(float((now-state_.started)/body_flip::handoff),0.f,1.f),w=t*t*(3-2*t);
            output=from_fov_+((state_.phase==body_flip::Phase::entering ? head:normal)-from_fov_)*w;
        }
        last_fov_=output;return output;
    }
};
}
