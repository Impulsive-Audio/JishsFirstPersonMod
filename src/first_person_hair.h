#pragma once
#include "Extension/Multiplayer/Remote/cosmetics.h"
#include <optional>
#include <algorithm>
namespace jishsfirstpersonmod {
inline constexpr std::uint32_t scalp_hair_slot=1787581620u;
inline constexpr const char* bald_hair_asset="Own_CustHairScalp_Bald_Smooth_00001";
// Only the scalp item is owned by this override. Colours, outfits and other
// slots remain the current live values, including changes made during play.
class HairSwap {
    std::optional<dingosdk::multiplayer::CosmeticSlot> original_;
public:
    bool owns() const { return original_.has_value(); }
    const auto& original() const { return original_; }
    void abandon() { original_.reset(); }
    template<class Apply> bool update(dingosdk::multiplayer::CosmeticRecipe current,bool wanted,Apply apply) {
        using namespace dingosdk::multiplayer;
        auto item=std::find_if(current.items.begin(),current.items.end(),[](const auto& slot) { return slot.slot==scalp_hair_slot; });
        if(item==current.items.end()) return false;
        if(original_ && item->asset!=bald_hair_asset) original_.reset(); // An external hairstyle change wins.
        if(wanted) {
            if(original_ || item->asset==bald_hair_asset) return true;
            const auto original=*item;item->asset=bald_hair_asset;
            if(!apply(current)) return false;
            original_=original;return true;
        }
        if(!original_) return true;
        *item=*original_;
        if(!apply(current)) return false;
        original_.reset();return true;
    }
};
void initialize_first_person_hair(std::uintptr_t base) noexcept;
void tick_first_person_hair() noexcept;
bool first_person_hair_available() noexcept;
}
