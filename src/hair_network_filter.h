#pragma once
#include "first_person_hair.h"
#include "Extension/Multiplayer/Remote/native_cosmetics_layout.h"
namespace jishsfirstpersonmod {
// Borrow only for the synchronous capture. ReSkate copies these values into its
// own strings/vectors; no mod allocation is ever transferred to ReSkate.
struct HairCaptureFilter {
    dingosdk::multiplayer::CosmeticSlot original;
    dingosdk::multiplayer::CosmeticArray<std::uint32_t> parameters;
    std::uintptr_t items{};
    std::size_t bytes{};
    bool replaced{};
    HairCaptureFilter(const dingosdk::multiplayer::CosmeticSlot& hair,std::uintptr_t address,std::size_t count)
        : original(hair),parameters(hair.parameters.size()),items(address),bytes(count*sizeof(dingosdk::multiplayer::NativeCosmeticItem)) {
        std::copy(original.parameters.begin(),original.parameters.end(),parameters.data());
    }
    void filter(std::uintptr_t address,void* output,std::size_t size) noexcept {
        using namespace dingosdk::multiplayer;
        if(address!=items || size!=bytes || !size || size%sizeof(NativeCosmeticItem)) return;
        auto* slots=static_cast<NativeCosmeticItem*>(output);
        for(std::size_t i=0;i<size/sizeof(NativeCosmeticItem);++i) {
            auto& item=slots[i];
            if(item.slot==scalp_hair_slot && item.hash==cosmetic_asset_hash(bald_hair_asset)) {
                item.asset=original.asset.c_str();item.parameters=parameters.data();
                item.hash=cosmetic_asset_hash(original.asset);replaced=true;
            }
        }
    }
};
}
