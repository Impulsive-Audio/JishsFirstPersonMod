#pragma once
#include "comfort_policy.h"
#include "Engine/Game/Skater/first_person_spring.h"
namespace jishsfirstpersonmod {
struct HeadPose { bool valid{};dingosdk::first_person::Matrix root{},head{}; };
HeadPose read_animation_head(const ComfortSnapshot& state) noexcept;
HeadPose read_animation_head_component(std::uintptr_t component,std::uintptr_t base) noexcept;
}
