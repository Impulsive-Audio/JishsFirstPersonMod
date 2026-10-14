#pragma once
#include <filesystem>
#include <fstream>
#include <stdexcept>
namespace jishsfirstpersonmod {
inline std::filesystem::path scripts_data_directory(const std::filesystem::path& companion_directory) {
    return companion_directory/L"FirstPerson";
}
inline void migrate_user_data(const std::filesystem::path& legacy,const std::filesystem::path& target) {
    namespace fs=std::filesystem;
    fs::create_directories(target/L"logs");
    fs::create_directories(target/L"presets");
    const auto stamp=target/L"appdata-migration-v1.done";
    if(fs::exists(stamp)) return;
    const auto copy_missing=[](const fs::path& from,const fs::path& to) {
        if(fs::is_symlink(from) || !fs::is_regular_file(from) || fs::exists(to)) return;
        fs::create_directories(to.parent_path());fs::copy_file(from,to,fs::copy_options::none);
    };
    if(fs::exists(legacy) && !fs::is_symlink(legacy)) {
        copy_missing(legacy/L"settings.ini",target/L"settings.ini");
        const auto presets=legacy/L"presets";
        if(fs::is_directory(presets) && !fs::is_symlink(presets))
            for(const auto& file:fs::directory_iterator(presets))
                if(file.path().extension()==L".ini") copy_missing(file.path(),target/L"presets"/file.path().filename());
        // Keep historical logs separate from the new session and its rotations.
        for(const auto& file:fs::directory_iterator(legacy)) {
            const auto name=file.path().filename().wstring();
            if(name==L"JishsFirstPersonMod.log" || name.starts_with(L"JishsFirstPersonMod.log.") ||
               name==L"proxy-loader.log" || name.starts_with(L"proxy-loader.log.") ||
               name.starts_with(L"bail-camera-trace-"))
                copy_missing(file.path(),target/L"logs/legacy-appdata"/file.path().filename());
        }
    }
    std::ofstream marker(stamp);marker<<"Existing files preserved; legacy AppData retained as backup.\n";
    marker.close();if(!marker) throw std::runtime_error("Could not finish user-data migration.");
}
}
