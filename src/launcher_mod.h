#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <stdexcept>
#include "External/rapidjson/document.h"
namespace jishsfirstpersonmod {
inline bool launcher_mod_enabled(const std::filesystem::path& mods) {
    // Keep older script-only installations working. New packages include this folder.
    if(!std::filesystem::is_directory(mods/"JishsFirstPersonMod")) return true;
    const auto file=mods/"mods.json";
    if(!std::filesystem::exists(file)) return true; // Unlisted folders default on in ReSkate.
    std::ifstream input(file,std::ios::binary);
    if(!input) throw std::runtime_error("Cannot read launcher mod settings");
    const auto size=std::filesystem::file_size(file);
    if(size>4*1024*1024) throw std::runtime_error("Launcher mod settings exceed size limit");
    std::string text(std::istreambuf_iterator<char>(input),{});
    if(text.starts_with("\xef\xbb\xbf")) text.erase(0,3);
    rapidjson::Document json;json.Parse(text.c_str());
    if(json.HasParseError() || !json.IsObject() || !json.HasMember("schema") || !json["schema"].IsInt() ||
       json["schema"].GetInt()!=1 || !json.HasMember("mods") || !json["mods"].IsArray())
        throw std::runtime_error("Invalid launcher mods.json");
    for(const auto& row:json["mods"].GetArray()) {
        if(!row.IsObject() || !row.HasMember("name") || !row["name"].IsString() ||
           !row.HasMember("enabled") || !row["enabled"].IsBool()) throw std::runtime_error("Invalid launcher mod row");
        std::string name=row["name"].GetString();
        for(auto& c:name) if(c>='A' && c<='Z') c+= 'a'-'A';
        if(name=="jishsfirstpersonmod") return row["enabled"].GetBool();
    }
    return true;
}
}
