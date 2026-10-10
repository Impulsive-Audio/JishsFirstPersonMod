#pragma once
#include "spring_policy.h"
#include "stick_runtime.h"
#include "player_state_contract.h"
namespace jishsfirstpersonmod {
inline bool bail_camera_locked(const ComfortSnapshot& state,const CameraMovementSample& movement) {
    return movement.bail_recovery || (state.state_valid && (state.bailing || state.recovery ||
        state.wipeout_request || state.animation_wipeout || state.physics==player_state_contract::wipeout));
}
inline bool action_camera_locked(const Config& config,const CameraMovementSample& movement) {
    return config.stick.enabled && ((config.stick.lock_mount_jump &&
        (movement.mounting || (movement.jumping && !movement.hippy.active && !movement.hippy.candidate))) ||
        (config.stick.lock_hippy_jump && movement.hippy.active));
}
inline bool native_camera_locked(const Config& config,const ComfortSnapshot& state,const CameraMovementSample& movement) {
    return movement.view_hold || bail_camera_locked(state,movement) || action_camera_locked(config,movement);
}
inline bool camera_overrides_allowed(const ComfortSnapshot& state,const CameraMovementSample& movement) {
    return movement.mode && movement.non_bail && state.state_valid && state.animation_valid &&
        !bail_camera_locked(state,movement) && !state.suppressed;
}
inline bool camera_look_allowed(const Config& config,const ComfortSnapshot& state,const CameraMovementSample& movement) {
    return camera_overrides_allowed(state,movement) && !(config.stick.enabled && movement.mounting) && (config.stick.look_during_parkour ||
        (!(movement.mode==1 && !movement.custom_mode) && !action_camera_locked(config,movement)));
}
inline bool character_facing_allowed(const ComfortSnapshot& state,const CameraMovementSample& movement) {
    return camera_overrides_allowed(state,movement) && movement.mode==1 && movement.custom_mode==1 &&
        !movement.jumping && !movement.mounting && !movement.hippy.active && !movement.hippy.candidate;
}
inline bool head_position_attached(const Config& config,const ComfortSnapshot& state,const CameraMovementSample& movement) {
    return bail_camera_locked(state,movement) || (movement.mode==1 && !movement.custom_mode) ||
        action_camera_locked(config,movement) || (config.disable_spring_on_foot && movement.mode==1);
}
inline Config effective_camera_config(Config config,bool active,const ComfortSnapshot& state,const CameraMovementSample& movement) {
    return camera_config_for_state(config,active,movement.mode,head_position_attached(config,state,movement));
}
}
