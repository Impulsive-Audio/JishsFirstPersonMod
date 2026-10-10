#include "Engine/Core/Hooks/hooks.h"
#include <iostream>
#include <cstdint>
extern "C" {
void JishsFirstPersonModCameraTail();
void* JishsFirstPersonModCameraOriginal{};
}
static std::uintptr_t captured_caller{};
static bool args_valid{};
extern "C" void JishsFirstPersonModCameraBefore(std::uintptr_t a,std::uintptr_t b,std::uintptr_t c,std::uintptr_t caller) noexcept {
    args_valid=a==11 && b==22 && c==33; captured_caller=caller;
}
extern "C" __declspec(noinline) std::uintptr_t target(std::uintptr_t a,std::uintptr_t b,std::uintptr_t c,std::uintptr_t d) {
    return a+b+c+d;
}
int main(int argc,char** argv) {
    using namespace dingosdk;
    if(argc!=2) return 1;
    const auto stock=LoadLibraryA(argv[1]);
    if(!stock) return 2;
    const auto install=reinterpret_cast<int(*)(void*)>(GetProcAddress(stock,"InstallStock"));
    if(!install || install(reinterpret_cast<void*>(&target))) return 3;
    if(hook_initialize()!=HookOk) return 4;
    const auto stock_handler=GetProcAddress(stock,"StockHandler");
    if(!stock_handler || hook_prepare(reinterpret_cast<void*>(stock_handler),reinterpret_cast<void*>(&JishsFirstPersonModCameraTail),&JishsFirstPersonModCameraOriginal)!=HookOk) return 5;
    if(hook_enable(reinterpret_cast<void*>(stock_handler))!=HookOk) return 6;
    // Detours follows the game's existing jump into stock's handler. The tail
    // forwarder must preserve both all arguments and the original native caller.
    const auto seen=target(11,22,33,44);
    if(!seen || seen!=captured_caller || !args_valid) {
        std::cerr<<"caller/argument preservation failed"; return 7;
    }
    if(hook_disable(reinterpret_cast<void*>(stock_handler))!=HookOk) return 8;
    return target(11,22,33,44) ? 0:9;
}
