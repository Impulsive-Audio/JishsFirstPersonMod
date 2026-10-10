#include "settings.h"
#include "menu.h"
#include <sqlite3.h>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path module_directory() { return std::filesystem::current_path(); }
std::filesystem::path data_directory() { return module_directory(); }
}
void require(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
int main(int argc,char** argv) {
    using namespace jishsfirstpersonmod;
    const auto root=std::filesystem::current_path()/"menu-test-data";
    std::filesystem::create_directories(root);
    const auto file=root/"settings.ini";
    require(argc==2,"recommended preset fixture missing");
    const auto recommended=read_config(argv[1]);validate_config(recommended);
    require(recommended.toggle==770 && recommended.spring.enabled && recommended.spring.rotation[0]<-16 &&
        recommended.stick.yaw_speed==171 && recommended.stick.mount_smoothing==70 && recommended.stick.dismount_smoothing==35 &&
        !recommended.link_smoothing && recommended.third_person_bailing && recommended.disable_spring_on_foot && recommended.stick.lock_mount_jump && recommended.stick.lock_hippy_jump,
        "tested recommended settings changed");
    Config preset_initial;preset_initial.menu_key=VK_F10;
    SettingsStore presets(root/"preset-settings.ini",preset_initial);
    presets.refresh_reserved({0x20,0,0});
    Command preset;preset.kind=CommandKind::recommended_preset;preset.preset=recommended;
    require(presets.submit(preset,10),"recommended preset rejected");presets.flush_due(10);
    const auto loaded=read_config(root/"preset-settings.ini");
    require(loaded.toggle==recommended.toggle && loaded.spring.rotation==recommended.spring.rotation &&
        loaded.look_spring.left==28 && loaded.stick.yaw_speed==171 && loaded.reserved[0]==0x20 &&
        loaded.disable_spring_on_foot && loaded.stick.lock_mount_jump && loaded.stick.lock_hippy_jump && loaded.menu_key==VK_F10 && !presets.take_enabled() && !presets.snapshot().unsaved,"preset did not save or changed activation/reserved controls/menu key");
    presets.refresh_reserved({recommended.toggle,0,0});
    const auto before_preset=presets.snapshot().revision;
    require(presets.submit(preset,20) && presets.snapshot().revision>before_preset && presets.snapshot().conflicts.toggle,
        "native conflict prevented preset loading or was not flagged");
    presets.flush_due(20);
    require(read_config(root/"preset-settings.ini").stick.yaw_speed==171 && presets.snapshot().feedback.find("paused")!=std::string::npos,
        "conflicting preset settings were not saved or conflict feedback disappeared");
    NativeControls native;native.active=true;native.bindings[0]=recommended.look[0].combo;
    presets.refresh_native_controls(native);
    require(presets.submit(preset,25) && presets.snapshot().conflicts.glance[0] && !presets.snapshot().conflicts.glance[1],
        "native glance conflict rejected entire preset or paused other glances");
    preset.preset.stick.mount_smoothing=101;
    require(!presets.submit(preset,30),"invalid preset accepted");
    SettingsStore store(file,{});
    MenuHotkey menu_key;
    require(menu_key.toggle(WM_KEYDOWN,VK_F9,VK_F9,false) && menu_key.hidden,"menu key did not hide");
    menu_key.toggle(WM_KEYDOWN,VK_F9,VK_F9,true);require(menu_key.hidden,"key repeat toggled menu");
    menu_key.toggle(WM_KEYUP,VK_F9,VK_F9,false);require(menu_key.hidden,"key release toggled menu");
    menu_key.toggle(WM_KEYDOWN,VK_F9,VK_F9,false);require(!menu_key.hidden,"hidden panel could not reopen");
    require(!menu_key.toggle(WM_KEYDOWN,VK_INSERT,VK_F9,false),"menu key intercepted Insert");
    menu_key.recording=true;menu_key.key(WM_KEYDOWN,VK_SHIFT,false);
    require(menu_key.recording && !menu_key.recorded,"modifier bound alone");
    menu_key.key(WM_KEYDOWN,VK_F10,true);require(!menu_key.recorded,"held key recorded without new press");
    menu_key.key(WM_KEYDOWN,VK_ESCAPE,false);require(!menu_key.recording && !menu_key.recorded,"Escape failed to cancel key recording");
    menu_key.recording=true;menu_key.key(WM_KEYDOWN,VK_F10,false);
    require(menu_key.recorded==VK_F10 && !menu_key.recording,"keyboard key not recorded");
    Command menu_binding;menu_binding.kind=CommandKind::menu_key;menu_binding.action=VK_F10;
    require(store.submit(menu_binding,0),"menu binding rejected");store.flush_due(0);
    require(read_config(file).menu_key==VK_F10,"menu key did not persist");
    menu_binding.action=VK_INSERT;require(!store.submit(menu_binding,1),"reserved Insert binding accepted");
    menu_binding.action=0;require(store.submit(menu_binding,2),"clear menu key rejected");store.flush_due(2);
    require(read_config(file).menu_key==0,"clear menu key did not persist");
    Command bind; bind.kind=CommandKind::binding; bind.action=0; bind.mask=0x30;
    require(store.submit(bind,100),"toggle binding rejected"); store.flush_due(100);
    require(read_config(file).toggle==0x30 && !store.snapshot().unsaved,"binding not saved immediately");
    bind.action=1; require(!store.submit(bind,200),"duplicate controller chord accepted");
    bind.mask=0x100; require(store.submit(bind,200),"look binding rejected"); store.flush_due(200);
    const auto saved=read_config(file);
    Command angle; angle.kind=CommandKind::angle; angle.action=1; angle.value=45;
    require(store.submit(angle,300),"angle change rejected"); store.flush_due(1049);
    require(read_config(file).look[0].angle==saved.look[0].angle,"slider saved before debounce");
    store.flush_due(1050); require(read_config(file).look[0].angle==45,"slider not persisted");
    angle.value=91; require(!store.submit(angle,1100),"invalid angle accepted");
    Command camera; camera.kind=CommandKind::camera_settings; camera.value=97; camera.spring.enabled=true;
    camera.spring.offset={.123456789f,-.234567891f,.345678912f}; camera.spring.rotation={7,8,9};
    require(store.submit(camera,1200),"camera settings rejected"); store.flush_due(1950);
    const auto roundtrip=read_config(file);
    require(roundtrip.spring.offset==camera.spring.offset && roundtrip.spring.rotation==camera.spring.rotation && roundtrip.fov==97 && roundtrip.spring.enabled,"camera values lost in roundtrip");
    store.refresh_reserved({0x30,0,0}); require(store.snapshot().binding_conflict,"stock-side conflict not detected");
    bind.action=0; bind.mask=0; require(store.submit(bind,2000),"cannot clear stock conflict");
    require(!store.snapshot().binding_conflict,"stock conflict not resolved");
    Command enabled; enabled.kind=CommandKind::set_enabled; enabled.enabled=true;
    require(store.submit(enabled,2000) && store.snapshot().requested_enabled==true,"loading-time First Person request not visible");
    enabled.enabled=false;require(store.submit(enabled,2000) && store.snapshot().requested_enabled==false && store.take_enabled()==false,"queued enable could not be cancelled");
    enabled.enabled=true;
    require(store.submit(enabled,2000) && store.take_enabled()==true && !store.take_enabled(),"typed enable did not consume once");
    BindingRecorder recorder;
    dingosdk::ControllerInput pad{true,0x100,1};
    recorder.begin(2,0); require(!recorder.update(pad,true,true,1),"recorded opener without neutral");
    pad.buttons=0; recorder.update(pad,true,true,2);
    pad.buttons=0x100; recorder.update(pad,true,true,3); pad.buttons=0x300; recorder.update(pad,true,true,4);
    pad.buttons=0; require(recorder.update(pad,true,true,5)==0x300,"simultaneous chord not recorded");
    recorder.begin(1,10); recorder.update(pad,true,true,11); pad.buttons=1; recorder.update(pad,true,true,12);
    pad.buttons=2; recorder.update(pad,true,true,13); pad.buttons=0;
    require(recorder.update(pad,true,true,14)==1,"sequential buttons were unioned");
    for(unsigned interruption=0;interruption<5;++interruption) {
        recorder.begin(0,100); pad={true,0,1}; recorder.update(pad,true,true,101); pad.buttons=0x300;
        recorder.update(pad,true,true,102);
        if(interruption==0) recorder.update(pad,false,true,103);
        if(interruption==1) recorder.update(pad,true,false,103);
        if(interruption==2) { pad.available=false; recorder.update(pad,true,true,103); }
        if(interruption==3) { pad.device=2; recorder.update(pad,true,true,103); }
        if(interruption==4) recorder.update(pad,true,true,30100);
        require(!recorder.recording,"interruption left capture active");
        pad={true,0,1}; require(!recorder.update(pad,true,true,30101),"cancelled capture committed");
    }
    PointerRouter input; input.visible=true; input.bounds={100,100,400,400};
    require(input.route(WM_INPUT,0,120,120),"raw input over panel reached stock button reader");
    require(!input.route(WM_INPUT,0,500,500),"raw input outside panel lost");
    require(input.route(WM_LBUTTONDOWN,0,120,120),"click in panel reached stock");
    require(input.route(WM_LBUTTONUP,0,500,500),"drag release leaked to stock");
    require(input.route(WM_CHAR,'1',0,0),"focused numeric entry reached stock");
    require(!input.route(WM_KEYDOWN,VK_INSERT,0,0) && !input.route(WM_KEYDOWN,VK_ESCAPE,0,0),"stock menu keys swallowed");
    require(!input.route(WM_LBUTTONDOWN,0,500,500) && !input.route(WM_CHAR,'1',0,0),"stock controls lost input");
    input.visible=false; require(!input.route(WM_LBUTTONDOWN,0,120,120),"hidden panel swallowed input");
    const auto blocked=root/"blocked"; { std::ofstream out(blocked); out<<"file"; }
    SettingsStore failure(blocked/"settings.ini",{}); bind.mask=1;
    require(failure.submit(bind,0),"valid edit failed before storage attempt"); failure.flush_due(0);
    require(failure.snapshot().unsaved && failure.snapshot().feedback.starts_with("Applied, but not saved"),"save failure not shown");
    const auto db_path=root/"profile.sqlite3";
    sqlite3* db{}; require(sqlite3_open(db_path.string().c_str(),&db)==SQLITE_OK,"fixture DB open failed");
    const char* sql="DROP TABLE IF EXISTS player_settings; CREATE TABLE player_settings(scope TEXT,name TEXT,value_json TEXT);"
        "INSERT INTO player_settings VALUES('/settings/bindings','first_person','48'),('/settings/bindings','look_left','256'),"
        "('/settings/first_person_look_angles','look_left','120'),('options','ReSkate.FirstPerson.Spring','true'),"
        "('/settings/gameplay','ReSkate.FirstPerson.Fov','95'),('/settings/gameplay','ReSkate.FirstPerson.Yaw','7');";
    require(sqlite3_exec(db,sql,nullptr,nullptr,nullptr)==SQLITE_OK,"fixture SQL failed"); sqlite3_close(db);
    const auto bytes=[&] { std::ifstream in(db_path,std::ios::binary); return std::string(std::istreambuf_iterator<char>(in),{}); };
    const auto original=bytes(); const auto migrated=import_profile(db_path);
    require(migrated.toggle==48 && migrated.look[2].combo==256 && migrated.look[2].angle==120 && migrated.spring.enabled && migrated.fov==95 && migrated.spring.rotation[1]==7,"native profile migration mismatch");
    require(bytes()==original,"profile database was changed");
    const auto legacy=root/"legacy"; std::filesystem::create_directories(legacy);
    write_config(legacy/"MyMod.ini",migrated);
    const auto initial=load_initial_settings(root/"new-settings.ini",legacy,db_path);
    require(initial.config.toggle==48 && initial.save,"legacy migration not automatic");
    const auto existing=load_initial_settings(file,legacy,db_path);
    require(existing.config.toggle==0x30 && !existing.save,"existing settings were overwritten by migration");
    std::cout<<"Menu recording, routing, persistence, conflicts, typed requests and native migration passed\n";
}
