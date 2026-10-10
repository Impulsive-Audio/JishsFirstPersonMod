#include "launcher_mod.h"
#include <iostream>
int main() {
    using namespace jishsfirstpersonmod;
    const auto root=std::filesystem::current_path()/"launcher-mod-test-data";
    std::filesystem::create_directories(root/"JishsFirstPersonMod");
    const auto write=[&](const char* text) { std::ofstream file(root/"mods.json");file<<text; };
    const auto check=[](bool ok) { if(!ok) throw std::runtime_error("Launcher startup switch regression"); };
    write(R"({"schema":1,"mods":[]})");check(launcher_mod_enabled(root));
    write(R"({"schema":1,"mods":[{"name":"JishsFirstPersonMod","enabled":false}]})");check(!launcher_mod_enabled(root));
    write(R"({"schema":1,"mods":[{"name":"JISHSFIRSTPERSONMOD","enabled":true}]})");check(launcher_mod_enabled(root));
    write(R"({"schema":1,"mods":[{"name":"OtherMod","enabled":false}]})");check(launcher_mod_enabled(root));
    write("broken");bool rejected{};try { launcher_mod_enabled(root); } catch(...) { rejected=true; }check(rejected);
    std::filesystem::remove(root/"mods.json");check(launcher_mod_enabled(root));
    check(launcher_mod_enabled(root/"legacy-install"));
    std::cout<<"Launcher enable/disable, unlisted defaults, case matching, malformed settings and legacy installs passed\n";
}
