

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

  const std::string_view color_start = level >= LogLevel::LOG_WARN //
                                           ? color_red
                                           : color_white;

  const auto message = fmt::vformat(format, args);
  const auto full_output = fmt::format("{}{}", file_info, message);
  fmt::print("{}{}{}", color_start, full_output, color_reset);
  fmt::print("\n");
}

template <typename... Args>
inline void log_me(LogLevel level, const char *file, int line,
                   const char *format, Args &&...args) {
  vlog(level, file, line, format,
       fmt::make_args_checked<Args...>(format, args...));
}

template <typename... Args>
inline void log_me(LogLevel level, const char *file, int line,
                   const wchar_t *format, Args &&...args) {
  vlog(level, file, line, format,
       fmt::make_args_checked<Args...>(format, args...));
}

template <>
inline void log_me(LogLevel level, const char *file, int line,
                   const char *format, const char *&&args) {
  vlog(level, file, line, format,
       fmt::make_args_checked<const char *>(format, args));
}

// Thread-safe storage for log_once_per timing
namespace {
static std::unordered_map<std::string, std::chrono::steady_clock::time_point>
    log_once_per_timestamps;
static std::mutex log_once_per_mutex;
} // namespace

template <typename Level, typename... Args>
inline void log_once_per(std::chrono::milliseconds interval, Level level,
                         const char *file, int line, const char *format,
                         Args &&...args) {
  if (static_cast<int>(level) < static_cast<int>(AFTER_HOURS_LOG_LEVEL))
    return;
  // Create a unique key for this log message
  std::string key = fmt::format("{}:{}:{}", file, line, format);
  {
    std::lock_guard<std::mutex> lock(log_once_per_mutex);
    auto now = std::chrono::steady_clock::now();
    auto it = log_once_per_timestamps.find(key);
    if (it == log_once_per_timestamps.end() || (now - it->second) >= interval) {
      // Log the message and update timestamp
      log_me(static_cast<LogLevel>(level), file, line, format,
             std::forward<Args>(args)...);
      log_once_per_timestamps[key] = now;
    }
  }
}

#include "log_macros.h"
