#include "hippy_jump.h"
#include <stdexcept>
#include <iostream>
using namespace jishsfirstpersonmod;
void check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
HippyJumpEvidence evidence(unsigned physics,float y,bool falling=false,bool board=false) {
    return {1,true,false,falling,!falling && !board,board,physics,{0,y,0}};
}
int main() {
    HippyJumpDetector detector;
    detector.update(evidence(100,0,false,true),0);
    auto first=detector.update(evidence(504,.05f,true),.05);
    check(first.candidate && !first.active && first.rising_samples==1,"candidate missing");
    check(detector.update(evidence(504,.06f,true),.05).rising_samples==1,"same tick advanced detection");
    check(detector.update(evidence(504,.1f,true),.1).active,"hippy jump not confirmed");
    check(detector.update(evidence(504,.09f,true),.15).active,"descent ended jump prematurely");
    check(!detector.update(evidence(100,.08f,false,true),.2).active,"remount retained hippy lock");
    detector.reset();detector.update(evidence(100,0,false,true),0);
    detector.update(evidence(504,0,true),.05);
    check(!detector.update(evidence(504,-.1f,true),.1).active,"ordinary dismount classified hippy");
    detector.reset();detector.update(evidence(100,0,false,true),0);
    detector.update(evidence(200,.1f,false,true),.05);
    detector.update(evidence(504,.2f,true),.1);
    check(!detector.update(evidence(504,.3f,true),.15).active,"ollie/general airtime classified hippy");
    detector.reset();detector.update(evidence(504,0),0);
    detector.update(evidence(504,.05f,true),.05);
    check(!detector.update(evidence(504,.1f,true),.1).active,"on-foot jump classified hippy");
    detector.reset();detector.update(evidence(100,0,false,true),0);
    detector.update(evidence(504,.1f,false),.05);
    check(!detector.update(evidence(504,.2f,false),.1).active,"grounded ascent classified hippy");
    detector.reset();detector.update(evidence(100,0,false,true),0);
    detector.update(evidence(504,.025f,true),.05);
    check(!detector.update(evidence(504,.05f,true),.1).active,"threshold equality classified hippy");
    for(int kind=0;kind<5;++kind) {
        detector.reset();detector.update(evidence(100,0,false,true),0);
        detector.update(evidence(504,.1f,true),.05);
        auto next=evidence(504,.2f,true);double now=.1;
        if(kind==0) next.bail=true;
        if(kind==1) next.owner=2;
        if(kind==2) now=.4;
        if(kind==3) next.position[0]=10;
        if(kind==4) next.valid=false;
        check(!detector.update(next,now).active,"lifecycle/bail retained candidate");
    }
    detector.reset();detector.update(evidence(100,0,false,true),0);
    detector.update(evidence(504,0,true),.05);
    detector.update(evidence(504,0,true),.2);
    check(!detector.update(evidence(504,.2f,true),.31).candidate,"expired confirmation window persisted");
    std::cout<<"Hippy-jump confirmation, exclusions and tick history passed\n";
}
