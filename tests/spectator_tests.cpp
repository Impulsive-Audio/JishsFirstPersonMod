#include "spectator_view.h"
#include "spectator_head.h"
#include "settings.h"
#include <map>
#include <vector>
#include <fstream>
#include <iostream>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path module_directory() { return std::filesystem::current_path(); }
std::filesystem::path data_directory() { return module_directory(); }
}
using namespace jishsfirstpersonmod;
using namespace dingosdk::first_person;
void check(bool yes,const char* why) { if(!yes)throw std::runtime_error(why); }
struct MockMemory {
    std::map<std::uintptr_t,std::vector<unsigned char>> regions;
    void block(std::uintptr_t address,std::size_t size) { regions[address].resize(size); }
    bool copy(std::uintptr_t address,void* out,std::size_t size,bool writing=false) {
        auto it=regions.upper_bound(address);if(it==regions.begin())return false;--it;
        const auto offset=address-it->first;
        if(offset>it->second.size() || size>it->second.size()-offset)return false;
        if(writing)std::memcpy(it->second.data()+offset,out,size);else std::memcpy(out,it->second.data()+offset,size);
        return true;
    }
    template<class T> void put(std::uintptr_t address,T value) { check(copy(address,&value,sizeof(value),true),"mock write outside bounds"); }
    void code(std::uintptr_t address,std::string_view hex) {
        auto nibble=[](char c){return c<='9'?c-'0':c-'a'+10;};
        for(std::size_t i=0;i<hex.size()/2;++i)put(address+i,static_cast<unsigned char>(16*nibble(hex[2*i])+nibble(hex[2*i+1])));
    }
};
Matrix pose(float angle,Vec3 position={}) { Matrix m{};const auto a=angle*std::numbers::pi_v<float>/360;write(m,{std::sin(a),0,0,std::cos(a)},position);return m; }
bool poses_close(const Matrix& a,const Matrix& b) { for(unsigned i=0;i<16;++i)if(std::abs(a[i]-b[i])>1e-5f)return false;return true; }
int main() { try {
    constexpr std::uintptr_t stock=0x10000000,party=0x20000000,published=0x21000000,remotes=0x22000000,
        component=0x23000000,holder=0x24000000,context=0x25000000,entity=0x26000000,buffer=0x27000000;
    MockMemory m;m.block(stock,spectator::party_pointer+0x1000);m.block(party,0xc000);m.block(published,8+spectator::players*spectator::record_stride);
    m.block(remotes,spectator::remote_stride*249);m.block(component,0x200);m.block(buffer,395*0x30);
    auto read=[&](std::uintptr_t address,void* out,std::size_t size){return m.copy(address,out,size);};
    auto write_scale=[&](std::uintptr_t address,std::array<float,3> scale){return m.copy(address,&scale,sizeof(scale),true);};
    m.code(stock+spectator::party_getter,spectator::party_signature);m.code(stock+spectator::remote_getter,spectator::remote_signature);
    m.code(stock+spectator::render_hook,spectator::render_signature);m.code(stock+spectator::party_transform_return-17,spectator::transform_signature);
    check(spectator::verify_contract(read,stock),"valid bridge rejected");
    m.put(stock+spectator::party_getter,static_cast<unsigned char>(0x90));check(!spectator::verify_contract(read,stock),"changed bridge accepted");
    m.code(stock+spectator::party_getter,spectator::party_signature);
    m.put(stock+spectator::party_pointer,party);m.put(stock+spectator::remote_pointer,remotes);
    m.put(party+spectator::published_offset,published);m.put(party+spectator::selected_offset,std::uint64_t{42});m.put(party+spectator::epoch_offset,std::uint64_t{7});
    constexpr auto record=published+8+3*spectator::record_stride;
    m.put(record,std::uint64_t{42});m.put(record+8,std::uint64_t{7});m.put(record+50,static_cast<unsigned char>(1));m.put(record+56,std::size_t{2});m.put(record+88,std::uint64_t{88});
    constexpr auto position=party+192+2*spectator::position_stride,remote=remotes+2*spectator::remote_stride;
    m.put(position,static_cast<unsigned char>(1));m.put(position+8,std::uint64_t{1000});
    m.put(remote,entity);m.put(remote+8,component);m.put(remote+24,context);m.put(remote+256,buffer);m.put(remote+264,std::uint64_t{1});m.put(component+0xa0,holder);
    std::string reason;auto target=spectator::read_target(read,stock,context,1000,reason);
    check(target && target->identity.id==42 && target->identity.slot==2 && spectator::same_actor(read,*target),"selected player identity failed");
    check(!spectator::read_target(read,stock,context+8,1000,reason),"wrong world accepted");
    check(spectator::read_target(read,stock,context,1250,reason).has_value(),"250ms boundary rejected");
    check(!spectator::read_target(read,stock,context,1251,reason),"stale pose accepted");
    check(!spectator::read_target(read,stock,context,999,reason),"future timestamp accepted");
    m.put(record+8,std::uint64_t{8});check(!spectator::read_target(read,stock,context,1000,reason),"rejoined player accepted");m.put(record+8,std::uint64_t{7});
    m.put(record+48,static_cast<unsigned char>(1));check(!spectator::read_target(read,stock,context,1000,reason),"local player accepted");m.put(record+48,static_cast<unsigned char>(0));
    m.put(record+56,std::size_t{249});check(!spectator::read_target(read,stock,context,1000,reason),"invalid slot accepted");m.put(record+56,std::size_t{2});
    m.put(published+8,std::uint64_t{42});check(!spectator::read_target(read,stock,context,1000,reason),"ambiguous ID accepted");m.put(published+8,std::uint64_t{0});
    m.put(remote+264,std::uint64_t{2});check(!spectator::same_actor(read,*target),"reused actor accepted");m.put(remote+264,std::uint64_t{1});
    auto racing_read=[&](std::uintptr_t address,void* out,std::size_t size) {
        const bool ok=read(address,out,size);
        if(address==remote+264)m.put(party+spectator::selected_offset,std::uint64_t{43});return ok;
    };
    check(!spectator::read_target(racing_read,stock,context,1000,reason),"target change during capture accepted");m.put(party+spectator::selected_offset,std::uint64_t{42});
    auto failed_read=[&](std::uintptr_t address,void* out,std::size_t size) {return address!=published+8 && read(address,out,size);};
    check(!spectator::read_target(failed_read,stock,context,1000,reason),"failed capture read accepted");
    const std::array<float,3> real{1,1.1f,.9f};m.put(buffer+103*0x30ULL,real);
    struct Pose { std::uintptr_t buffer;unsigned count; };
    auto rendered=[&](std::uintptr_t){return Pose{buffer,395};};
    spectator::HiddenHead hidden;
    check(hidden.hide(read,write_scale,rendered,*target,buffer) && hidden.owned(),"head hide failed");
    std::array<float,3> scale{};read(buffer+103*0x30ULL,&scale,sizeof(scale));check(scale==std::array<float,3>{.001f,.001f,.001f},"render head not hidden");
    hidden.restore(read,write_scale,rendered);read(buffer+103*0x30ULL,&scale,sizeof(scale));check(scale==real && !hidden.owned(),"head scale not restored");
    hidden.hide(read,write_scale,rendered,*target,buffer);m.put(remote+264,std::uint64_t{2});
    hidden.restore(read,write_scale,rendered);read(buffer+103*0x30ULL,&scale,sizeof(scale));check(scale!=real && !hidden.owned(),"destroyed actor buffer overwritten");
    m.put(remote+264,std::uint64_t{1});m.put(buffer+103*0x30ULL,real);
    hidden.hide(read,write_scale,rendered,*target,buffer);auto replaced=[&](std::uintptr_t){return Pose{buffer+0x10,395};};
    hidden.restore(read,write_scale,replaced);read(buffer+103*0x30ULL,&scale,sizeof(scale));check(scale!=real && hidden.owned(),"unpublished buffer was written or its restore lease was lost");
    hidden.restore(read,write_scale,rendered);read(buffer+103*0x30ULL,&scale,sizeof(scale));check(scale==real && !hidden.owned(),"returning pose buffer kept hidden head scale");
    check(!hidden.hide(read,[](auto,auto){return false;},rendered,*target,buffer),"failed hide write acquired lease");
    spectator::View view;auto normal=pose(0,{0,.2f,0}),head=pose(80,{0,0,0});
    view.observe(true,true,context);check(!view.preferred(),"session did not start in follow view");
    view.apply(normal,head,target->identity,2);check(view.fov(70,90,2)==70,"follow FOV changed");check(view.toggle(),"session toggle rejected");
    auto entry=view.apply(normal,head,target->identity,2);
    check(length(subtract(body_flip::position(entry),body_flip::position(normal)))<1e-5,"entry snapped");
    auto moving=pose(155,{1,1,1});auto mid=view.apply(normal,moving,target->identity,2.125);
    check(std::abs(length(subtract(body_flip::position(mid),body_flip::position(moving)))-.1f)<1e-5,"moving head pulled away during handoff");
    check(std::abs(view.fov(70,90,2.125)-80)<1e-5,"FOV entry blend failed");
    auto locked=view.apply(normal,moving,target->identity,2.25);check(locked==moving,"locked pose is not direct");check(view.fov(70,90,2.25)==90,"head FOV failed");
    check(view.toggle(),"exit toggle failed");auto exit=view.apply(normal,moving,target->identity,2.26);check(poses_close(exit,locked),"exit snapped");
    auto followed=view.apply(normal,moving,target->identity,2.51);check(poses_close(followed,normal) && view.fov(70,90,2.51)==70,"follow camera/FOV not restored");
    view.toggle();view.apply(normal,head,target->identity,2.52);view.apply(normal,head,target->identity,2.77);
    auto changed=target->identity;changed.id=43;auto new_view=view.apply(normal,head,changed,2.78);
    check(view.preferred() && poses_close(new_view,normal),"target switch lost preference or reused stale pose");
    view.apply(normal,head,changed,3.03);check(poses_close(view.apply(normal,head,changed,3.281),normal),"sample gap kept stale view");
    auto teleported=pose(80,{20,0,0});check(poses_close(view.apply(normal,teleported,changed,3.29),normal),"teleport kept stale view");
    view.observe(true,true,context,true);check(view.preferred() && !view.owns_camera(),"focus/menu interruption did not discard history");
    view.observe(false,true,context);check(!view.preferred() && !view.toggle(),"session end retained spectator mode");
    view.observe(true,true,context);check(!view.preferred(),"new session reused view choice");view.toggle();view.observe(true,false,context);check(!view.toggle(),"disabled mode consumed input");
    view.observe(true,true,context);view.toggle();view.observe(true,true,context+8);check(!view.preferred(),"world change retained view choice");
    Config cfg;check(!cfg.spectator_first_person,"default must be off");
    const auto directory=std::filesystem::current_path()/("spectator-test-settings-"+std::to_string(GetCurrentProcessId()));std::filesystem::create_directories(directory);
    const auto file=directory/"settings.ini";cfg.spectator_first_person=true;cfg.menu_key=VK_F8;cfg.look_only=true;cfg.cycle_restore_pending=true;cfg.cycle_restore_enabled=true;
    write_config(file,cfg);check(read_config(file).spectator_first_person,"setting round trip failed");
    const auto older=directory/"older.ini";{std::ofstream out(older);out<<"[JishsFirstPersonMod]\n";}check(!read_config(older).spectator_first_person,"older file default changed");
    SettingsStore settings(file,cfg);Command setting;setting.kind=CommandKind::spectator_first_person;setting.enabled=false;check(settings.submit(setting,1),"setting command failed");
    setting.enabled=true;settings.submit(setting,2);
    PresetLibrary presets(directory/"presets");presets.save("Spectator",cfg,false);check(presets.load("Spectator").spectator_first_person,"preset omitted option");
    const auto before=settings.snapshot().config;view.observe(true,true,context);view.toggle();view.toggle();
    check(settings.snapshot().config.menu_key==before.menu_key && settings.snapshot().config.look_only==before.look_only &&
        settings.snapshot().config.cycle_restore_pending==before.cycle_restore_pending,"view toggle modified settings");
    {std::ofstream out(older);out<<"spectator_first_person=2\n";}bool invalid=false;try{read_config(older);}catch(...){invalid=true;}check(invalid,"malformed option accepted");
    std::filesystem::remove_all(directory);
    std::cout<<"Spectator contracts, identity, render visibility, handoffs, FOV, lifecycle and persistence passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;} }
