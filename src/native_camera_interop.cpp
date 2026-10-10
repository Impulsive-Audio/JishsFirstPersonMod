#include "native_camera_interop.h"
#include "stock_contract.h"
#include "stock_bridge.h"
#include "bridge_resolver.h"
#include "game_camera_ownership.h"
#include "camera_policy.h"
#include "settings.h"
#include "owned_hooks.h"
#include "Engine/Core/Platform/memory.h"
#include <array>
#include <string_view>
namespace jishsfirstpersonmod {
namespace {
using Matrix=dingosdk::first_person::Matrix;
using Vec3=dingosdk::first_person::Vec3;
// MSVC x64 ABI for the free function first_person::update returning a
// 64-byte Matrix: RCX=return storage, RDX=Spring, R8=head, R9=Settings,
// [entry RSP+40]=time, [entry RSP+48]=origin. Verified against the release
// DLL's render, FrameSpring::sample, and activation call sites.
using Sample=Matrix*(*)(Matrix*,void*,const Matrix*,const NativeFirstPersonSettings*,double,const Vec3*);
Sample original_sample{};
Matrix* sample_hook(Matrix* output,void* spring,const Matrix* head,
                    const NativeFirstPersonSettings* saved,double now,const Vec3* origin) {
    const auto error=GetLastError();
    bool locked{};
    NativeFirstPersonSettings settings{};
    if(compatibility::enabled() && !game_camera_suspended() && stock_first_person_requested() && saved) {
        const auto config=menu_snapshot().config;
        const auto owner=comfort_snapshot();const auto movement=live_camera_movement();
        locked=owner.state_valid && movement.mode && head_position_attached(config,owner,movement);
        if(locked) locked=dingosdk::memory::read(reinterpret_cast<std::uintptr_t>(saved),settings);
    }
    if(locked) settings=effective_native_sample(settings,true);
    SetLastError(error);
    return original_sample(output,spring,head,locked ? &settings:saved,now,origin);
}
}
bool install_native_camera_interop(void* module) {
    using namespace dingosdk;
    if(!stock_contract::is_v2()) return true;
    const std::string_view version(stock_contract::contract().version);
    if(version!="2.0.0" && version!="2.0.1" && version!="2.0.2" && version!="2.0.3") return false;
    // The exact-hash reviewed 2.0.x DLLs use this sampler. Refuse a modified
    // entry rather than patching an unknown function or another mod's detour.
    const auto sample_rva=stock_contract::native_sampler_rva();
    constexpr std::array<unsigned char,32> prefix{
        0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,
        0x41,0x57,0x48,0x8d,0xa8,0x68,0xfd,0xff,0xff,0x48,0x81,0xec,0x60,0x03,0x00,0x00};
    const auto target=reinterpret_cast<std::uintptr_t>(module)+sample_rva;
    std::array<unsigned char,32> live{};
    if(!memory::read(target,live) || live!=prefix) {
        log("ERROR: native first-person sampler differs; temporary camera coordination unavailable.");return false;
    }
    void* forwarding{};
    auto result=owned_hook_prepare(reinterpret_cast<void*>(target),reinterpret_cast<void*>(&sample_hook),&forwarding);
    if(result==HookOk) {
        original_sample=reinterpret_cast<Sample>(forwarding);
        result=hook_queue_enable(reinterpret_cast<void*>(target));
        if(result==HookOk) result=hook_apply_queued();
    }
    if(result!=HookOk) { log("ERROR: native sampler hook failed: "+std::to_string(result));return false; }
    log("Native camera coordination active: stabilization and Follow flips yield to mod head locks; saved preferences untouched.");
    return true;
}
}
