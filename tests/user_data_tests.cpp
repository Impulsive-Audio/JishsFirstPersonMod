#include "user_data.h"
#include <iostream>
using namespace jishsfirstpersonmod;
void check(bool value,const char* text) {if(!value)throw std::runtime_error(text);}
std::string read(const std::filesystem::path& path) {std::ifstream in(path);return {std::istreambuf_iterator<char>(in),{}};}
int main() {
    namespace fs=std::filesystem;
    const auto root=fs::current_path()/"user-data-migration-tests";
    fs::remove_all(root);
    const auto old=root/"old",target=root/"scripts/Custom/FirstPerson";
    fs::create_directories(old/"presets");fs::create_directories(target/"presets");
    std::ofstream(old/"settings.ini")<<"legacy";
    std::ofstream(old/"presets/Existing.ini")<<"old";
    std::ofstream(old/"presets/Imported.ini")<<"imported";
    std::ofstream(old/"JishsFirstPersonMod.log")<<"history";
    std::ofstream(old/"bail-camera-trace-v11.csv.1")<<"trace";
    std::ofstream(target/"presets/Existing.ini")<<"current";
    check(scripts_data_directory(root/"scripts/Custom")==target,"data root not under scripts");
    migrate_user_data(old,target);
    check(read(target/"settings.ini")=="legacy","settings lost");
    check(read(target/"presets/Existing.ini")=="current","existing preset overwritten");
    check(read(target/"presets/Imported.ini")=="imported","preset missing");
    check(read(target/"logs/legacy-appdata/JishsFirstPersonMod.log")=="history","history missing");
    check(read(target/"logs/legacy-appdata/bail-camera-trace-v11.csv.1")=="trace","rotated trace missing");
    check(fs::exists(old/"settings.ini"),"legacy backup removed");
    fs::remove(target/"presets/Imported.ini");
    std::ofstream(target/"settings.ini")<<"updated";
    migrate_user_data(old,target);
    check(!fs::exists(target/"presets/Imported.ini"),"deleted preset resurrected");
    check(read(target/"settings.ini")=="updated","second migration overwrote settings");
    const auto fresh=root/"fresh";migrate_user_data(root/"absent",fresh);
    check(fs::is_directory(fresh/"logs") && fs::is_directory(fresh/"presets"),"fresh install folders missing");
    fs::remove_all(root);std::cout<<"User data migration and preservation passed\n";
}
