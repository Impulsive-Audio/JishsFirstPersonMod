#include "native.h"
#include "mod.h"
#include "Extension/Multiplayer/Remote/native_pose_layout.h"

namespace {
std::mutex hidden_mutex;
std::recursive_mutex visibility_gate;
std::uintptr_t hidden_component{}, hidden_buffer{};
std::array<float,3> real_scale{};
bool write_scale(std::uintptr_t address, const std::array<float,3>& value) noexcept {
    __try { std::memcpy(reinterpret_cast<void*>(address),value.data(),sizeof(value)); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
}
namespace jishsfirstpersonmod {
bool begin_head_hide() noexcept {
    visibility_gate.lock();
    if (native_tick_active()) { visibility_gate.unlock(); return false; }
    return true;
}
void end_head_hide() noexcept { visibility_gate.unlock(); }
void restore_hidden_head() noexcept {
    using namespace dingosdk::client_source::detail;
    try {
        std::lock_guard gate(visibility_gate);
        std::lock_guard lock(hidden_mutex);
        if (!hidden_component || !hidden_buffer) return;
        auto& arm = first_person_arm();
        std::uintptr_t holder{};
        if (!first_person_read(hidden_component+0xa0,&holder,8)) return;
        const auto pose = dingosdk::multiplayer::read_native_pose_layout(first_person_read,arm.watched_base.load(),holder,512);
        if (pose.count != 395 || pose.buffer != hidden_buffer) return;
        std::array<float,3> scale{};
        if (first_person_read(pose.buffer+103*0x30ULL,scale.data(),sizeof(scale)) &&
            scale[0] == .001f && scale[1] == .001f && scale[2] == .001f)
            (void)write_scale(pose.buffer+103*0x30ULL,real_scale);
    } catch (...) {}
}
void set_hidden_joint(std::uintptr_t component, std::uint16_t joint, const std::array<float,3>& scale) noexcept {
    using namespace dingosdk::client_source::detail;
    if (!component) {
        restore_hidden_head();
        std::lock_guard lock(hidden_mutex); hidden_component = hidden_buffer = 0; return;
    }
    try {
        auto& arm = first_person_arm();
        std::uintptr_t holder{};
        if (joint != 103 || !first_person_read(component+0xa0,&holder,8)) return;
        const auto pose = dingosdk::multiplayer::read_native_pose_layout(first_person_read,arm.watched_base.load(),holder,512);
        if (pose.count != 395 || !pose.buffer) return;
        std::lock_guard lock(hidden_mutex);
        hidden_component = component; hidden_buffer = pose.buffer; real_scale = scale;
    } catch (...) {}
}
}
