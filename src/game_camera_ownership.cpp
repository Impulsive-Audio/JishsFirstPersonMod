#include "game_camera_ownership.h"
#include "native.h"
#include "stock_bridge.h"
#include "stock_contract.h"
#include "stick_runtime.h"
#include "Engine/Game/Build/image_identity.h"
#include <atomic>
#include <mutex>
#include <string_view>

namespace jishsfirstpersonmod {
namespace {
// Source: stock 1.1.8 native_party.h and native_party_hooks.cpp. Its bool()
// hook includes ReSkate's remote-party spectator state, not just EA players.
constexpr std::uintptr_t spectator_active=0x7572a0;
constexpr std::array<unsigned char,32> spectator_prefix{
    0x48,0x83,0xec,0x28,0x48,0x8b,0x05,0xd5,0x74,0xa9,0x06,0x48,0x85,0xc0,0x74,0x4b,
    0x48,0x8b,0x48,0x08,0x48,0x85,0xc9,0x74,0x06,0x48,0x83,0x39,0x00,0x75,0x0e,0x48};
using SpectatorActive=bool(*)();
std::atomic<SpectatorActive> spectator_query{};
std::mutex ownership_mutex;
GameCameraOwnershipPolicy policy;
std::atomic<bool> suspended{},managing{};
bool query_spectator(bool& result) noexcept {
    __try { result=spectator_query.load()();return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
}
bool game_camera_suspended() { return suspended.load(); }
bool game_camera_managing() { return managing.load(); }
const char* game_camera_status() {
    return suspended.load() ? "First Person armed - game spectator camera owns the view.":
        !spectator_query.load() ? "Game camera observer unavailable; see the mod log.":"";
}
void acknowledge_game_camera(bool master,bool active) {
    std::lock_guard lock(ownership_mutex);policy.acknowledge(master,active);managing=policy.managing;
}
void install_game_camera_observer(std::uintptr_t base) {
    // Bootstrap checks the exact game hash. Only the reviewed stock version
    // is accepted here; automatic camera-bridge detection proves no party ABI.
    if((std::string_view(stock_contract::contract().version)!="1.1.8" && !stock_contract::is_v2()) ||
       !dingosdk::supported_build::running_image_matches(base)) {
        log("Game camera observer unavailable: spectator contract requires verified ReSkate 1.1.8, 2.0.0, 2.0.1 or 2.0.2.");return;
    }
    std::array<unsigned char,32> live{};
    const bool native=dingosdk::memory::read(base+spectator_active,live) && live==spectator_prefix;
    if(!native && !resolve_stock_handler(base+spectator_active)) {
        log("Game camera observer unavailable: spectator query has an unverified owner.");return;
    }
    // Call the entry, not a trampoline: the stock hook supplies party state.
    spectator_query=reinterpret_cast<SpectatorActive>(base+spectator_active);
    log("Game camera observer installed: verified native/stock spectator query. Replay pre-entry and scripted/game-mode camera contracts are unavailable.");
}
void update_game_camera_ownership(std::uintptr_t client,std::uintptr_t base) {
    if(!spectator_query.load()) return;
    GameCameraEvidence evidence;
    try {
        using namespace dingosdk::client_source::detail;
        SourceReader reader;
        source_require(reader.pointer(client)==base+dingosdk::addr::engine::client_vtable,"Camera ownership client changed.");
        evidence.context=reader.pointer(client,8);
        const auto world=reader.value<std::uint32_t>(client,0xc4);
        reader.verify();
        if(world!=13 && world!=21) return;
        evidence.valid=query_spectator(evidence.spectator);
        if(evidence.valid && !evidence.spectator && suspended.load()) {
            SourceTrial trial;trial.base=base;
            const auto camera=source_camera_snapshot(trial,client);
            std::uintptr_t type{};
            evidence.local_camera=camera.identity.context==evidence.context && camera.mode==0 && camera.view_enabled &&
                !camera.input_active && dingosdk::memory::read(camera.active,type) && type==base+dingosdk::addr::engine::camera_vtable;
        }
    } catch(...) { /* Missing evidence neither suspends nor restores. */ }
    bool changed{},active{};
    {
        std::lock_guard lock(ownership_mutex);
        const auto before=policy.reason;policy.observe(evidence);
        active=policy.suspended();changed=before!=policy.reason;
        suspended=active;managing=policy.managing;
    }
    if(changed) {
        suspend_custom_camera();
        clear_stock_camera_blends();
        log(active ? "Game camera ownership: spectator takeover; releasing First Person.":
            "Game camera ownership: spectator ended and local gameplay camera verified; existing restrictions still apply.");
    }
}
}
