#pragma once

#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>

namespace utils {

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

// TODO merge with math_util.h
template <typename T> int sgn(const T &val) {
  return (T(0) < val) - (val < T(0));
}

std::string to_string(float value, int precision) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(precision) << value;
  return stream.str();
}

} // namespace utils
