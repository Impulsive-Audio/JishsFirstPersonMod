#include "menu_controller.h"
#include "settings.h"
#include <stdexcept>
#include <iostream>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path module_directory(){return std::filesystem::current_path()/"controller-menu-tests";}
std::filesystem::path data_directory(){return module_directory();}
}
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
int main(){try {
    using namespace jishsfirstpersonmod;
    MenuControllerState state;GameplayInput pad;pad.available=true;pad.device=1;
    check(!state.update(0x30,pad,false,true,false).toggle,"neutral toggled menu");
    pad.buttons=0x30;check(state.update(0x30,pad,false,true,false).toggle,"hidden menu shortcut failed");
    check(!state.update(0x30,pad,true,true,false).toggle,"held shortcut repeated");
    check(!state.update(0x30,pad,true,true,false).ready,"opening chord leaked to navigation");
    pad.buttons=0;state.update(0x30,pad,true,true,false);
    pad.buttons=0x100;auto frame=state.update(0x30,pad,true,true,false);
    check(frame.ready && frame.tab_delta==-1,"LB did not switch tab");
    check(state.update(0x30,pad,true,true,false).tab_delta==0,"held shoulder repeated tab change");
    pad.buttons=0x200;check(state.update(0x30,pad,true,true,false).tab_delta==1,"RB did not switch tab");
    pad.buttons=1;pad.left_x=.8f;frame=state.update(0x30,pad,true,true,false);
    check(frame.navigation.buttons==1 && frame.navigation.left_x==.8f,"D-pad/analog navigation missing");
    check(!state.update(0x30,pad,true,false,false).ready,"unfocused menu accepted controls");
    pad.buttons=0x30;check(!state.update(0x30,pad,true,true,false).toggle,"focus return accepted held shortcut");
    pad.buttons=0;pad.left_x=0;state.update(0x30,pad,true,true,false);
    pad.buttons=0x30;check(state.update(0x30,pad,true,true,false).toggle,"close shortcut failed");
    pad.buttons=0;state.update(0x30,pad,true,true,true);pad.buttons=0x30;
    frame=state.update(0x30,pad,true,true,true);check(!frame.toggle && !frame.ready,"recording chord activated menu or widgets");
    pad.device=2;check(!state.update(0x30,pad,true,true,false).toggle,"new controller resumed held shortcut");
    Config config;config.menu_combo=0x10030;auto path=data_directory()/"settings.ini";
    write_config(path,config);check(read_config(path).menu_combo==0x10030,"trigger menu shortcut did not save");
    SettingsStore store(path,{});Command command;command.kind=CommandKind::binding;command.action=6;command.mask=0x30;
    check(store.submit(command,1) && store.snapshot().config.menu_combo==0x30,"menu shortcut recorder command failed");
    command.kind=CommandKind::apply_preset;command.preset={};
    check(store.submit(command,2) && store.snapshot().config.menu_combo==0x30,"camera preset replaced global menu access");
    std::cout<<"Hidden/open shortcuts, navigation, tabs, focus, recording, reconnect and persistence passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
