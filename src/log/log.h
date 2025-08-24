

#pragma once

#include <cassert>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>


#include <magic_enum/magic_enum.hpp>
#include <fmt/format.h>

#if defined(BUILT_WITH_XMAKE)
#else
#include <fmt/std.h>
#endif

#include "log_level.h"

#ifndef AFTER_HOURS_LOG_LEVEL
#define AFTER_HOURS_LOG_LEVEL LogLevel::LOG_INFO
#endif

// TODO allow people to specify their own colors

#ifdef AFTER_HOURS_LOG_WITH_COLOR
constexpr std::string_view color_reset = "\033[0m";
constexpr std::string_view color_red = "\033[31m";
constexpr std::string_view color_white = "\033[37m";
#else
constexpr std::string_view color_reset = "";
constexpr std::string_view color_red = "";
constexpr std::string_view color_white = "";
#endif

// TODO log to file

inline const std::string_view level_to_string(LogLevel level) {
  return magic_enum::enum_name(level);
}

inline void vlog(LogLevel level, const char *file, int line,
                 fmt::string_view format, fmt::format_args args) {
  if (level < AFTER_HOURS_LOG_LEVEL)
    return;
  auto file_info =
      fmt::format("{}: {}: {}: ", file, line, level_to_string(level));
  if (line == -1) {
    file_info = "";
  }

  const std::string_view color_start = level >= LogLevel::LOG_WARN ? color_red
                                                                   : color_white;

  const auto message = fmt::vformat(format, args);
  const auto full_output = fmt::format("{}{}", file_info, message);
  fmt::print("{}{}{}\n", color_start, full_output, color_reset);
}

template <typename... Args>
inline void log_me(LogLevel level, const char *file, int line,
                   fmt::format_string<Args...> format, Args const &...args) {
  vlog(level, file, line, format, fmt::make_format_args(args...));
}

// Thread-safe storage for log_once_per timing
namespace {
static std::unordered_map<std::string, std::chrono::steady_clock::time_point>
    log_once_per_timestamps;
static std::mutex log_once_per_mutex;
} // namespace

template <typename Level, typename... Args>
inline void log_once_per(std::chrono::milliseconds interval, Level level,
                         const char *file, int line,
                         fmt::format_string<Args...> format,
                         Args const &...args) {
  if (static_cast<int>(level) < static_cast<int>(AFTER_HOURS_LOG_LEVEL))
    return;
  std::string key = fmt::format("{}:{}", file, line);
  {
    std::lock_guard<std::mutex> lock(log_once_per_mutex);
    auto now = std::chrono::steady_clock::now();
    auto it = log_once_per_timestamps.find(key);
    if (it == log_once_per_timestamps.end() || (now - it->second) >= interval) {
      log_me(static_cast<LogLevel>(level), file, line, format, args...);
      log_once_per_timestamps[key] = now;
    }
  }
}

#include "log_macros.h"
