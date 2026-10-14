#pragma once
#include "mod.h"
#include "reskate_spectator_spring.h"
namespace jishsfirstpersonmod::spectator {
inline reskate_spectator::Settings look_settings(const Config& config) {
    reskate_spectator::Settings result;
    result.enabled=config.spring.enabled;result.offset=config.spring.offset;result.rotation=config.spring.rotation;
    result.up=config.spring.up;result.down=config.spring.down;result.left=config.spring.left;result.right=config.spring.right;
    if(config.native.active) {
        result.stabilize=config.native.stabilize;result.follow_flips=config.native.follow_flips;
        result.smoothing=config.native.smoothing;result.head_pitch=config.native.head_pitch;result.head_roll=config.native.head_roll;result.bob=config.native.bob;
    }
    return result;
}
}
