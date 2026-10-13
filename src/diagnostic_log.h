#pragma once
#include "rotating_log.h"
#include <Windows.h>
#include <mutex>
namespace jishsfirstpersonmod {
inline thread_local bool diagnostic_file_io{};
struct DiagnosticFileIo {
    bool previous=diagnostic_file_io;
    DiagnosticFileIo(){diagnostic_file_io=true;}
    ~DiagnosticFileIo(){diagnostic_file_io=previous;}
};
inline void write_diagnostic_log(const std::filesystem::path& dir,const std::string& text) noexcept {
    try {
        DiagnosticFileIo io;
        static auto* mutex=new std::mutex;
        std::lock_guard lock(*mutex);
        std::filesystem::create_directories(dir);
        append_rotating_log(dir/L"JishsFirstPersonMod.log",
            std::to_string(GetTickCount64())+" [JishsFirstPersonMod] "+text+'\n',text_log_limit);
        OutputDebugStringA(("[JishsFirstPersonMod] "+text+"\n").c_str());
    } catch(...) {}
}
}
