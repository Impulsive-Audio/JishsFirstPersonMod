#include "Engine/Core/Hooks/hooks.h"
#include <intrin.h>
#include <cstdint>
using Target=std::uintptr_t(*)(std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t);
static Target original{};
extern "C" __declspec(dllexport) __declspec(noinline) std::uintptr_t StockHandler(
    std::uintptr_t a,std::uintptr_t b,std::uintptr_t c,std::uintptr_t d) {
    const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if(original(a,b,c,d)!=a+b+c+d) return 0;
    return caller;
}
extern "C" __declspec(dllexport) int InstallStock(void* target) {
    using namespace dingosdk;
    if(hook_initialize()!=HookOk) return 1;
    void* forwarding{};
    if(hook_prepare(target,reinterpret_cast<void*>(&StockHandler),&forwarding)!=HookOk) return 2;
    original=reinterpret_cast<Target>(forwarding);
    return hook_enable(target)==HookOk ? 0:3;
}
