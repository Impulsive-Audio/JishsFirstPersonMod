#pragma once
#include "spectator_contract.h"
#include "Engine/Game/Skater/first_person_spring.h"
namespace jishsfirstpersonmod::spectator {
using namespace dingosdk::first_person;
struct EyePose { Matrix camera{};Vec3 origin{}; };
// ReSkate 2.0.7 client_first_person.cpp: Head has +X up, +Y out of
// the face and -Z right. Raw joint orientation is not camera orientation.
template<class Read> EyePose eye_pose(Read read,std::uintptr_t buffer,const Matrix& native) {
    struct Joint { Vec3 position{};Quat rotation{0,0,0,1};float scale=1; };
    Memory memory(read);
    auto child=[&](const Joint& parent,unsigned index) {
        const auto bone=memory.template get<std::array<float,12>>(buffer,index*0x30ULL);
        for(unsigned i:{0u,1u,2u,4u,5u,6u,7u,8u,9u,10u})
            if(!std::isfinite(bone[i]) || std::abs(bone[i])>=1000000)throw std::runtime_error("Watched eye pose is invalid.");
        const float norm=bone[4]*bone[4]+bone[5]*bone[5]+bone[6]*bone[6]+bone[7]*bone[7];
        if(norm<.5f || norm>1.5f || bone[0]<.05f || bone[1]<.05f || bone[2]<.05f || bone[0]>20 || bone[1]>20 || bone[2]>20)
            throw std::runtime_error("Watched eye pose is unavailable.");
        Joint result;
        result.position=add(parent.position,rotate(parent.rotation,{bone[8]*parent.scale,bone[9]*parent.scale,bone[10]*parent.scale}));
        result.rotation=multiply(parent.rotation,{bone[4],bone[5],bone[6],bone[7]});
        result.scale=parent.scale*(bone[0]+bone[1]+bone[2])/3;
        return result;
    };
    EyePose result;result.camera=native;Joint head;
    for(unsigned index:{0u,1u,7u,42u,43u,44u,45u,101u,102u,103u}) { head=child(head,index);if(index==1)result.origin=head.position; }
    const auto face=child(head,107),right_eye=child(face,108),left_eye=child(face,109);
    const auto separation=length(subtract(right_eye.position,left_eye.position))/std::max(head.scale,.01f);
    if(separation<=.03f || separation>=.15f)throw std::runtime_error("Watched eye pose failed its shape check.");
    const auto right=rotate(head.rotation,{0,0,-1}),up=rotate(head.rotation,{1,0,0}),forward=rotate(head.rotation,{0,1,0});
    for(unsigned i=0;i<3;++i) {
        result.camera[i]=right[i];result.camera[4+i]=up[i];result.camera[8+i]=-forward[i];
        result.camera[12+i]=(right_eye.position[i]+left_eye.position[i])*.5f+forward[i]*.05f*head.scale;
    }
    for(float value:result.camera)if(!std::isfinite(value))throw std::runtime_error("Watched eye camera is invalid.");
    return result;
}
}
