#pragma once

#include <string>
#include <ostream>
#include <iomanip>

namespace utils {

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

std::string to_string(float value, int precision) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

} // namespace utils
