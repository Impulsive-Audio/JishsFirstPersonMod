#include "Engine/Game/Input/controller_bindings.h"
#include "Engine/Game/Skater/first_person_spring.h"
#include <stdexcept>
int main() {
using namespace dingosdk;
    const auto expect = [](bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    };
    const auto near = [](float a, float b) { return std::abs(a-b) < 0.0001f; };
    const auto same = [&](const first_person::Matrix& a, const first_person::Matrix& b) {
        for (unsigned i=0; i<a.size(); ++i) if (!near(a[i],b[i])) return false;
        return true;
    };
    auto controls = default_look_controls;
    for (unsigned i=0; i<4; ++i) controls[i].combo = 1u<<i;
    controls[1].angle=60; controls[3].angle=120;
    ControllerLookState state;
    ControllerInput pad{true,0,1,ControllerStyle::xbox};
    const auto neutral = [&](std::array<float,2> value) { expect(value[0]==0 && value[1]==0,"neutral look"); };
    neutral(state.update(controls,pad,false));
    pad.buttons=1; auto look=state.update(controls,pad,false);expect(look[0]==30 && look[1]==0,"up held");
    expect(state.update(controls,pad,false)[0]==30,"held look remains active");
    pad.buttons=0;neutral(state.update(controls,pad,false));
    pad.buttons=2;expect(state.update(controls,pad,false)[0]==-60,"down held");
    pad.buttons=4;expect(state.update(controls,pad,false)[1]==-90,"left held");
    pad.buttons=8;expect(state.update(controls,pad,false)[1]==120,"right held");
    pad.buttons=5;look=state.update(controls,pad,false);expect(look[0]==30 && look[1]==-90,"diagonal glance");
    pad.buttons=15;neutral(state.update(controls,pad,false));
    for (unsigned interruption=0; interruption<5; ++interruption) {
        pad.buttons=1;neutral(state.update(controls,pad,true));
        neutral(state.update(controls,pad,false));
        pad.buttons=0;neutral(state.update(controls,pad,false));
        pad.buttons=1;expect(state.update(controls,pad,false)[0]==30,"interruption requires neutral");
    }
    pad.device=2;neutral(state.update(controls,pad,false));pad.buttons=0;state.update(controls,pad,false);
    pad.buttons=1;expect(state.update(controls,pad,false)[0]==30,"reconnect release rearms");
    controls[0].combo=0x30;pad.buttons=0x30;neutral(state.update(controls,pad,false));
    pad.buttons=0;state.update(controls,pad,false);pad.buttons=0x30;expect(state.update(controls,pad,false)[0]==30,"changed combo release rearms");
    controls[0].angle=40;neutral(state.update(controls,pad,false));
    neutral(state.update(controls,{},false));neutral(state.update(controls,pad,false));
    pad.buttons=0x10;neutral(state.update(controls,pad,false));
    pad.buttons=0x30;neutral(state.update(controls,pad,false));
    pad.buttons=0;state.update(controls,pad,false);
    pad.buttons=0x30;expect(state.update(controls,pad,false)[0]==40,"all chord buttons release before rearming");


    first_person::Matrix head{};head[0]=head[5]=head[10]=head[15]=1;head[12]=4;head[13]=5;head[14]=6;
    first_person::Settings settings;
    auto target=first_person::target(head,settings,{30,0});
    expect(-target[9]>0 && near(-target[10],-std::cos(std::numbers::pi_v<float>/6)),"positive pitch looks up");
    expect(-first_person::target(head,settings,{-30,0})[9]<0,"negative pitch looks down");
    expect(near(-first_person::target(head,settings,{0,90})[8],1),"positive yaw looks right");
    expect(near(-first_person::target(head,settings,{0,-90})[8],-1),"negative yaw looks left");
    for (unsigned i=12;i<16;++i) expect(target[i]==head[i],"glance preserves position");
    settings.offset={0.3f,0.2f,0.1f};settings.rotation={10,20,5};
    const auto base=first_person::target(head,settings);
    target=first_person::target(head,settings,{30,90});
    for (unsigned i=12;i<16;++i) expect(near(target[i],base[i]),"static position offset preserved");
    const auto base_q=first_person::orientation(base);
    const auto expected_q=first_person::multiply(base_q,first_person::multiply(
        {0,std::sin(-std::numbers::pi_v<float>/4),0,std::cos(-std::numbers::pi_v<float>/4)},
        {std::sin(std::numbers::pi_v<float>/12),0,0,std::cos(std::numbers::pi_v<float>/12)}));
    auto expected=head;first_person::write(expected,expected_q,{base[12],base[13],base[14]});
    expect(same(target,expected),"yaw then pitch respects static rotation");
    first_person::FrameSpring frame;
    for (unsigned tick=0;tick<20;++tick) {
        frame.begin(tick/60.0);auto tick_pose=frame.sample(head,settings,{}, {30,90});
        auto render_pose=frame.sample(head,settings,{}, {30,90});
        expect(same(tick_pose,target) && same(tick_pose,render_pose),"disabled spring immediate and no accumulation");
    }
    frame.begin(0.34);expect(same(frame.sample(head,settings,{}),base),"disabled spring immediate return");
    settings.enabled=true;
    for (unsigned direction=0;direction<4;++direction) {
        first_person::LookAngles angles{direction==0 ? 30.0f : direction==1 ? -30.0f : 0.0f,
            direction==2 ? -60.0f : direction==3 ? 60.0f : 0.0f};
        first_person::FrameSpring smooth;
        smooth.begin(0);smooth.sample(head,settings,{});
        smooth.begin(1.0/60);auto first=smooth.sample(head,settings,{},angles);
        expect(!same(first,first_person::target(head,settings,angles)) && !same(first,base),"spring smooths activation");
        expect(same(first,smooth.sample(head,settings,{},angles)),"tick and render integrate only once");
        for (unsigned tick=2;tick<=300;++tick) {smooth.begin(tick/60.0);smooth.sample(head,settings,{},angles);}
        smooth.begin(301.0/60);auto returning=smooth.sample(head,settings,{});
        expect(!same(returning,base),"spring smooths release");
        for (unsigned tick=302;tick<=600;++tick) {smooth.begin(tick/60.0);returning=smooth.sample(head,settings,{});}
        expect(same(returning,base),"spring returns to existing rotation offset");
    }
    // The same directional strengths used for animated head movement govern held looks.
    settings={};settings.enabled=true;settings.up=settings.down=settings.left=settings.right=0;
    first_person::Spring immediate;first_person::update(immediate,head,settings,0);
    expect(same(first_person::update(immediate,head,settings,1.0/60,{}, {30,60}),first_person::target(head,settings,{30,60})),"zero strengths remain immediate");
    settings={};settings.enabled=true;settings.up=100;settings.down=0;
    first_person::Spring directional;first_person::update(directional,head,settings,0);
    expect(!same(first_person::update(directional,head,settings,1.0/60,{}, {30,0}),first_person::target(head,settings,{30,0})),"up strength respected");
    first_person::Spring other;first_person::update(other,head,settings,0);
    expect(same(first_person::update(other,head,settings,1.0/60,{}, {-30,0}),first_person::target(head,settings,{-30,0})),"down zero strength respected");
    settings={};settings.enabled=true;settings.left=100;settings.right=0;
    first_person::Spring left;first_person::update(left,head,settings,0);
    expect(!same(first_person::update(left,head,settings,1.0/60,{}, {0,-30}),first_person::target(head,settings,{0,-30})),"left strength respected");
    first_person::Spring right;first_person::update(right,head,settings,0);
    expect(same(first_person::update(right,head,settings,1.0/60,{}, {0,30}),first_person::target(head,settings,{0,30})),"right zero strength respected");
}
