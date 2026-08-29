#pragma once

#define log_trace(...)                                                         \
  if (static_cast<int>(LogLevel::LOG_TRACE) >=                                 \
      static_cast<int>(AFTER_HOURS_LOG_LEVEL))                                 \
  log_me(LogLevel::LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)

#define log_info(...)                                                          \
  if (static_cast<int>(LogLevel::LOG_INFO) >=                                  \
      static_cast<int>(AFTER_HOURS_LOG_LEVEL))                                 \
  log_me(LogLevel::LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)                                                          \
  if (static_cast<int>(LogLevel::LOG_WARN) >=                                  \
      static_cast<int>(AFTER_HOURS_LOG_LEVEL))                                 \
  log_me(LogLevel::LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
// do/while(0) so the assert stays attached. Without it the assert is a
// separate statement, so `if (x) log_error(...);` aborts whatever x is --
// which is how afterhours' e2e runner started aborting on every command.
#define log_error(...)                                                         \
  do {                                                                         \
    if (static_cast<int>(LogLevel::LOG_ERROR) >=                               \
        static_cast<int>(AFTER_HOURS_LOG_LEVEL))                               \
      log_me(LogLevel::LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__);            \
    assert(false);                                                             \
  } while (0)

#define log_clean(level, ...)                                                  \
  if (static_cast<int>(level) >= static_cast<int>(AFTER_HOURS_LOG_LEVEL))      \
    log_me(level, "", -1, __VA_ARGS__);

#define log_if(x, ...)                                                         \
  {                                                                            \
    if (x)                                                                     \
      log_me(LogLevel::LOG_IF, __FILE__, __LINE__, __VA_ARGS__);               \
  }

#define log_ifx(x, level, ...)                                                 \
  {                                                                            \
    if (x)                                                                     \
      log_me(level, __FILE__, __LINE__, __VA_ARGS__);                          \
  }

#define log_once_per(interval, level, ...)                                     \
  project_log_once_per(interval, level, __FILE__, __LINE__, __VA_ARGS__)
