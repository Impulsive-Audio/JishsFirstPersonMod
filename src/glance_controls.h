#pragma once
#include "Engine/Game/Input/controller_bindings.h"
#include <algorithm>
namespace jishsfirstpersonmod {
class GlanceControlsState {
    dingosdk::ControllerLookState held_;
    dingosdk::FirstPersonLookControls controls_{};
    std::array<bool,4> previous_{};
    unsigned device_{};
    bool initialized_{},toggle_{},armed_{};
    int selected_=-1;
    std::uint32_t emote_release_{};
public:
    void reset() { *this={}; }
    void centre_for_emote(std::uint32_t directions) {
        selected_=-1;held_={};previous_={};emote_release_=directions&15;
    }
    std::array<float,2> update(const dingosdk::FirstPersonLookControls& controls,
        dingosdk::ControllerInput input,bool inhibited,bool toggle) noexcept {
        bool changed=!initialized_ || toggle!=toggle_ || input.device!=device_;
        for(unsigned i=0;i<4;++i) changed=changed || controls[i].combo!=controls_[i].combo || controls[i].angle!=controls_[i].angle;
        if(changed) { reset();initialized_=true;controls_=controls;device_=input.device;toggle_=toggle; }
        if(emote_release_) {
            if(inhibited || !input.available) { reset();return {}; }
            if(input.buttons&emote_release_) return {};
            emote_release_=0;armed_=true;previous_={};
            auto neutral=input;neutral.buttons=0;held_.update(controls,neutral,false);
        }
        if(!toggle) return held_.update(controls,input,inhibited);
        if(inhibited || !input.available) { selected_=-1;armed_=false;previous_={};return {}; }
        std::uint32_t bound{};for(const auto& binding:controls) bound|=binding.combo;
        if(!armed_) {
            armed_=(input.buttons&bound)==0;previous_={};return {};
        }
        int pressed=-1;unsigned count{};
        for(unsigned i=0;i<4;++i) {
            const auto combo=controls[i].combo;
            const bool full=combo && dingosdk::valid_controller_combo(combo) && (input.buttons&combo)==combo;
            if(full && !previous_[i]) { pressed=int(i);++count; }
            previous_[i]=full;
        }
        if(count==1) selected_=selected_==pressed ? -1:pressed;
        // Simultaneous new directions are ambiguous; consume their edges without
        // replacing the currently selected direction. Partial-chord release rearms.
        if(selected_<0) return {};
        const float angle=controls[unsigned(selected_)].angle;
        if(selected_==0) return {angle,0};
        if(selected_==1) return {-angle,0};
        if(selected_==2) return {0,-angle};
        return {0,angle};
    }
};
}
