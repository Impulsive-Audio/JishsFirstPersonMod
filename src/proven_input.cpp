#include "mod.h"
#include "menu.h"
#include "stick_runtime.h"
#include "Engine/Core/Hooks/hooks.h"
#include "stock_bridge.h"
#include "Extension/UI/Overlay/playstation_input.h"
#include <Xinput.h>
#include <atomic>
#include <mutex>
#include <cstring>
namespace dingosdk::overlay::detail {
namespace {
struct ControllerSample {
    XINPUT_GAMEPAD pad{};
    unsigned device{};
    dingosdk::ControllerStyle style{};
};
using GetState = DWORD (WINAPI*)(DWORD, XINPUT_STATE*);
GetState xinput_get_state() {
    // Optional OS API, resolved outside DllMain. Keep the module for process
    // lifetime; a missing controller/runtime must not prevent keyboard flight.
    static const auto get_state = []() -> GetState {
        for (const auto name : {L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll"}) {
            const auto module = LoadLibraryExW(name, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            if (!module) continue;
            const auto entry = GetProcAddress(module, "XInputGetState");
            if (entry) {
                GetState fn{};
                static_assert(sizeof(fn) == sizeof(entry));
                std::memcpy(&fn, &entry, sizeof(fn));
                return fn;
            }
            FreeLibrary(module);
        }
        return nullptr;
    }();
    return get_state;
}
// The XInput slots with a pad in them, shared by every reader. Polling an empty
// slot is slow (it looks for a device), so readers poll only these, and a
// background thread retries the empty ones once a second while pads are read:
// the client tick, which reads bound combos, never waits for that.
struct XInputSlots {
    std::atomic<unsigned> connected{};
    std::atomic<ULONGLONG> next_probe{};
    std::atomic<HANDLE> wake{}; // set when a probe is due; the probe thread waits on it
    std::mutex start;           // starting the thread (tried again at the next probe if it fails)
};
XInputSlots& xinput_slots() { static auto* value = new XInputSlots; return *value; } // process lifetime
DWORD WINAPI probe_xinput_slots(void* parameter) noexcept {
    auto& slots = *static_cast<XInputSlots*>(parameter);
    const auto get_state = xinput_get_state();
    for (;;) {
        WaitForSingleObject(slots.wake.load(), INFINITE);
        /* Private reader: the stock capture hook still filters menu input. */
        for (DWORD slot = 0; get_state && slot < XUSER_MAX_COUNT; ++slot) {
            const unsigned bit = 1u << slot;
            if (slots.connected.load() & bit) continue;
            XINPUT_STATE sample{};
            if (get_state(slot, &sample) == ERROR_SUCCESS) slots.connected.fetch_or(bit);
        }
    }
}
// Wakes the probe thread (starting it the first time) at most once a second.
void request_xinput_probe(XInputSlots& slots) {
    const auto now = GetTickCount64();
    auto due = slots.next_probe.load();
    if (now < due || !slots.next_probe.compare_exchange_strong(due, now + 1000)) return;
    if (!slots.wake.load()) {
        std::lock_guard lock(slots.start);
        if (!slots.wake.load()) {
            if (const auto wake = CreateEventW(nullptr, FALSE, FALSE, nullptr)) {
                slots.wake.store(wake);
                if (const auto thread = CreateThread(nullptr, 0, probe_xinput_slots, &slots, 0, nullptr))
                    CloseHandle(thread);
                else {
                    slots.wake.store(nullptr);
                    CloseHandle(wake);
                }
            }
        }
    }
    if (const auto wake = slots.wake.load()) SetEvent(wake);
}
ControllerSample read_controller_sample() {
    /* Private reader: the stock capture hook still filters menu input. */ // Binding capture needs the real controller while the menu is open.
    const auto get_state = xinput_get_state();
    // Virtual pads (DS4Windows, Steam, ViGEm leftovers) can take any slot, and
    // an idle one may sit ahead of the pad in use, so merge every connected
    // slot. Empty slots are retried in the background (xinput_slots above).
    auto& slots = xinput_slots();
    if (get_state) request_xinput_probe(slots);
    const auto connected_slots = slots.connected.load();
    unsigned connected = 0;
    ControllerSample result;
    for (DWORD slot = 0; get_state && slot < XUSER_MAX_COUNT; ++slot) {
        const unsigned bit = 1u << slot;
        if (!(connected_slots & bit)) continue;
        XINPUT_STATE sample{};
        if (get_state(slot, &sample) != ERROR_SUCCESS) { slots.connected.fetch_and(~bit); continue; }
        connected |= bit;
        const auto& pad = sample.Gamepad;
        result.pad.wButtons |= pad.wButtons;
        result.pad.bLeftTrigger = std::max(result.pad.bLeftTrigger, pad.bLeftTrigger);
        result.pad.bRightTrigger = std::max(result.pad.bRightTrigger, pad.bRightTrigger);
        const auto magnitude = [](SHORT x, SHORT y) { return std::abs(int(x)) + std::abs(int(y)); };
        if (magnitude(pad.sThumbLX, pad.sThumbLY) > magnitude(result.pad.sThumbLX, result.pad.sThumbLY)) {
            result.pad.sThumbLX = pad.sThumbLX; result.pad.sThumbLY = pad.sThumbLY;
        }
        if (magnitude(pad.sThumbRX, pad.sThumbRY) > magnitude(result.pad.sThumbRX, result.pad.sThumbRY)) {
            result.pad.sThumbRX = pad.sThumbRX; result.pad.sThumbRY = pad.sThumbRY;
        }
    }
    // A PlayStation pad the tool left visible still names the buttons, even
    // while its virtual XInput twin supplies them.
    const auto playstation = read_playstation_pads();
    result.style = playstation.style;
    if (connected) {
        // XInput wins whenever a pad is there: reading both would double a
        // DS4Windows pad and mix its remapped buttons with the physical ones.
        result.device = connected;
        return result;
    }
    if (!playstation.available) return {};
    const auto& pad = playstation.pad;
    result.pad.wButtons = pad.buttons;
    result.pad.bLeftTrigger = pad.left_trigger; result.pad.bRightTrigger = pad.right_trigger;
    result.pad.sThumbLX = pad.left_x; result.pad.sThumbLY = pad.left_y;
    result.pad.sThumbRX = pad.right_x; result.pad.sThumbRY = pad.right_y;
    result.device = 0x100u + (playstation.generation & 0xffffffu);
    return result;
}

}
}
namespace jishsfirstpersonmod {
thread_local bool raw_controller_read{};
void prepare_input() {
    using namespace dingosdk::overlay::detail;
    (void)xinput_get_state();
    request_xinput_probe(xinput_slots());
    (void)read_playstation_pads();
}
GameplayInput read_controller(bool allow_menu) {
    using namespace dingosdk::overlay::detail;
    const auto window = GetForegroundWindow();
    DWORD pid{};
    if (!window || !GetWindowThreadProcessId(window,&pid) || pid != GetCurrentProcessId()) return {};
    bool visible{};
    if (!overlay_available(visible) || (!allow_menu && (visible || panel_gameplay_blocked()))) return {};
    struct RawRead { bool previous{raw_controller_read};RawRead(){raw_controller_read=true;} ~RawRead(){raw_controller_read=previous;} } raw;
    const auto sample = read_controller_sample();
    GameplayInput out{};
    out.style = sample.style;
    if (!sample.device) return out;
    out.available = true; out.device = sample.device;
    out.right_x=normalize_axis(sample.pad.sThumbRX);out.right_y=normalize_axis(sample.pad.sThumbRY);
    out.left_x=normalize_axis(sample.pad.sThumbLX);out.left_y=normalize_axis(sample.pad.sThumbLY);
    out.buttons = sample.pad.wButtons & dingosdk::controller_button_mask;
    if (sample.pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) out.buttons |= 0x10000;
    if (sample.pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) out.buttons |= 0x20000;
    return out;
}
}
