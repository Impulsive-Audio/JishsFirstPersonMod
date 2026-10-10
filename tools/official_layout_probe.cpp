#include "Extension/UI/Overlay/overlay_internal.h"
#include <iostream>
#include <cstddef>
int main() {
    using namespace dingosdk::overlay;
    static_assert(offsetof(dingosdk::overlay::detail::State,callbacks)==0x4b8);
    static_assert(offsetof(dingosdk::overlay::detail::State,model)==0x4e8);
    static_assert(offsetof(Model,debug)==128);
    static_assert(offsetof(Model,bindings)==0x5f50);
    static_assert(offsetof(dingosdk::ControllerBindingsModel,action_combos)==40);
    static_assert(offsetof(dingosdk::ControllerBindingsModel,status)==72);
    static_assert(offsetof(dingosdk::first_person::Settings,board_only)==46);
    static_assert(offsetof(DebugModel,camera_unavailable)==0xa0);
    static_assert(offsetof(DebugModel,status)==0x1a8);
    static_assert(offsetof(DebugModel,first_person_arm)==16);
    static_assert(offsetof(dingosdk::first_person::Settings,stabilize)==44);
    static_assert(sizeof(DebugRequest)==12 && offsetof(DebugRequest,value)==8);
    static_assert(sizeof(CallbacksV3)==48 && offsetof(CallbacksV3,read_model)==8 && offsetof(CallbacksV3,queue_debug)==24);
    static_assert(int(DebugAction::set_first_person)==13 && int(DebugAction::set_first_person_spring_right)==25 && int(DebugAction::reset_first_person_arm)==33);
    std::cout << "callbacks=" << offsetof(dingosdk::overlay::detail::State,callbacks) << '\n';
    std::cout << "model=" << offsetof(dingosdk::overlay::detail::State,model) << '\n';
    std::cout << "debug=" << offsetof(Model,debug) << '\n';
    std::cout << "issue=" << offsetof(DebugModel,camera_unavailable) << '\n';
    std::cout << "status=" << offsetof(DebugModel,status) << '\n';
    std::cout << "arm=" << offsetof(DebugModel,first_person_arm) << '\n';
    std::cout << "arm_size=" << sizeof(dingosdk::first_person::Settings) << '\n';
    std::cout << "fp=" << int(DebugAction::set_first_person) << " reset=" << int(DebugAction::reset_first_person_arm) << '\n';
}
