#include "stick_runtime.h"
#include "stick_input_filter.h"
#include "stock_bridge.h"
#include "owned_hooks.h"
#include <Xinput.h>
#include <vector>
#include <set>
#include <map>
#include <mutex>
namespace jishsfirstpersonmod {
extern thread_local bool raw_controller_read;
namespace {
using GetState=DWORD(WINAPI*)(DWORD,XINPUT_STATE*);
GetState originals[10]{};
template<unsigned Index> DWORD WINAPI filtered_state(DWORD slot,XINPUT_STATE* output) {
    const auto result=originals[Index](slot,output);const auto error=GetLastError();
    if(result==ERROR_SUCCESS && output && !raw_controller_read && stick_captures_input(slot))
        output->Gamepad.sThumbRX=output->Gamepad.sThumbRY=0;
    SetLastError(error);return result;
}
using RawData=UINT(WINAPI*)(HRAWINPUT,UINT,LPVOID,PUINT,UINT);
using RawBuffer=UINT(WINAPI*)(PRAWINPUT,PUINT,UINT);
RawData original_data{};RawBuffer original_buffer{};
std::mutex devices_mutex;
std::map<HANDLE,dingosdk::PlayStationPad> devices;
void filter_record(RAWINPUT* record,UINT bytes) {
    constexpr auto header=offsetof(RAWINPUT,data.hid.bRawData);
    if(!record || bytes<header || record->header.dwType!=RIM_TYPEHID || !stick_captures_input()) return;
    dingosdk::PlayStationPad kind{};
    {
        std::lock_guard lock(devices_mutex);
        const auto found=devices.find(record->header.hDevice);
        if(found!=devices.end()) kind=found->second;
        else {
            RID_DEVICE_INFO info{};info.cbSize=sizeof(info);UINT size=sizeof(info);
            if(GetRawInputDeviceInfoW(record->header.hDevice,RIDI_DEVICEINFO,&info,&size)!=UINT(-1) && info.dwType==RIM_TYPEHID)
                kind=dingosdk::playstation_pad(static_cast<std::uint16_t>(info.hid.dwVendorId),static_cast<std::uint16_t>(info.hid.dwProductId));
            if(devices.size()>32) devices.clear();devices.emplace(record->header.hDevice,kind);
        }
    }
    if(kind==dingosdk::PlayStationPad::none) return;
    auto& hid=record->data.hid;
    if(!hid.dwSizeHid || hid.dwCount>(bytes-header)/hid.dwSizeHid) return;
    for(UINT i=0;i<hid.dwCount;++i) neutral_sony_stick(kind,{hid.bRawData+i*hid.dwSizeHid,hid.dwSizeHid});
}
UINT WINAPI filtered_data(HRAWINPUT raw,UINT command,LPVOID data,PUINT size,UINT header) {
    const auto result=original_data(raw,command,data,size,header);const auto error=GetLastError();
    if(result!=UINT(-1) && data && command==RID_INPUT) filter_record(static_cast<RAWINPUT*>(data),result);
    SetLastError(error);return result;
}
UINT WINAPI filtered_buffer(PRAWINPUT data,PUINT size,UINT header) {
    const auto capacity=size ? *size:0;
    const auto result=original_buffer(data,size,header);const auto error=GetLastError();
    if(result!=UINT(-1) && data) {
        auto* cursor=reinterpret_cast<unsigned char*>(data);UINT remaining=capacity;
        for(UINT i=0;i<result && remaining>=sizeof(RAWINPUTHEADER);++i) {
            auto* record=reinterpret_cast<RAWINPUT*>(cursor);
            const auto bytes=record->header.dwSize;if(bytes<sizeof(RAWINPUTHEADER) || bytes>remaining) break;
            filter_record(record,bytes);
            const auto aligned=(bytes+sizeof(void*)-1)&~(sizeof(void*)-1);
            if(aligned>remaining) break;cursor+=aligned;remaining-=static_cast<UINT>(aligned);
        }
    }
    SetLastError(error);return result;
}
}
bool install_stick_input_filter() {
    using namespace dingosdk;
    const void* detours[]{reinterpret_cast<void*>(&filtered_state<0>),reinterpret_cast<void*>(&filtered_state<1>),
        reinterpret_cast<void*>(&filtered_state<2>),reinterpret_cast<void*>(&filtered_state<3>),
        reinterpret_cast<void*>(&filtered_state<4>),reinterpret_cast<void*>(&filtered_state<5>),
        reinterpret_cast<void*>(&filtered_state<6>),reinterpret_cast<void*>(&filtered_state<7>),
        reinterpret_cast<void*>(&filtered_state<8>),reinterpret_cast<void*>(&filtered_state<9>)};
    const wchar_t* names[]{L"xinput1_4.dll",L"xinput1_3.dll",L"xinput9_1_0.dll",L"xinput1_2.dll",L"xinput1_1.dll"};
    std::set<std::uintptr_t> seen;std::vector<void*> pending;
    unsigned count{};
    for(unsigned i=0;i<5;++i) {
        const auto module=GetModuleHandleW(names[i]);if(!module) continue;
        for(unsigned extended=0;extended<2;++extended) {
            const auto entry=reinterpret_cast<std::uintptr_t>(GetProcAddress(module,extended ? MAKEINTRESOURCEA(100):"XInputGetState"));
            if(!entry) continue;
            const auto handler=resolve_stock_handler(entry);const auto address=handler ? handler:entry;
            if(!seen.insert(address).second) continue;
            const unsigned index=i*2+extended;void* forwarding{};
            auto* target=reinterpret_cast<void*>(address);
            if(owned_hook_prepare(target,const_cast<void*>(detours[index]),&forwarding)!=HookOk) return false;
            originals[index]=reinterpret_cast<GetState>(forwarding);pending.push_back(target);++count;
        }
    }
    if(!count) return false;
    const auto user32=GetModuleHandleW(L"user32.dll");
    const auto add=[&](const char* name,void* detour,void** original) {
        const auto entry=reinterpret_cast<std::uintptr_t>(GetProcAddress(user32,name));
        const auto handler=resolve_stock_handler(entry);auto* target=reinterpret_cast<void*>(handler ? handler:entry);
        if(owned_hook_prepare(target,detour,original)!=HookOk) return false;
        pending.push_back(target);return true;
    };
    void* data{};void* buffer{};
    if(!add("GetRawInputData",reinterpret_cast<void*>(&filtered_data),&data) ||
       !add("GetRawInputBuffer",reinterpret_cast<void*>(&filtered_buffer),&buffer)) return false;
    original_data=reinterpret_cast<RawData>(data);original_buffer=reinterpret_cast<RawBuffer>(buffer);
    for(auto* target:pending) if(hook_queue_enable(target)!=HookOk) return false;
    return apply_owned_hooks()==HookOk;
}
}
