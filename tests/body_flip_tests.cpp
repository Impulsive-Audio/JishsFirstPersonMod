#include "body_flip.h"
#include "bail_view.h"
#include "settings.h"
#include "camera_policy.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path module_directory() { return std::filesystem::current_path(); }
std::filesystem::path data_directory() { return module_directory(); }
}
using namespace jishsfirstpersonmod;
using namespace dingosdk::first_person;
Vec3 position(const Matrix& m) { return {m[12],m[13],m[14]}; }
void check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
Matrix pose(float degrees=0,Vec3 where={}) { Matrix m{};const float angle=degrees*std::numbers::pi_v<float>/360;write(m,{std::sin(angle),0,0,std::cos(angle)},where);return m; }
body_flip::Evidence evidence(float tilt,bool air=true) {
    auto m=pose(tilt);body_flip::Evidence e;e.owner={1,2,3,4};e.valid=e.enabled=true;e.airborne=air;e.up={m[4],m[5],m[6]};return e;
}
body_flip::Status launch(body_flip::Detector& d,double start=0) {
    auto s=d.update(evidence(0),start);
    for(int i=1;i<=20;++i) s=d.update(evidence(float(i*5)),start+i*.01);
    check(s.owns_head(),"front flip was not detected");return s;
}
int main() { try {
    body_flip::Detector d;auto s=launch(d);check(s.tilt>=80,"locked below entry threshold");
    body_flip::Detector below;for(int i=0;i<80;++i) check(!below.update(evidence(float(i)),i*.01).owns_camera(),"transition below 80 degrees triggered");
    body_flip::Detector quick;quick.update(evidence(75),0);check(!quick.update(evidence(85),.01).owns_camera(),"brief pose change triggered");
    body_flip::Detector back;
    for(int i=0;i<=20;++i) s=back.update(evidence(float(-i*5)),i*.01);
    check(s.owns_head(),"back flip was not detected");
    body_flip::Detector roll;
    for(int i=0;i<=20;++i) { auto e=evidence(0);float a=i*5*std::numbers::pi_v<float>/180;e.up={std::sin(a),std::cos(a),0};s=roll.update(e,i*.01); }
    check(s.owns_head(),"sideways roll was not detected");
    body_flip::Detector spin;
    for(int i=0;i<100;++i) check(!spin.update(evidence(0),i*.01).owns_camera(),"upright spin/board-only flip triggered");
    body_flip::Detector foot;
    for(int i=0;i<=20;++i) check(!foot.update(evidence(float(i*5),false),i*.01).owns_camera(),"off-board motion triggered");
    body_flip::Detector continuous;launch(continuous);
    for(int i=21;i<=50;++i) s=continuous.update(evidence(float(i*5)),i*.01);
    check(s.phase==body_flip::Phase::locked,"sustained flip did not reach direct lock");
    body_flip::Detector settling;launch(settling);
    for(int i=21;i<=34;++i) s=settling.update(evidence(100),i*.01);
    check(s.phase==body_flip::Phase::leaving,"quiet rotation did not release during entry");
    for(int i=35;i<=62;++i) s=settling.update(evidence(100),i*.01);
    check(s.phase==body_flip::Phase::idle,"exit did not finish");
    for(int i=63;i<=75;++i) s=settling.update(evidence(float(100+(i-62)*4)),i*.01);
    check(!s.owns_head(),"retriggered without rearming");
    settling.update(evidence(30),.76);for(int i=1;i<=14;++i) s=settling.update(evidence(float(30+i*5)),.76+i*.01);
    check(s.owns_head(),"rearming below 60 degrees failed");
    body_flip::Detector landing;launch(landing);s=landing.update(evidence(100,false),.21);check(s.phase==body_flip::Phase::leaving,"landing did not start exit");
    body_flip::Detector disabled;launch(disabled);auto off=evidence(100);off.enabled=false;s=disabled.update(off,.21);check(s.phase==body_flip::Phase::leaving,"disabling did not start exit");
    for(int failure=0;failure<6;++failure) {
        body_flip::Detector reset;auto before=launch(reset);auto e=evidence(105);double t=.21;
        if(failure==0)e.valid=false; if(failure==1)e.owner[0]=9;if(failure==2)e.position={6,0,0};
        if(failure==3)t=.6;if(failure==4)t=.1;if(failure==5)e.up={NAN,0,0};
        s=reset.update(e,t);check(!s.owns_camera() && s.epoch!=before.epoch,"discontinuity kept stale lock");
    }
    body_flip::Camera camera;auto normal=pose(0,{0,0,0}),head=pose(120,{2,0,0});body_flip::Status status;
    camera.apply(normal,head,status,1);status.phase=body_flip::Phase::entering;status.started=1;
    check(length(subtract(position(camera.apply(normal,head,status,1)),position(normal)))<1e-6f,"entry position snapped");
    auto halfway=camera.apply(normal,head,status,1.125);check(std::abs(halfway[12]-1)<1e-5,"entry smoothstep midpoint wrong");
    auto fully=camera.apply(normal,head,status,1.25);check(length(subtract(position(fully),position(head)))<1e-5,"entry did not reach head in 250ms");
    for(float direction:{-1.f,1.f}) {
        body_flip::Camera fast;body_flip::Status handoff;
        const auto start_head=pose(direction*80,{10,2,4});
        const auto start_view=pose(direction*65,{10,2.2f,4.1f});
        fast.apply(start_view,start_head,handoff,4);
        handoff.phase=body_flip::Phase::entering;handoff.started=4;
        const auto initial=fast.apply(start_view,start_head,handoff,4);
        check(length(subtract(position(initial),position(start_view)))<1e-5,"moving entry snapped");
        const auto moving_head=pose(direction*155,{11,2.8f,5});
        const auto tracking=fast.apply(start_view,moving_head,handoff,4.125);
        check(std::abs(length(subtract(position(tracking),position(moving_head)))-length(subtract(position(start_view),position(start_head)))*.5f)<1e-5,
            "fast flip translation/rotation increased handoff offset");
        const auto inv_start=Quat{-orientation(start_head)[0],0,0,orientation(start_head)[3]};
        const auto residual=normalized(multiply(inv_start,orientation(start_view)));
        const auto expected_rotation=normalized(multiply(orientation(moving_head),body_flip::interpolate(residual,{0,0,0,1},.5f)));
        float dot=0;for(unsigned i=0;i<4;++i)dot+=orientation(tracking)[i]*expected_rotation[i];
        check(std::abs(dot)> .99999f,"fast flip rotation lagged world-space starting pose");
        check(length(subtract(position(fast.apply(start_view,moving_head,handoff,4.25)),position(moving_head)))<1e-5,"moving entry timing changed");
    }
    status.phase=body_flip::Phase::locked;auto moving=pose(140,{3,2,1});auto direct=camera.apply(normal,moving,status,1.26);
    check(length(subtract(position(direct),position(moving)))<1e-6,"locked position was smoothed");
    check(std::abs(orientation(direct)[0]-orientation(moving)[0])<1e-6,"locked rotation was smoothed");
    status.phase=body_flip::Phase::leaving;status.started=1.27;auto exit=camera.apply(normal,moving,status,1.27);
    check(length(subtract(position(exit),position(direct)))<1e-6,"exit position snapped");
    auto destination=pose(0,{.5f,0,0});exit=camera.apply(destination,moving,status,1.52);
    check(length(subtract(position(exit),position(destination)))<1e-5,"exit did not track moving normal target");
    auto qa=orientation(pose(170)),qb=orientation(pose(-170));auto q=body_flip::interpolate(qa,qb,.5f);
    check(std::abs(q[0])>.999f,"quaternion took long path across wraparound");
    for(auto& v:qb)v=-v;check(std::abs(body_flip::interpolate(qa,qb,.5f)[0])>.999f,"antipodal quaternion changed path");
    BailView calibrated;HeadPose hp;hp.valid=true;hp.head=pose(0,{0,0,0});hp.root=hp.head;
    auto eye=pose(10,{0,.1f,0});calibrated.remember(eye,eye,hp,true);hp.head=pose(100,{1,2,3});
    auto attached=calibrated.attach_full(normal,hp);auto expected=add(position(hp.head),rotate(orientation(hp.head),{0,.1f,0}));
    check(length(subtract(position(attached),expected))<1e-5,"calibrated eye offset lost");
    check(std::abs(orientation(attached)[0]-orientation(pose(110))[0])<1e-5,"calibrated rotation offset lost");
    body_flip::Camera aborted;body_flip::Status in;aborted.apply(normal,head,in,2);
    in.phase=body_flip::Phase::entering;in.started=2;aborted.apply(normal,head,in,2);
    auto partial=aborted.apply(normal,head,in,2.1);in.phase=body_flip::Phase::leaving;in.started=2.1;
    check(length(subtract(position(aborted.apply(normal,head,in,2.1)),position(partial)))<1e-6,"interrupted entry snapped");
    in.epoch=1;in.phase=body_flip::Phase::idle;
    check(length(subtract(position(aborted.apply(normal,head,in,2.11)),position(normal)))<1e-6,"reset reused stale pose");
    Settings explicit_offset;explicit_offset.enabled=true;
    explicit_offset.offset={.036f,.030f,.018f};explicit_offset.rotation={-16.3f,-.5f,-1.1f};
    for(float angle:{-170.f,-100.f,100.f,170.f}) {
        auto animated=pose(angle,{4,2,8});auto exact=body_flip::head_target(animated,explicit_offset);
        const auto expected_position=add(position(animated),rotate(orientation(animated),{.036f,.030f,-.018f}));
        check(length(subtract(position(exact),expected_position))<1e-5,"flip introduced an inferred positional offset");
        check(std::abs(length(subtract(position(exact),position(animated)))-length(explicit_offset.offset))<1e-5,"front/back flip offset lengths differed");
        body_flip::Camera independent;auto polluted=pose(0,{4,2.3f,8.25f});body_flip::Status locked;locked.phase=body_flip::Phase::locked;
        auto output=independent.apply(polluted,exact,locked,3);
        check(length(subtract(position(output),expected_position))<1e-5,"locked flip inherited Spring lag");
        Settings zero;auto centered=body_flip::head_target(animated,zero);
        check(length(subtract(position(centered),position(animated)))<1e-6,"zero offsets did not lock exactly to head");
    }
    Config policy;policy.spring.enabled=true;policy.lock_body_flips=true;policy.stick.enabled=false;
    ComfortSnapshot owner;owner.state_valid=owner.animation_valid=true;CameraMovementSample motion;motion.mode=2;motion.non_bail=true;motion.body_flip_head_lock=true;
    check(head_position_attached(policy,owner,motion),"body-flip lock depended on right-stick look");
    check(!effective_camera_config(policy,true,owner,motion).spring.enabled,"effective camera retained Spring during flip");
    policy.look_only=true;check(!head_position_attached(policy,owner,motion),"compatibility mode enabled head lock");
    policy.look_only=false;policy.lock_body_flips=false;check(!head_position_attached(policy,owner,motion),"disabled flip option retained lock");
    auto file=std::filesystem::current_path()/"body-flip-settings/settings.ini";Config cfg;
    check(!cfg.lock_body_flips,"default must be off");cfg.lock_body_flips=true;write_config(file,cfg);
    check(read_config(file).lock_body_flips,"option persistence failed");
    auto older=file.parent_path()/"older.ini";{std::ofstream out(older);out<<"[JishsFirstPersonMod]\n";}
    check(!read_config(older).lock_body_flips,"older preset unexpectedly enabled flip lock");
    SettingsStore store(file,{});Command cmd;cmd.kind=CommandKind::lock_body_flips;cmd.enabled=true;
    check(store.submit(cmd,1) && store.snapshot().config.lock_body_flips,"settings command failed");
    PresetLibrary presets(file.parent_path()/"presets");presets.save("Flip",cfg,false);check(presets.load("Flip").lock_body_flips,"preset omitted body-flip setting");presets.erase("Flip");
    std::cout<<"Body flip detector, direct head lock, smooth handoffs and persistence passed\n";
 } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; } }
