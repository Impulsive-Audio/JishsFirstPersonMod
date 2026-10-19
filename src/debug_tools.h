#pragma once
#include "Engine/Game/Skater/first_person_spring.h"
namespace jishsfirstpersonmod {
// Master switch. When off the mod stops requesting, driving and composing the
// first-person camera, so ReSkate's own camera owns the view completely.
bool mod_enabled() noexcept;
void set_mod_enabled(bool enabled) noexcept;
int take_mod_enable_request() noexcept; // -1 none, 0 disable, 1 enable
void request_mod_enable_toggle() noexcept;
namespace debug {
struct FrameFlags {
    int mode{},action{},custom_mode{},yield{},reason{},sub_offset{-1},sub_vtable{-1};
    bool eligible{},suspended{},requested{},active{},jumping{},mounting{};
    bool aim_capture{},aim_active{};
    float aim_yaw{},aim_pitch{};
};
void publish(const dingosdk::first_person::Matrix& native,const dingosdk::first_person::Matrix& final,
    const dingosdk::first_person::Matrix* shown,const FrameFlags& flags) noexcept;
void toggle_hud() noexcept;
void toggle_recording() noexcept;
bool hud_visible() noexcept;
bool recording() noexcept;
void tick() noexcept;
}
}
