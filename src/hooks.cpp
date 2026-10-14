#include "native.h"
#include "mod.h"
#include "stock_bridge.h"
#include "comfort_policy.h"
#include "game_camera_ownership.h"
#include "stick_runtime.h"
#include "owned_hooks.h"
#include "bridge_resolver.h"
#include "native_camera_interop.h"
#include "bail_collision.h"
#include "Engine/Game/Build/20260929/runtime.h"
namespace {
using Tick=void(*)(std::uintptr_t,std::uintptr_t);
Tick original_tick{};
std::atomic<bool> active{};
std::uintptr_t stock_handler(std::uintptr_t address) {
    const auto stock=GetModuleHandleW(L"reskate.dll");
    if(!stock) return 0;
    for(unsigned i=0;i<8;++i) {
        HMODULE owner{};
        if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(address),&owner) && owner==stock) return address;
        unsigned char bytes[6]{};
        if(!dingosdk::memory::read_bytes(address,bytes,sizeof(bytes))) return 0;
        std::int32_t displacement{};
        if(bytes[0]==0xe9) {
            std::memcpy(&displacement,bytes+1,4);
            address=static_cast<std::uintptr_t>(static_cast<std::int64_t>(address+5)+displacement);
        } else if(bytes[0]==0xff && bytes[1]==0x25) {
            std::memcpy(&displacement,bytes+2,4);
            const auto slot=static_cast<std::uintptr_t>(static_cast<std::int64_t>(address+6)+displacement);
            if(!dingosdk::memory::read(slot,address)) return 0;
        } else return 0;
    }
    return 0;
}
void tick_hook(std::uintptr_t client,std::uintptr_t update) {
    const auto error=GetLastError();
    jishsfirstpersonmod::observe_collision_engine_thread();
    if(active.load(std::memory_order_acquire) && jishsfirstpersonmod::compatibility::enabled()) jishsfirstpersonmod::tick_mod(client);
    SetLastError(error);
    original_tick(client,update);
    const auto outgoing_error=GetLastError();
    SetLastError(outgoing_error);
}
}
namespace jishsfirstpersonmod {
std::uintptr_t resolve_stock_handler(std::uintptr_t address) { return stock_handler(address); }
bool install_hooks(std::uintptr_t base) {
    using namespace dingosdk;
    std::uintptr_t handler{};
    for(unsigned tries=0;tries<6000 && !handler;++tries) {
        handler=stock_handler(base+addr::runtime::client_tick);
        if(!handler) Sleep(100);
    }
    if(!handler || !stock_bridge_ready()) { log("ERROR: native input bridge unavailable; stock camera left unchanged.");return false; }
    void* forwarding{};
    auto* target=reinterpret_cast<void*>(handler);
    auto result=owned_hook_prepare(target,reinterpret_cast<void*>(&tick_hook),&forwarding);
    if(result==HookOk) {
        original_tick=reinterpret_cast<Tick>(forwarding);
        result=hook_queue_enable(target);
        if(result==HookOk) result=apply_owned_hooks();
    }
    if(result!=HookOk) { log("ERROR: controller tick hook failed: "+hook_failure_description(result));return false; }
    active.store(true,std::memory_order_release);
    initialize_bail_collision(base);
    log("Controller input tick installed; thread retries="+std::to_string(owned_hook_retries)+". Native ReSkate camera and pose lifecycle remain in control.");
    install_comfort_observer(base);
    install_game_camera_observer(base);
    if(!install_native_camera_interop(GetModuleHandleW(L"reskate.dll"))) return false;
    if(!install_stick_runtime(base)) { log("ERROR: on-foot stick look installation failed.");return false; }
    if(!install_stock_camera_feed(base,GetModuleHandleW(L"reskate.dll")))
        { log("ERROR: look/FOV input feed failed; rolling back mod hooks.");return false; }
    return true;
}
}
