#include "first_person_hair.h"
#include "first_person_hair_contract.h"
#include "hair_network_filter.h"
#include "hair_network_contract.h"
#include "owned_hooks.h"
#include "Extension/Multiplayer/Remote/native_cosmetics_layout.h"
#include "Engine/Core/Platform/memory.h"
#include "comfort_policy.h"
#include "game_camera_ownership.h"
#include "stock_bridge.h"
#include "settings.h"
#include "bridge_resolver.h"
#include <atomic>
namespace jishsfirstpersonmod {
namespace {
using namespace dingosdk::multiplayer;
using Copy=void(*)(std::uintptr_t,const NativeCosmeticRecipe*);
Copy copy_recipe{};std::uintptr_t game_base{},owner{},component_owner{};
std::atomic<bool> supported{};HairSwap hair;
std::mutex hair_mutex;
using Capture=void*(*)(void*,std::uintptr_t,const void*,void*);
using Read=bool(*)(std::uintptr_t,void*,std::size_t);
static_assert(sizeof(CosmeticSlot)==0x40 && sizeof(CosmeticRecipe)==0x38 && sizeof(Appearance)==0xe0);
static_assert(sizeof(std::optional<Appearance>)==0xe8);
Capture original_capture{};Read original_read{};
thread_local HairCaptureFilter* capture_filter{};
DWORD update_thread{};ULONGLONG retry_after{};std::string last_issue;
bool copy(std::uintptr_t component,const NativeCosmeticRecipe* recipe) noexcept {
    __try { copy_recipe(component,recipe);return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool outgoing_read(std::uintptr_t address,void* output,std::size_t size) {
    const bool ok=original_read(address,output,size);
    if(ok && capture_filter) capture_filter->filter(address,output,size);
    return ok;
}
struct CaptureScope {
    HairCaptureFilter* previous=capture_filter;
    explicit CaptureScope(HairCaptureFilter* filter) { capture_filter=filter; }
    ~CaptureScope() { capture_filter=previous; }
};
void* outgoing_capture(void* result,std::uintptr_t base,const void* frame,void* detail) {
    std::optional<HairCaptureFilter> filter;
    try {
        std::lock_guard lock(hair_mutex);
        std::uintptr_t entity{};
        if(hair.owns() && base==game_base) {
            if(!dingosdk::memory::read(reinterpret_cast<std::uintptr_t>(frame)+0x18,entity))
                throw std::runtime_error("Outgoing skater is unavailable.");
        }
        if(hair.owns() && base==game_base && entity==owner) {
            const CosmeticMemory memory{[](std::uintptr_t address,void* out,std::size_t size) { return dingosdk::memory::read_bytes(address,out,size); },game_base};
            if(memory.component(entity)==component_owner) {
                const auto items=memory.ptr(component_owner,0x158);
                const auto count=memory.count(items,sizeof(NativeCosmeticItem),max_cosmetic_slots);
                filter.emplace(*hair.original(),items,count);
            }
        }
    } catch(...) {
        // A failed snapshot must not publish temporary bald hair. The compiled
        // caller/destructor check this flag before touching the optional value.
        static std::atomic<ULONGLONG> last_log{};
        const auto now=GetTickCount64();auto previous=last_log.load();
        if(now-previous>10000 && last_log.compare_exchange_strong(previous,now))
            log("Multiplayer hairstyle capture waiting: original-hair snapshot unavailable.");
        static_cast<unsigned char*>(result)[hair_network_contract::optional_engaged]=0;
        return result;
    }
    CaptureScope scope(filter ? &*filter:nullptr);
    return original_capture(result,base,frame,detail);
}
template<std::size_t N> bool matches(std::uintptr_t module,std::uintptr_t rva,const std::array<unsigned char,N>& expected) {
    std::array<unsigned char,N> actual{};
    return dingosdk::memory::read_bytes(module+rva,actual.data(),actual.size()) && actual==expected;
}
bool install_hair_capture_filter() {
    using namespace dingosdk;
    using namespace hair_network_contract;
    const auto module=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(L"reskate.dll"));
    const bool v211=stock_contract::uses_v211_layout(stock_contract::selected);
    const bool verified=v211 ?
        (matches(module,v211_contract::capture_rva,v211_contract::capture_bytes) && matches(module,v211_contract::read_rva,v211_contract::read_bytes) &&
         matches(module,v211_contract::array_rva,v211_contract::array_bytes) && matches(module,v211_contract::caller_rva,v211_contract::caller_bytes)) :
        (matches(module,capture_rva,capture_bytes) && matches(module,read_rva,read_bytes) &&
         matches(module,array_rva,array_bytes) && matches(module,caller_rva,caller_bytes));
    if(!module || !verified) {
        log("Local-only bald hairstyle unavailable: ReSkate cosmetic capture contract differs.");return false;
    }
    void* read_forward{};void* capture_forward{};
    auto* read_target=reinterpret_cast<void*>(module+(v211 ? v211_contract::read_rva:read_rva));
    auto* capture_target=reinterpret_cast<void*>(module+(v211 ? v211_contract::capture_rva:capture_rva));
    auto status=owned_hook_prepare(read_target,reinterpret_cast<void*>(&outgoing_read),&read_forward);
    if(status==HookOk) status=owned_hook_prepare(capture_target,reinterpret_cast<void*>(&outgoing_capture),&capture_forward);
    if(status!=HookOk) { log("Local-only hair capture preparation failed: "+hook_failure_description(status));return false; }
    original_read=reinterpret_cast<Read>(read_forward);original_capture=reinterpret_cast<Capture>(capture_forward);
    status=hook_queue_enable(read_target);
    if(status==HookOk) status=hook_queue_enable(capture_target);
    if(status==HookOk) status=apply_owned_hooks();
    if(status!=HookOk) { log("Local-only hair capture installation failed: "+hook_failure_description(status));return false; }
    log("Local-only bald hairstyle: ReSkate outgoing cosmetic capture/read layouts verified; original scalp hair retained for multiplayer.");
    return true;
}
}
bool first_person_hair_available() noexcept { return supported.load(std::memory_order_acquire); }
void initialize_first_person_hair(std::uintptr_t base) noexcept {
    auto bytes=hair_contract::copy_bytes;
    if(!dingosdk::memory::read_bytes(base+hair_contract::copy_rva,bytes.data(),bytes.size()) || bytes!=hair_contract::copy_bytes) {
        log("First-person bald hairstyle unavailable: native cosmetic copy differs.");return;
    }
    game_base=base;copy_recipe=reinterpret_cast<Copy>(base+hair_contract::copy_rva);
    if(!install_hair_capture_filter()) return; // Never enable a swap that could be sent to peers.
    supported.store(true,std::memory_order_release);
    log("First-person bald hairstyle: component-only cosmetic copy verified; saved loadouts are not edited.");
}
void tick_first_person_hair() noexcept {
    if(!first_person_hair_available()) return;
    if(!update_thread) update_thread=GetCurrentThreadId();
    if(update_thread!=GetCurrentThreadId()) return;
    std::lock_guard lock(hair_mutex);
    const auto error=GetLastError();const auto now=GetTickCount64();
    if(now<retry_after) return;
    try {
        const auto state=comfort_snapshot();const auto config=menu_snapshot().config;
        bool menu{};DWORD pid{};const auto foreground=GetForegroundWindow();
        const bool focused=foreground && GetWindowThreadProcessId(foreground,&pid) && pid==GetCurrentProcessId();
        const bool wanted=compatibility::enabled() && config.bald_first_person && stock_first_person_active() && state.state_valid && !state.suppressed &&
            !game_camera_suspended() && focused && !(overlay_available(menu) && menu);
        if(!wanted && !hair.owns()) { SetLastError(error);return; }
        const CosmeticMemory memory{[](std::uintptr_t address,void* out,std::size_t size) { return dingosdk::memory::read_bytes(address,out,size); },game_base};
        // Restore the old live actor first. Never carry its hairstyle onto a
        // replacement entity/component; new actors load their saved outfit.
        if(owner && (owner!=state.entity || !wanted)) {
            std::uintptr_t actor_type{};
            const auto previous=dingosdk::memory::read(owner,actor_type) && actor_type==game_base+dingosdk::addr::engine::skater_entity_vtable ? memory.component(owner):0;
            if(previous!=component_owner) { hair.abandon();owner=component_owner=0; }
            else {
                auto current=memory.capture(owner);
                const auto apply=[&](const CosmeticRecipe& recipe) {
                    const auto resource=memory.resource(previous);memory.validate(resource,recipe);
                    BorrowedCosmeticRecipe borrowed(recipe);
                    NativeCosmeticRecipe existing{};
                    if(!dingosdk::memory::read(previous+0x150,existing)) return false;
                    const auto mask=existing.mask ? memory.array<std::uint32_t>(reinterpret_cast<std::uintptr_t>(existing.mask),max_cosmetic_slots):std::vector<std::uint32_t>{};
                    borrowed.mask=CosmeticArray<std::uint32_t>(mask.size());std::copy(mask.begin(),mask.end(),borrowed.mask.data());
                    borrowed.value.mask=borrowed.mask.data();borrowed.value.word=existing.word;borrowed.value.flag=existing.flag;
                    return copy(previous,&borrowed.value);
                };
                if(!hair.update(std::move(current),false,apply)) throw std::runtime_error("Original hairstyle restore is waiting.");
                owner=component_owner=0;
                log("First-person hairstyle: original scalp hair restored.");
            }
        }
        if(wanted && state.entity) {
            const auto component=memory.component(state.entity);
            if(owner && component_owner!=component) { hair.abandon();owner=component_owner=0; }
            if(hair.owns()) { SetLastError(error);return; }
            auto current=memory.capture(state.entity);
            const auto resource=memory.resource(component);
            const auto apply=[&](const CosmeticRecipe& recipe) {
                memory.validate(resource,recipe); // Includes bald asset/category availability.
                BorrowedCosmeticRecipe borrowed(recipe);NativeCosmeticRecipe existing{};
                if(!dingosdk::memory::read(component+0x150,existing)) return false;
                const auto mask=existing.mask ? memory.array<std::uint32_t>(reinterpret_cast<std::uintptr_t>(existing.mask),max_cosmetic_slots):std::vector<std::uint32_t>{};
                borrowed.mask=CosmeticArray<std::uint32_t>(mask.size());std::copy(mask.begin(),mask.end(),borrowed.mask.data());
                borrowed.value.mask=borrowed.mask.data();borrowed.value.word=existing.word;borrowed.value.flag=existing.flag;
                return copy(component,&borrowed.value);
            };
            if(!hair.update(std::move(current),true,apply)) throw std::runtime_error("Bald hairstyle update is waiting.");
            if(hair.owns()) { owner=state.entity;component_owner=component;log("First-person hairstyle: temporary smooth bald hair applied; colours unchanged."); }
        }
        last_issue.clear();
    } catch(const std::exception& e) {
        // Dirty appearance flags during asynchronous mesh changes are normal;
        // retain the original hair and retry through the existing client tick.
        retry_after=now+100;
        if(last_issue!=e.what()) { last_issue=e.what();log("First-person hairstyle waiting: "+last_issue); }
    } catch(...) { retry_after=now+100;log("First-person hairstyle update failed; original hair retained for restoration."); }
    SetLastError(error);
}
}
