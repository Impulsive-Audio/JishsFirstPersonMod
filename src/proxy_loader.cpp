#include <Windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "Engine/Game/Build/image_identity.h"
#include "Extension/UI/Overlay/overlay.h"
#include <filesystem>
#include <fstream>
#include <mutex>
#include "rotating_log.h"

namespace {
HMODULE self{};
INIT_ONCE forwarding_once=INIT_ONCE_STATIC_INIT;
HMODULE genuine{};
BOOL CALLBACK initialize_forwarding(PINIT_ONCE, PVOID, PVOID*) {
    wchar_t system[MAX_PATH]{};
    const auto size=GetSystemDirectoryW(system,MAX_PATH);
    if(!size || size>=MAX_PATH) return FALSE;
    const auto path=std::filesystem::path(system)/L"dinput8.dll";
    genuine=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    return genuine!=nullptr;
}
template<class T> T native(const char* name) {
    if(!InitOnceExecuteOnce(&forwarding_once,initialize_forwarding,nullptr,nullptr)) return nullptr;
    return reinterpret_cast<T>(GetProcAddress(genuine,name));
}
void log(const char* message) noexcept {
    try {
        wchar_t path[32768]{};
        const auto size=GetModuleFileNameW(self,path,32768);
        if(!size || size>=32768) return;
        const auto root=std::filesystem::path(path).parent_path()/L"scripts/Custom/FirstPerson/logs";
        std::filesystem::create_directories(root);
        const auto record=std::to_string(GetTickCount64())+" [dinput8 proxy] "+message+'\n';
        jishsfirstpersonmod::append_rotating_log(root/L"proxy-loader.log",record,jishsfirstpersonmod::text_log_limit);
    } catch(...) {}
}
DWORD WINAPI load_companion(void*) noexcept {
    try {
        wchar_t host[32768]{},path[32768]{};
        if(!GetModuleFileNameW(nullptr,host,32768) || !GetModuleFileNameW(self,path,32768)) return 0;
        if(_wcsicmp(std::filesystem::path(host).filename().c_str(),L"Skate.exe")!=0) return 0;
        const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
        if(!dingosdk::supported_build::running_image_matches(base)) {
            log("Unsupported Skate.exe; companion not loaded. DirectInput forwarding remains active.");return 0;
        }
        HMODULE pinned{};
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
                             reinterpret_cast<LPCWSTR>(&load_companion),&pinned)) return 0;
        const auto dll=std::filesystem::path(path).parent_path()/L"scripts/Custom/JishsFirstPersonMod.dll";
        if(!std::filesystem::is_regular_file(dll)) { log("Companion DLL missing; DirectInput forwarding remains active.");return 0; }
        log("Loaded by Skate. Waiting for the standard launcher's ReSkate graphics initialization.");
        using Status=void(*)(dingosdk::overlay::Status*);
        bool ready=false;
        for(unsigned attempt=0;attempt<6000;++attempt) {
            if(const auto runtime=GetModuleHandleW(L"ReSkate.dll")) {
                if(const auto query=reinterpret_cast<Status>(GetProcAddress(runtime,"DingoSDKOverlayGetStatus"))) {
                    dingosdk::overlay::Status status{};
                    query(&status);
                    if(status.started && status.ready && !status.failed && !status.stopping) { ready=true;break; }
                    if(status.failed || status.stopping) { log("ReSkate failed or stopped; companion not loaded.");return 0; }
                }
            }
            Sleep(100);
        }
        if(!ready) { log("ReSkate readiness timed out; companion not loaded.");return 0; }
        // Loader work runs on this worker, never while holding DllMain's loader lock.
        if(GetModuleHandleW(L"JishsFirstPersonMod.dll")) { log("Companion already loaded; no second load.");return 0; }
        if(!LoadLibraryExW(dll.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)) {
            log("Companion LoadLibraryEx failed; DirectInput forwarding remains active.");return 0;
        }
        log("Companion DLL loaded. See JishsFirstPersonMod.log for camera and panel initialization.");
    } catch(...) { log("Companion loader exception; DirectInput forwarding remains active."); }
    return 0;
}
}
extern "C" HRESULT WINAPI ProxyDirectInput8Create(HINSTANCE instance,DWORD version,REFIID iid,LPVOID* output,LPUNKNOWN outer) {
    using Fn=HRESULT(WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
    const auto fn=native<Fn>("DirectInput8Create");return fn ? fn(instance,version,iid,output,outer):E_FAIL;
}
extern "C" HRESULT WINAPI ProxyDllCanUnloadNow() {
    const auto fn=native<HRESULT(WINAPI*)()>("DllCanUnloadNow");return fn ? fn():S_FALSE;
}
extern "C" HRESULT WINAPI ProxyDllGetClassObject(REFCLSID clsid,REFIID iid,LPVOID* output) {
    using Fn=HRESULT(WINAPI*)(REFCLSID,REFIID,LPVOID*);
    const auto fn=native<Fn>("DllGetClassObject");return fn ? fn(clsid,iid,output):E_FAIL;
}
extern "C" HRESULT WINAPI ProxyDllRegisterServer() {
    const auto fn=native<HRESULT(WINAPI*)()>("DllRegisterServer");return fn ? fn():E_FAIL;
}
extern "C" HRESULT WINAPI ProxyDllUnregisterServer() {
    const auto fn=native<HRESULT(WINAPI*)()>("DllUnregisterServer");return fn ? fn():E_FAIL;
}
extern "C" LPCDIDATAFORMAT WINAPI ProxyGetdfDIJoystick() {
    const auto fn=native<LPCDIDATAFORMAT(WINAPI*)()>("GetdfDIJoystick");return fn ? fn():nullptr;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        self=module;DisableThreadLibraryCalls(module);
        if(const auto thread=CreateThread(nullptr,0,load_companion,nullptr,0,nullptr)) CloseHandle(thread);
    }
    return TRUE;
}
