#pragma once
#include "Engine/Game/Skater/first_person_spring.h"
#include <cstddef>
#include <type_traits>
namespace jishsfirstpersonmod {
// ReSkate 2.0.x extends the legacy 44-byte settings prefix. Keep that ABI
// separate from the mod's saved settings and its proven 1.15 camera math.
struct NativeFirstPersonSettings {
    bool enabled{};
    dingosdk::first_person::Vec3 offset{},rotation{};
    float up{},down{},left{},right{};
    bool stabilize{},follow_flips{},board_only{};
    float smoothing{},head_pitch{},head_roll{},bob{};
};
static_assert(std::is_trivially_copyable_v<NativeFirstPersonSettings>);
static_assert(sizeof(NativeFirstPersonSettings)==64);
static_assert(offsetof(NativeFirstPersonSettings,stabilize)==44);
static_assert(offsetof(NativeFirstPersonSettings,follow_flips)==45);
static_assert(offsetof(NativeFirstPersonSettings,board_only)==46);
static_assert(offsetof(NativeFirstPersonSettings,smoothing)==48);
inline NativeFirstPersonSettings effective_native_sample(NativeFirstPersonSettings saved,bool head_locked) {
    if(head_locked) {
        // The mod owns attachment, bail hold and animation handoffs here.
        // No queue command, profile mutation, or shared settings write occurs.
        saved.enabled=false;
        saved.stabilize=false;
        saved.follow_flips=false;
    }
    return saved;
}
bool install_native_camera_interop(void* module);
}
