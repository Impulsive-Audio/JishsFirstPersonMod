#pragma once
#include "spectator_contract.h"
#include <cmath>
#include <algorithm>
#include <vector>
namespace jishsfirstpersonmod::spectator {
// A render-buffer-only lease. Received pose vectors and cosmetic data are never
// touched; an actor generation and buffer identity guard every restoration.
class HiddenHead {
    struct Lease { Target actor;std::uintptr_t buffer;std::array<float,3> scale; };
    std::vector<Lease> leases_;
public:
    bool owned() const { return !leases_.empty(); }
    template<class Read,class Write,class Pose> void restore(Read read,Write write,Pose pose) {
        std::erase_if(leases_,[&](const Lease& lease) {
            if(!same_actor(read,lease.actor))return true;
            try {
                const auto current_pose=pose(lease.actor.holder);
                // Do not write a buffer that is no longer published. Keep its
                // lease until the same actor publishes it again, so a rotating
                // pose buffer cannot return with our hidden scale still set.
                if(current_pose.count!=395 || current_pose.buffer!=lease.buffer)return false;
                std::array<float,3> current{};
                if(!read(lease.buffer+103*0x30ULL,&current,sizeof(current)))return false;
                return current!=std::array<float,3>{.001f,.001f,.001f} || write(lease.buffer+103*0x30ULL,lease.scale);
            } catch(...) {return false;}
        });
    }
    template<class Read,class Write,class Pose> bool hide(Read read,Write write,Pose pose,const Target& actor,std::uintptr_t buffer) {
        restore(read,write,pose);
        if(leases_.size()>=16)return false;
        if(!buffer || !same_actor(read,actor))return false;
        try { const auto rendered=pose(actor.holder);if(rendered.count!=395 || rendered.buffer!=buffer)return false; }
        catch(...) { return false; }
        std::array<float,3> scale{};
        if(!read(buffer+103*0x30ULL,&scale,sizeof(scale)) ||
            !std::all_of(scale.begin(),scale.end(),[](float v) { return std::isfinite(v) && v>=.05f && v<=20; }))return false;
        if(!write(buffer+103*0x30ULL,std::array<float,3>{.001f,.001f,.001f}))return false;
        leases_.push_back(Lease{actor,buffer,scale});return true;
    }
};
}
