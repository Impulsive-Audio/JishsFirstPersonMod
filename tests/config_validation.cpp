#include "mod.h"
#include <fstream>
#include <stdexcept>
int main(int argc,char** argv) {
    if (argc!=2) return 1;
    const auto defaults=jishsfirstpersonmod::read_config(argv[1]);
    if (defaults.toggle || defaults.fov || defaults.spring.enabled || defaults.look[2].angle!=90) return 2;
    const auto path=std::filesystem::current_path()/L"config-test.ini";
    const auto reject=[&](const char* text) {
        { std::ofstream out(path); out<<text; }
        bool rejected{};
        try { (void)jishsfirstpersonmod::read_config(path); } catch (...) { rejected=true; }
        if (!rejected) throw std::runtime_error("invalid configuration accepted");
    };
    for (const auto text : {"first_person=-1","first_person=0x40000","first_person=0x100000000",
        "first_person=1.5","look_up=1\nlook_down=1",
        "look_up_angle=0","look_up_angle=91",
        "look_left_angle=181","look_up_angle=nan","fov=39","fov=121","spring=2",
        "offset_x=4","pitch=181","strength_up=101","strength_down=-1","typo=1",
        "spring=0\nspring=1","disable_spring_on_foot=2","disable_spring_on_foot=-1","lock_camera_mount_jump=2","lock_camera_hippy_jump=2","toggle_glances=2","toggle_glances=-1"}) reject(text);
    { std::ofstream out(path); out<<"first_person=0x30\nlook_up=0x100\nlook_up_angle=45\nspring=1\nfov=90"; }
    const auto cfg=jishsfirstpersonmod::read_config(path);
    std::filesystem::remove(path);
    return cfg.toggle==0x30 && cfg.look[0].combo==0x100 && cfg.look[0].angle==45 && cfg.spring.enabled && cfg.fov==90 && !cfg.toggle_glances ? 0:3;
}
