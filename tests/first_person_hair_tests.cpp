#include "first_person_hair.h"
#include "hair_network_filter.h"
#include "settings.h"
#include <iostream>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path module_directory() { return std::filesystem::current_path(); }
std::filesystem::path data_directory() { return module_directory(); }
}
using namespace jishsfirstpersonmod;
using namespace dingosdk::multiplayer;
void check(bool value,const char* message) { if(!value) { std::cerr<<message<<'\n';std::exit(1); } }
int main() {
    CosmeticRecipe live{skater_recipe_key,1,{3,4},{{scalp_hair_slot,"Own_CustHairScalp_Straight_LowPonytail_00001",{}},{3991046852u,"Own_CustScalp_HairColor_00008",{42}},{7,"Top",{9}}}};
    const auto original=live;HairSwap hair;unsigned calls{};
    const auto apply=[&](const auto& recipe) { ++calls;live=recipe;return true; };
    check(hair.update(live,true,apply) && hair.owns(),"Hair swap failed");
    check(live.items[0].asset==bald_hair_asset && live.items[1]==original.items[1] && live.scalars==original.scalars,"Bald swap changed colour or appearance parameters");
    {
        NativeCosmeticItem native[]={{bald_hair_asset,nullptr,cosmetic_asset_hash(bald_hair_asset),scalp_hair_slot},
            {"Other",nullptr,cosmetic_asset_hash("Other"),7}};
        const auto before=native[0];const auto unaffected=native[1];
        const auto address=reinterpret_cast<std::uintptr_t>(native);
        auto outgoing=std::array{native[0],native[1]};
        auto saved=*hair.original();saved.parameters={11,22};
        HairCaptureFilter filter(saved,address,2);
        filter.filter(address+1,outgoing.data(),sizeof(native));
        check(!filter.replaced,"Unrelated read was filtered");
        filter.filter(address,outgoing.data(),sizeof(native)-1);
        check(!filter.replaced,"Partial array read was filtered");
        filter.filter(address,outgoing.data(),sizeof(native));
        check(filter.replaced && std::string(outgoing[0].asset)==saved.asset && outgoing[0].hash==cosmetic_asset_hash(saved.asset),"Outgoing hair was not restored");
        check(outgoing[0].parameters[0]==11 && outgoing[0].parameters[1]==22,"Original hair parameters lost");
        std::uint32_t count{};std::memcpy(&count,reinterpret_cast<const std::byte*>(outgoing[0].parameters)-4,4);
        check(count==2,"Original hair parameters lack native array count");
        check(native[0].asset==before.asset && native[0].hash==before.hash && native[0].parameters==before.parameters,"Live hairstyle was modified by network capture");
        check(outgoing[1].asset==unaffected.asset && outgoing[1].hash==unaffected.hash,"Another cosmetic was filtered");
        outgoing[0].hash=cosmetic_asset_hash("External Hair");filter.replaced=false;
        filter.filter(address,outgoing.data(),sizeof(native));check(!filter.replaced,"External hairstyle change was overwritten");
        outgoing[0].hash=cosmetic_asset_hash(bald_hair_asset);outgoing[0].slot=7;
        filter.filter(address,outgoing.data(),sizeof(native));check(!filter.replaced,"Bald hash in another slot was filtered");
    }
    hair.update(live,true,apply);check(calls==1,"Bald hairstyle reapplied each frame");
    live.items[2].asset="New Top";
    check(hair.update(live,false,apply) && !hair.owns(),"Third-person restore failed");
    check(live.items[0]==original.items[0] && live.items[2].asset=="New Top","Restore lost original hair or unrelated changes");
    hair.update(live,true,apply);live.items[0].asset="Different Hairstyle";
    hair.update(live,false,apply);check(live.items[0].asset=="Different Hairstyle","Restore overwrote an external hairstyle change");
    const auto failed=[](auto&) { return false; };
    check(!hair.update(live,true,failed) && !hair.owns(),"Failed apply acquired ownership");
    hair.update(live,true,apply);
    check(!hair.update(live,false,failed) && hair.owns(),"Failed restore discarded original hair");
    hair.update(live,false,apply);check(!hair.owns(),"Restore retry failed");
    live.items[0].asset=bald_hair_asset;calls=0;hair.update(live,true,apply);hair.update(live,false,apply);
    check(calls==0 && !hair.owns() && live.items[0].asset==bald_hair_asset,"Naturally bald player changed");
    live=original;hair.update(live,true,apply);hair.abandon();live=original;live.items[0].asset="Respawn Hair";
    hair.update(live,true,apply);hair.update(live,false,apply);check(live.items[0].asset=="Respawn Hair","Old actor hairstyle carried to replacement");
    const auto path=std::filesystem::current_path()/L"hair-test-data"/L"settings.ini";
    SettingsStore store(path,Config{});check(store.snapshot().config.bald_first_person,"Bald option default changed");
    Command command;command.kind=CommandKind::bald_hair;command.enabled=false;
    check(store.submit(command,100),"Bald option command failed");store.flush_due(100);
    check(!store.snapshot().unsaved && !read_config(path).bald_first_person,"Bald option did not persist");
    std::filesystem::remove(path);
    std::cout<<"Hair swap, exact restoration, external edits, failure retries and settings passed\n";
}
