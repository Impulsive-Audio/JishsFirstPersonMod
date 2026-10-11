#pragma once
#include "camera_policy.h"
#include "bail_view.h"
namespace jishsfirstpersonmod {
void record_bail_camera(const dingosdk::first_person::Matrix& native_pose,
    const dingosdk::first_person::Matrix& final_pose,const ComfortSnapshot& state,
    const CameraMovementSample& movement,const Config& config,const StickAim& aim,
    float pitch,float yaw,const BailViewInfo& view={},const HeadPose& head={},std::uint64_t settings_revision=0) noexcept;
void flush_bail_camera_trace() noexcept;
}
