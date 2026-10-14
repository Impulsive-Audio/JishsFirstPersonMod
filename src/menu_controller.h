#pragma once
#include "mod.h"
namespace jishsfirstpersonmod {
struct MenuControllerResult { bool toggle{},ready{};int tab_delta{};GameplayInput navigation; };
class MenuControllerState {
    dingosdk::ControllerComboLatch shortcut_;
    unsigned device_{};
    std::uint32_t previous_{};
    bool neutral_{},shown_{};
public:
    MenuControllerResult update(std::uint32_t binding,GameplayInput input,bool shown,bool focused,bool recording) {
        MenuControllerResult result;
        result.toggle=shortcut_.update(binding,input,!focused || recording);
        if(!focused || !input.available || device_!=input.device || shown!=shown_) {
            neutral_=false;previous_=input.buttons;device_=input.device;shown_=shown;
        }
        if(result.toggle || recording || !shown || !focused) {neutral_=false;previous_=input.buttons;return result;}
        if(!neutral_) {
            neutral_=input.buttons==0 && std::hypot(input.left_x,input.left_y)<.25f;
            previous_=input.buttons;return result;
        }
        const auto pressed=input.buttons&~previous_;previous_=input.buttons;
        result.tab_delta=(pressed&0x200 ? 1:0)-(pressed&0x100 ? 1:0);
        result.ready=true;result.navigation=input;
        return result;
    }
};
}
