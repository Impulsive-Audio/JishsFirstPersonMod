#pragma once
#include <cstdint>
#include <array>
// Private type-only compatibility shim; no ReSkate UI implementation is linked.
namespace dingosdk::overlay {
struct DebugModel {
    std::uintptr_t skater_identity{};
    std::array<float,3> skater_position{};
    bool skater_position_valid{};
};
struct FlightInput {
    bool active{}, boost{};
    float right{}, up{}, forward{}, look_x{}, look_y{};
};
struct Status {
    bool started{}, bound{}, ready{}, visible{}, stopping{}, failed{};
    std::uint64_t rendered_frames{};
};
}
