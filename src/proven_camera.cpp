#include "native.h"
#include "client_source_spawn_internal.h"
#include "Engine/Game/Build/20260929/engine.h"

#include "free_flight.h"
#include <intrin.h>

namespace dingosdk::client_source::detail {
SourceCameraSnapshot source_camera_snapshot(const SourceTrial& trial, std::uintptr_t client) {
    // This smaller ownership/TLS snapshot deliberately does not require the
    // skater, physics, UI state or player binding to survive until restoration.
    SourceReader reader;
    const auto base = trial.base;
    source_require(source_object(client) && reader.pointer(client) == base + addr::engine::client_vtable,
                   "Free camera client identity rejected.");
    SourceCameraSnapshot result;
    auto& id = result.identity;
    id.context = reader.pointer(client, 8);
    const auto type_offset = reader.value<std::uint32_t>(base, addr::engine::context_type_offset);
    const auto client_offset = reader.value<std::uint32_t>(base, spawn::context_client_offset);
    const auto manager_offset = reader.value<std::uint32_t>(base, addr::engine::context_player_manager_offset);
    source_require(source_object(id.context) && type_offset >= 8 && type_offset <= 0x1000000 &&
        client_offset >= 8 && client_offset <= 0x1000000 && manager_offset <= 0x1000000,
        "Free camera context layout rejected.");
    const auto array = static_cast<std::uintptr_t>(__readgsqword(0x58));
    const auto index = reader.value<std::uint32_t>(base, addr::engine::tls_index);
    source_require(source_object(array) && index <= 4095, "Free camera TLS layout rejected.");
    const auto block = reader.pointer(array, std::uintptr_t{index} * 8);
    source_require(source_object(block) && reader.value<unsigned char>(block, 0xb19) &&
        reader.pointer(block, 0x550) == id.context &&
        reader.value<std::uint32_t>(id.context, type_offset) == 0xbf0f9789 &&
        reader.pointer(id.context, client_offset) == client, "Free camera native client scope mismatch.");
    source_require(!reader.value<std::uint32_t>(base, spawn::camera_local_id), "Native camera local ID is not zero.");
    id.manager = reader.pointer(id.context, manager_offset);
    source_require(source_object(id.manager) && reader.pointer(id.manager) == base + addr::engine::local_player_manager_vtable,
                   "Free camera client manager identity rejected.");
    id.controller = reader.pointer(id.manager, 0x710);
    source_require(source_object(id.controller) && reader.pointer(id.controller) == base + spawn::camera_controller_vtable &&
        !reader.value<std::uint32_t>(id.controller, 0x118), "Existing local camera controller unavailable.");
    result.mode = reader.value<std::uint32_t>(id.controller, 8);
    id.priority = reader.value<std::uint32_t>(id.controller, 0xd4);
    id.selector = reader.pointer(id.controller, 0xc0);
    id.camera = reader.pointer(id.controller, 0xc8);
    source_require(source_object(id.selector) && source_object(id.camera) &&
        reader.pointer(id.camera) == base + spawn::free_camera_vtable, "Existing FreeCamera identity rejected.");
    source_require(!reader.value<std::uint32_t>(id.selector) && !reader.value<std::uint32_t>(id.selector, 4),
                   "Existing camera selector local ID or kind changed.");
    const auto sentinel = id.selector + 8; // Native header/sentinel is inline, not its left-node pointer.
    (void)reader.pointer(id.selector, 8);
    const auto root = reader.pointer(id.selector, 0x18);
    const auto count = reader.value<std::uint32_t>(id.selector, 0x28);
    id.node = reader.pointer(id.selector, 0x38);
    source_require(source_object(sentinel) && source_object(root) && count && count <= 32 &&
        source_object(id.node) && id.node != sentinel && !reader.value<std::uint32_t>(id.node, 0x20) &&
        reader.pointer(id.node, 0x28) == id.camera, "Existing selector no longer selects the owned FreeCamera.");
    id.helpers = {reader.pointer(id.camera, 0x1d0), reader.pointer(id.camera, 0x1d8)};
    for (std::size_t i = 0; i < id.helpers.size(); ++i) {
        const auto helper = id.helpers[i];
        source_require(source_object(helper) && reader.pointer(helper) == base + spawn::free_camera_helper_vtables[i] &&
            source_object(reader.pointer(helper, 0x40)) && reader.value<unsigned char>(helper, 0x52) <= 1,
            "Existing free-camera input helper unavailable.");
    }
    const auto selected_helper = reader.pointer(id.camera, 0x1c8);
    source_require(selected_helper == id.helpers[0] || selected_helper == id.helpers[1],
                   "FreeCamera selected helper changed.");
    result.input_active = reader.value<unsigned char>(id.camera, 0x1c0);
    source_require(result.input_active <= 1, "FreeCamera activation flag rejected.");
    id.view_manager = reader.pointer(base, spawn::view_manager);
    source_require(source_object(id.view_manager) && reader.pointer(id.view_manager) == base + spawn::view_manager_vtable,
                   "Camera view manager identity rejected.");
    source_require(reader.pointer(base, spawn::view_manager_registration) == id.view_manager &&
        reader.pointer(base + spawn::view_manager_vtable) == base + spawn::view_manager_methods[0] &&
        reader.pointer(base + spawn::view_manager_vtable, 8) == base + spawn::view_manager_methods[1] &&
        reader.pointer(base + spawn::view_manager_vtable, 0x28) == base + spawn::view_manager_methods[2] &&
        reader.pointer(base + spawn::view_manager_vtable, 0x98) == base + spawn::view_manager_methods[3],
        "Native camera registration service changed.");
    const auto begin = reader.pointer(id.view_manager, 8), end = reader.pointer(id.view_manager, 0x10);
    const auto view_count = reader.count(begin, end, 8);
    source_require(view_count <= 32, "Camera view collection exceeds bound.");
    for (std::size_t i = 0; i < view_count; ++i) {
        const auto view = reader.pointer(begin, i * 8);
        source_require(source_object(view) && reader.pointer(view) == base + spawn::camera_view_vtable,
                       "Camera view type rejected.");
        if (!reader.value<std::uint32_t>(view, 0x148) && !reader.value<std::uint32_t>(view, 0x14c)) {
            source_require(!id.view, "Local camera view is ambiguous.");
            id.view = view;
        }
    }
    source_require(id.view != 0, "Local camera view unavailable.");
    result.view_enabled = reader.value<unsigned char>(id.view, 0x45d);
    source_require(result.view_enabled <= 1, "Local camera view activation flag rejected.");
    result.active = reader.pointer(id.view, 0x2b0);
    source_require(source_object(result.active), "Local view has no selected camera.");
    std::array<unsigned char, 24> prefix{};
    source_require(reader.pointer(base + spawn::camera_controller_vtable, 0xb8) == base + spawn::camera_mode_switch &&
        reader.raw(base + spawn::camera_mode_switch, prefix.data(), prefix.size()) && prefix == spawn::camera_mode_prefix,
        "Native camera mode-switch identity changed.");
    reader.verify();
    return result;
}
std::array<float, 16> debug_view_matrix(const SourceTrial& trial, const SourceCameraSnapshot& camera) {
    SourceReader reader;
    const auto vtable = reader.pointer(camera.active);
    source_require(vtable == trial.base + spawn::free_camera_vtable || vtable == trial.base + addr::engine::camera_vtable,
        "The current camera does not expose a supported view transform.");
    std::array<unsigned char, 24> prefix{};
    source_require(reader.pointer(trial.base + spawn::free_camera_vtable, 0xd0) == trial.base + spawn::camera_transform_setter &&
        reader.raw(trial.base + spawn::camera_transform_setter, prefix.data(), prefix.size()) && prefix == spawn::camera_transform_prefix,
        "Freecam transform setter changed.");
    std::array<float, 16> matrix{};
    for (std::size_t row = 0; row < 4; ++row) {
        const auto values = reader.value<std::array<float, 4>>(camera.active, 0x50 + row * 16);
        std::copy(values.begin(), values.end(), matrix.begin() + row * 4);
    }
    source_require(valid_flight_transform(matrix), "Current camera pose is unavailable.");
    reader.verify();
    return matrix;
}
}
