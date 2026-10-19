#pragma once
#include "Engine/Game/Input/playstation_report.h"
#include <array>
#include <cstddef>
#include <cstdint>
namespace jishsfirstpersonmod {
// ReSkate 2.0.7 and 2.1.1 input_capture.cpp::InputCaptureHooks::HidRead. POD only.
struct StockHidRead { void* file{};void* buffer{};std::uint32_t capacity{};dingosdk::PlayStationPad kind{}; };
static_assert(sizeof(StockHidRead)==24 && offsetof(StockHidRead,buffer)==8 && offsetof(StockHidRead,capacity)==16 && offsetof(StockHidRead,kind)==20);
inline constexpr std::array<unsigned char,54> hid_prefix_207{0x48,0x89,0x5c,0x24,0x10,0x57,0x48,0x83,0xec,0x40,0x8b,0xda,0x48,0x8b,0xf9,0xe8,0xcc,0xcb,0xff,0xff,0x84,0xc0,0x0f,0x84,0x52,0x02,0x00,0x00,0x4c,0x8b,0x57,0x08,0x4d,0x85,0xd2,0x0f,0x84,0x45,0x02,0x00,0x00,0x3b,0x5f,0x10,0x0f,0x87,0x3c,0x02,0x00,0x00,0x0f,0xb6,0x57,0x14};
inline constexpr std::array<unsigned char,54> hid_prefix_211{0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x50,0x8b,0xfa,0x48,0x8b,0xf1,0xe8,0xfc,0xc8,0xff,0xff,0x84,0xc0,0x0f,0x84,0x36,0x01,0x00,0x00,0x48,0x89,0x5c,0x24,0x60,0x48,0x8b,0x5e,0x08,0x48,0x85,0xdb,0x0f,0x84,0x1f,0x01,0x00,0x00,0x3b,0x7e,0x10,0x0f,0x87,0x16,0x01,0x00};
inline constexpr std::array<std::uintptr_t,3> hid_callers_207{0x42d0c9,0x42d0e0,0x42d9b2};
inline constexpr std::array<std::uintptr_t,3> hid_callers_211{0xdf35ae,0xdf3e9b,0};
inline std::uintptr_t stock_hid_rva=0x42f360;
inline std::array<unsigned char,54> stock_hid_prefix=hid_prefix_207;
inline std::array<std::uintptr_t,3> stock_hid_callers=hid_callers_207;
inline std::size_t stock_hid_caller_count=3;
inline void select_layout(bool v211) {
    stock_hid_rva=v211 ? 0xdf5b00:0x42f360;
    stock_hid_prefix=v211 ? hid_prefix_211:hid_prefix_207;
    stock_hid_callers=v211 ? hid_callers_211:hid_callers_207;
    stock_hid_caller_count=v211 ? 2:3;
}
template<class Read> bool verified_stock_hid(Read read,std::uintptr_t base) {
    std::array<unsigned char,54> actual{};
    if(!read(base+stock_hid_rva,actual.data(),actual.size()) || actual!=stock_hid_prefix) return false;
    for(std::size_t index=0;index<stock_hid_caller_count;++index) {
        const auto caller=stock_hid_callers[index];
        std::array<unsigned char,5> call{};if(!read(base+caller,call.data(),call.size()) || call[0]!=0xe8) return false;
        std::int32_t displacement{};std::memcpy(&displacement,call.data()+1,4);
        if(static_cast<std::intptr_t>(caller)+5+displacement!=stock_hid_rva) return false;
    }
    return true;
}
}
