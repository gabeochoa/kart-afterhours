#pragma once

#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>

namespace utils {

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

std::string to_string(float value, int precision) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(precision) << value;
  return stream.str();
}

} // namespace utils
