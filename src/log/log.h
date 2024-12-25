

#pragma once

#include <cassert>
#include <functional>
#include <iostream>
#include <string>
#include <fstream>

#include <magic_enum/magic_enum.hpp>


#include "log_level.h"

#ifndef AFTER_HOURS_LOG_LEVEL 
#define AFTER_HOURS_LOG_LEVEL LogLevel::LOG_INFO
#endif

// TODO log to file

inline const std::string_view level_to_string(LogLevel level) {
    return magic_enum::enum_name(level);
}

inline void vlog(LogLevel level, const char* file, int line,
                 fmt::string_view format, fmt::format_args args) {
    if (level < AFTER_HOURS_LOG_LEVEL) return;
    auto file_info =
        fmt::format("{}: {}: {}: ", file, line, level_to_string(level));
    if (line == -1) {
        file_info = "";
    }
    const auto message = fmt::vformat(format, args);
    const auto full_output = fmt::format("{}{}", file_info, message);
    fmt::print("{}", full_output);
    fmt::print("\n");
}

template<typename... Args>
inline void log_me(LogLevel level, const char* file, int line,
                   const char* format, Args&&... args) {
    vlog(level, file, line, format,
         fmt::make_args_checked<Args...>(format, args...));
}

template<typename... Args>
inline void log_me(LogLevel level, const char* file, int line,
                   const wchar_t* format, Args&&... args) {
    vlog(level, file, line, format,
         fmt::make_args_checked<Args...>(format, args...));
}

template<>
inline void log_me(LogLevel level, const char* file, int line,
                   const char* format, const char*&& args) {
    vlog(level, file, line, format,
         fmt::make_args_checked<const char*>(format, args));
}

#include "log_macros.h"
