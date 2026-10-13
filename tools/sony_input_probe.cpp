#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 0x0800
#include <Windows.h>
#include <cfgmgr32.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <dinput.h>
#include <iostream>
#include <vector>
#include <cwctype>
#include <algorithm>
#include "Engine/Game/Input/playstation_report.h"

int main() {
    GUID hid{};HidD_GetHidGuid(&hid);ULONG count{};
    if(CM_Get_Device_Interface_List_SizeW(&count,&hid,nullptr,CM_GET_DEVICE_INTERFACE_LIST_PRESENT)!=CR_SUCCESS) return 1;
    std::vector<wchar_t> paths(count);
    if(CM_Get_Device_Interface_ListW(&hid,nullptr,paths.data(),count,CM_GET_DEVICE_INTERFACE_LIST_PRESENT)!=CR_SUCCESS) return 1;
    for(const wchar_t* path=paths.data();*path;path+=wcslen(path)+1) {
        std::wstring lower(path);std::transform(lower.begin(),lower.end(),lower.begin(),[](wchar_t c){return std::towlower(c);});
        if(lower.find(L"054c")==std::wstring::npos) continue;
        const auto file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);
        if(file==INVALID_HANDLE_VALUE) {std::cout<<"Sony HID open failed: "<<GetLastError()<<'\n';continue;}
        HIDD_ATTRIBUTES attributes{sizeof(attributes)};PHIDP_PREPARSED_DATA preparsed{};HIDP_CAPS caps{};
        if(HidD_GetAttributes(file,&attributes) && HidD_GetPreparsedData(file,&preparsed)) {
            HidP_GetCaps(preparsed,&caps);HidD_FreePreparsedData(preparsed);
            std::cout<<"Sony HID VID/PID="<<std::hex<<attributes.VendorID<<'/'<<attributes.ProductID
                <<" collection="<<caps.UsagePage<<'/'<<caps.Usage<<std::dec<<" input bytes="<<caps.InputReportByteLength<<'\n';
            if(caps.UsagePage==1 && caps.Usage==5 && caps.InputReportByteLength>=10) {
                std::vector<unsigned char> report(caps.InputReportByteLength);OVERLAPPED read{};
                read.hEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);DWORD bytes{};
                const auto success=ReadFile(file,report.data(),static_cast<DWORD>(report.size()),&bytes,&read);
                const auto error=GetLastError();
                if(!success && error==ERROR_IO_PENDING) WaitForSingleObject(read.hEvent,250);
                if(GetOverlappedResult(file,&read,&bytes,FALSE)) {
                    const auto parsed=dingosdk::parse_playstation_report(dingosdk::playstation_pad(attributes.VendorID,attributes.ProductID),caps.InputReportByteLength>64,report.data(),bytes);
                    std::cout<<"USB sample size="<<bytes<<" id="<<std::hex<<unsigned(report[0])<<" buttons="<<(parsed?parsed->buttons:0)<<std::dec<<" parsed="<<bool(parsed)<<'\n';
                } else {CancelIoEx(file,&read);GetOverlappedResult(file,&read,&bytes,TRUE);std::cout<<"Sony sample unavailable\n";}
                CloseHandle(read.hEvent);
            }
        }
        CloseHandle(file);
    }
    IDirectInput8W* owner{};
    if(FAILED(DirectInput8Create(GetModuleHandleW(nullptr),DIRECTINPUT_VERSION,IID_IDirectInput8W,reinterpret_cast<void**>(&owner),nullptr))) return 2;
    owner->EnumDevices(DI8DEVCLASS_GAMECTRL,[](const DIDEVICEINSTANCEW* info,void* pointer)->BOOL {
        auto* owner=static_cast<IDirectInput8W*>(pointer);IDirectInputDevice8W* device{};
        if(FAILED(owner->CreateDevice(info->guidInstance,&device,nullptr))) return DIENUM_CONTINUE;
        DIPROPDWORD identity{};identity.diph={sizeof(identity),sizeof(identity.diph),0,DIPH_DEVICE};
        if(SUCCEEDED(device->GetProperty(DIPROP_VIDPID,&identity.diph))) {
            std::cout<<"DirectInput VID/PID="<<std::hex<<LOWORD(identity.dwData)<<'/'<<HIWORD(identity.dwData)<<std::dec<<'\n';
            if(LOWORD(identity.dwData)==0x054c) {
                std::cout<<"Set standard format="<<std::hex<<device->SetDataFormat(&c_dfDIJoystick2)<<std::dec<<'\n';
                device->EnumObjects([](const DIDEVICEOBJECTINSTANCEW* object,void*)->BOOL {
                    std::cout<<"  object offset="<<object->dwOfs<<" type="<<std::hex<<object->dwType<<" usage="<<object->wUsagePage<<'/'<<object->wUsage<<std::dec<<'\n';return DIENUM_CONTINUE;
                },nullptr,DIDFT_ALL);
                for(DWORD offset:{DWORD(offsetof(DIJOYSTATE2,rgdwPOV)),DWORD(offsetof(DIJOYSTATE2,rgbButtons)+4)}) {
                    DIDEVICEOBJECTINSTANCEW object{};object.dwSize=sizeof(object);
                    const auto result=device->GetObjectInfo(&object,offset,DIPH_BYOFFSET);
                    std::cout<<"  offset lookup "<<offset<<" HRESULT="<<std::hex<<result<<" usage="<<object.wUsagePage<<'/'<<object.wUsage<<std::dec<<'\n';
                }
            }
        }
        device->Release();return DIENUM_CONTINUE;
    },owner,DIEDFL_ATTACHEDONLY);
    owner->Release();
}
