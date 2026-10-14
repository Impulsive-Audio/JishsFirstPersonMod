#pragma once
#include "mod.h"
#include <cstddef>
namespace jishsfirstpersonmod {
extern thread_local bool raw_controller_read;
struct RecordingInputAccess {
    bool previous{raw_controller_read};
    RecordingInputAccess() { raw_controller_read=true; }
    ~RecordingInputAccess() { raw_controller_read=previous; }
};
// ReSkate 2.0.x adds keyboard state after the legacy controller prefix.
// Never pass the smaller mod structure to its native reader.
struct NativeRecordingInput {
    bool available{};
    std::uint32_t buttons{};
    unsigned device{};
    dingosdk::ControllerStyle style{};
    std::array<std::uint64_t,4> keys{};
};
static_assert(sizeof(NativeRecordingInput)==48);
static_assert(offsetof(NativeRecordingInput,keys)==16);
using NativeRecordingReader=void (*)(NativeRecordingInput*,bool);
inline dingosdk::ControllerInput read_native_recording(NativeRecordingReader reader) {
    RecordingInputAccess access;
    NativeRecordingInput native;
    reader(&native,true); // Same allow_menu flag as ReSkate's Binds page.
    return {native.available,native.buttons & dingosdk::controller_button_mask,native.device,native.style};
}
}
