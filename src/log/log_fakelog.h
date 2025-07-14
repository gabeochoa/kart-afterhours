
#pragma once

#include "log_level.h"
#include <iostream>
inline void log_me() { std::cout << std::endl; }
template <typename T, typename... Args>
inline void log_me(const T &arg, const Args &...args) {
  std::cout << arg << " ";
  log_me(args...);
}
#include "log_macros.h"
