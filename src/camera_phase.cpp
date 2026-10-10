#include "native.h"
#include "mod.h"
#include "Engine/Game/Build/20260929/camera_observer.h"
#include <intrin.h>

namespace dingosdk {
namespace {
namespace camera_addresses = addr::camera_observer;
#include "proven_camera_identity.inc"
struct PhaseState {
    std::uintptr_t base{};
    std::atomic<std::uint64_t> generation{};
    std::atomic<unsigned> ticks{};
};
PhaseState phase;
struct Sample {
    CameraIdentity before;
    std::uint64_t generation{}, time{};
    bool valid{}, in_tick{}, post{};
};
thread_local Sample sample;
}
}

// Called by an assembly tail-forwarder. The stock observer must see the native
// caller's ORIGINAL return address; a C++ call-wrapper would invalidate its probe.
extern "C" void JishsFirstPersonModCameraBefore(std::uintptr_t callback, std::uintptr_t input,
    std::uintptr_t, std::uintptr_t caller) noexcept {
    using namespace dingosdk;
    const auto error = GetLastError();
    try {
        const auto generation = ++phase.generation;
        if (caller != phase.base + camera_addresses::native_caller_return ||
            sample.in_tick || phase.ticks.load()) { sample.valid = false; SetLastError(error); return; }
        std::uintptr_t evaluator{};
        if (!memory::peek(input + 8,evaluator)) { sample.valid = false; SetLastError(error); return; }
        const auto before = camera_identity(phase.base,callback,evaluator);
        sample.before = before; sample.generation = generation; sample.time = GetTickCount64();
        // As in stock's observer, readiness requires the AFTER snapshot to be
        // idle; the native callback may still hold scratch state on entry.
        sample.valid = before.available;
    } catch (...) { sample.valid = false; }
    SetLastError(error);
}

namespace jishsfirstpersonmod {
void set_camera_phase_base(std::uintptr_t base) { dingosdk::phase.base = base; }
void camera_tick_begin() noexcept {
    auto& s = dingosdk::sample;
    if (s.in_tick || dingosdk::phase.ticks.fetch_add(1) != 0) { s.valid = false; ++dingosdk::phase.generation; }
    s.in_tick = true; s.post = false;
}
void camera_tick_end() noexcept {
    dingosdk::sample.post = false; dingosdk::sample.in_tick = false;
    dingosdk::phase.ticks.fetch_sub(1);
}
void camera_native_return() noexcept { dingosdk::sample.post = true; }
bool camera_ready(std::uintptr_t context) noexcept {
    using namespace dingosdk;
    auto& s = sample;
    if (!s.valid || !s.in_tick || !s.post || phase.ticks.load() != 1 ||
        s.generation != phase.generation.load() || GetTickCount64() - s.time > 500) return false;
    const auto after = camera_identity(phase.base,s.before.callback,s.before.evaluator);
    return camera_same_identity(s.before,after) && camera_idle_identity(after,context) &&
        s.generation == phase.generation.load();
}
}
