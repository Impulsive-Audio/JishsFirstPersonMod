#pragma once
#include "mod.h"
#include <cstdio>
namespace dingosdk::logging {
enum class Level { info, warning, error };
enum class Channel { input };
inline void write(Level, Channel, const char* text) { jishsfirstpersonmod::log(text); }
template<class... A> void printf(Level, Channel, const char* format, A... args) {
    char text[512]{}; std::snprintf(text, sizeof(text), format, args...); jishsfirstpersonmod::log(text);
}
}
