#include "head_attachment.h"
#include "Engine/Core/Platform/memory.h"
#include "Extension/Multiplayer/Remote/native_pose_layout.h"
#include <Windows.h>
namespace jishsfirstpersonmod {
HeadPose read_animation_head(const ComfortSnapshot& state) noexcept {
    using namespace dingosdk;using namespace first_person;
    HeadPose result;if(!state.state_valid || !state.entity) return result;
    try {
        auto read=[](std::uintptr_t address,void* data,std::size_t size) { return memory::read_bytes(address,data,size); };
        const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
        const auto component=multiplayer::read_native_skater_component(read,base,state.entity);
        std::uintptr_t holder{};if(!memory::read(component+0xa0,holder)) return result;
        const auto pose=multiplayer::read_native_pose_layout(read,base,holder,512);
        if(!pose.buffer || pose.count!=395) return result;
        Quat rotation{0,0,0,1};Vec3 position{};float scale=1;
        result.root[15]=result.head[15]=1;
        for(const unsigned joint:{0u,1u,7u,42u,43u,44u,45u,101u,102u,103u}) {
            std::array<float,12> bone{};if(!memory::read(pose.buffer+joint*0x30ULL,bone)) return {};
            for(unsigned i=0;i<11;++i) if(!std::isfinite(bone[i]) || std::abs(bone[i])>1000000) return {};
            const float norm=bone[4]*bone[4]+bone[5]*bone[5]+bone[6]*bone[6]+bone[7]*bone[7];
            if(norm<.5f || norm>1.5f) return {};
            position=add(position,rotate(rotation,{bone[8]*scale,bone[9]*scale,bone[10]*scale}));
            rotation=normalized(multiply(rotation,{bone[4],bone[5],bone[6],bone[7]}));
            if(joint!=103 && (bone[0]<.05f || bone[1]<.05f || bone[2]<.05f || bone[0]>20 || bone[1]>20 || bone[2]>20)) return {};
            scale*=(bone[0]+bone[1]+bone[2])/3;
            if(joint==1) write(result.root,rotation,position);
        }
        // Hidden Head scale affects descendants, never the Head joint's own origin.
        write(result.head,rotation,position);
        std::uintptr_t checked_holder{};
        if(!memory::read(component+0xa0,checked_holder) || checked_holder!=holder) return {};
        const auto checked=multiplayer::read_native_pose_layout(read,base,holder,512);
        if(checked.buffer!=pose.buffer || checked.count!=pose.count) return {};
        result.valid=true;
    } catch(...) { return {}; }
    return result;
}
}
