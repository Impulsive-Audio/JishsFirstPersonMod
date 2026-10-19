#include "settings.h"
#include "menu.h"
#include "native_recording.h"
#include "stock_contract.h"
#include "Engine/Core/Platform/memory.h"
#include "Extension/UI/Overlay/playstation_input.h"
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Gaming.Input.h>
#include <algorithm>
#include <vector>
#include <roapi.h>

namespace jishsfirstpersonmod {
namespace {
NativeRecordingReader native_reader() {
    using namespace stock_contract;
    if(!selected) return nullptr;
    std::uintptr_t rva{};
    // Signature checked before hooking: the reader is a plain function on the
    // stock Binds page, and 2.1.1 rebuilt it at a different address with a
    // different body.
    const char* prefix_hex=nullptr;
    const std::string_view hash(selected->sha256);
    if(hash==contracts[4].sha256) rva=0x3e55a0;
    else if(hash==contracts[5].sha256) rva=0x3e5f40;
    else if(hash==contracts[6].sha256) rva=0x3eb480;
    else if(hash==contracts[7].sha256) rva=0x3c6c80;
    else if(hash==contracts[8].sha256 || hash==contracts[9].sha256) rva=0x3d00f0;
    else if(std::string_view(selected->version)=="2.0.8") rva=0xc1e1e0;
    else if(std::string_view(selected->version)=="2.0.7") rva=0x3f87e0;
    else if(std::string_view(selected->version)=="2.1.1") { rva=0xdea9e0;prefix_hex="4885c90f84520200004c8bdc5541564883ec78488b05c6bd98004833c4488944"; }
    else if(uses_v206_layout(selected)) rva=0x3e8940;
    else return nullptr;
    if(!prefix_hex) prefix_hex="4885c90f84c8010000488bc45641564883ec78";
    const auto module=GetModuleHandleW(L"reskate.dll");
    if(!module) return nullptr;
    const auto address=reinterpret_cast<std::uintptr_t>(module)+rva;
    const std::string_view prefix(prefix_hex);
    if(prefix.size()%2) return nullptr;
    std::array<unsigned char,64> actual{};
    const auto length=prefix.size()/2;
    if(length>actual.size() || !dingosdk::memory::read_bytes(address,actual.data(),length)) return nullptr;
    auto nibble=[](char c) { return c<='9' ? c-'0':c-'a'+10; };
    for(std::size_t i=0;i<length;++i) if(actual[i]!=(nibble(prefix[2*i])*16+nibble(prefix[2*i+1]))) return nullptr;
    log("Recording uses ReSkate's native Binds-page controller reader.");
    return reinterpret_cast<NativeRecordingReader>(address);
}
}
// Independent Windows snapshots remain readable when stock's XInput hooks
// intentionally return neutral gameplay input to the game behind its menu.
bool initialize_panel_recording() { return true; }
dingosdk::ControllerInput recording_controller() {
    RecordingInputAccess access;
    static const auto reader=native_reader();
    if(reader) return read_native_recording(reader);
    using namespace winrt::Windows::Gaming::Input;
    using dingosdk::ControllerInput;
    ControllerInput result;
    const auto window=GetForegroundWindow(); DWORD pid{};
    if(!window || !GetWindowThreadProcessId(window,&pid) || pid!=GetCurrentProcessId()) return result;
    try {
        static thread_local bool initialized=[] {
            const auto hr=RoInitialize(RO_INIT_MULTITHREADED);
            return SUCCEEDED(hr) || hr==RPC_E_CHANGED_MODE;
        }();
        if(initialized) {
            static thread_local std::vector<std::uintptr_t> previous;
            static thread_local unsigned generation=1;
            std::vector<std::uintptr_t> connected;
            const auto pads=Gamepad::Gamepads();
            for(const auto& pad:pads) {
                const auto reading=pad.GetCurrentReading();
                connected.push_back(reinterpret_cast<std::uintptr_t>(winrt::get_abi(pad)));
                const auto held=static_cast<unsigned>(reading.Buttons);
                const auto has=[&](GamepadButtons bit) { return (held&static_cast<unsigned>(bit))!=0; };
                const struct { GamepadButtons source; unsigned target; } mapping[]{
                    {GamepadButtons::DPadUp,1},{GamepadButtons::DPadDown,2},{GamepadButtons::DPadLeft,4},{GamepadButtons::DPadRight,8},
                    {GamepadButtons::Menu,0x10},{GamepadButtons::View,0x20},{GamepadButtons::LeftThumbstick,0x40},{GamepadButtons::RightThumbstick,0x80},
                    {GamepadButtons::LeftShoulder,0x100},{GamepadButtons::RightShoulder,0x200},
                    {GamepadButtons::A,0x1000},{GamepadButtons::B,0x2000},{GamepadButtons::X,0x4000},{GamepadButtons::Y,0x8000}};
                for(const auto& button:mapping) if(has(button.source)) result.buttons|=button.target;
                if(reading.LeftTrigger>30.0/255.0) result.buttons|=0x10000;
                if(reading.RightTrigger>30.0/255.0) result.buttons|=0x20000;
            }
            std::sort(connected.begin(),connected.end());
            if(connected!=previous) { previous=connected; ++generation; }
            if(!connected.empty()) { result.available=true; result.device=0x20000000u|(generation&0xfffffff); }
        }
    } catch(const winrt::hresult_error&) { /* Sony HID remains available below. */ }
    const auto sony=dingosdk::overlay::detail::read_playstation_pads();
    result.style=sony.style;
    if(result.available) return result; // Avoid doubling a physical/virtual pair.
    if(!sony.available) return {};
    result.available=true; result.device=0x10000000u|(sony.generation&0xfffffff);
    result.buttons=sony.pad.buttons;
    if(sony.pad.left_trigger>30) result.buttons|=0x10000;
    if(sony.pad.right_trigger>30) result.buttons|=0x20000;
    return result;
}
GameplayInput menu_controller() {
    RecordingInputAccess access;
    auto input=read_controller(true);
    const auto buttons=recording_controller();
    static_cast<dingosdk::ControllerInput&>(input)=buttons;
    try {
        using namespace winrt::Windows::Gaming::Input;
        static thread_local const auto apartment=RoInitialize(RO_INIT_MULTITHREADED);(void)apartment;
        for(const auto& pad:Gamepad::Gamepads()) {
            const auto reading=pad.GetCurrentReading();
            if(std::hypot(reading.LeftThumbstickX,reading.LeftThumbstickY)>std::hypot(input.left_x,input.left_y)) {
                input.left_x=static_cast<float>(reading.LeftThumbstickX);input.left_y=static_cast<float>(reading.LeftThumbstickY);
            }
        }
    } catch(...) {}
    return input;
}

}
