
#pragma once

#include <stdio.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <math.h>
#include <memory>
#include <numeric>
#include <optional>
#include <ostream>
#include <random>
#include <set>
#include <source_location>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#if !defined(M_PI)
#define M_PI 3.14159265358979323846f
#endif

#include <fmt/format.h>

namespace afterhours { namespace ui { enum struct Dim; enum struct Axis; } }

template <> struct fmt::formatter<afterhours::ui::Dim> : fmt::formatter<string_view> {
  template <typename FormatContext>
  auto format(afterhours::ui::Dim d, FormatContext &ctx) const {
    using afterhours::ui::Dim;
    string_view name = "None";
    switch (d) {
      case Dim::None: name = "None"; break;
      case Dim::Pixels: name = "Pixels"; break;
      case Dim::Text: name = "Text"; break;
      case Dim::Percent: name = "Percent"; break;
      case Dim::Children: name = "Children"; break;
      case Dim::ScreenPercent: name = "ScreenPercent"; break;
    }
    return fmt::formatter<string_view>::format(name, ctx);
  }
};

template <> struct fmt::formatter<afterhours::ui::Axis> : fmt::formatter<string_view> {
  template <typename FormatContext>
  auto format(afterhours::ui::Axis a, FormatContext &ctx) const {
    using afterhours::ui::Axis;
    string_view name = "X";
    switch (a) {
      case Axis::X: name = "X"; break;
      case Axis::Y: name = "Y"; break;
      case Axis::left: name = "left"; break;
      case Axis::top: name = "top"; break;
      case Axis::right: name = "right"; break;
      case Axis::bottom: name = "bottom"; break;
    }
    return fmt::formatter<string_view>::format(name, ctx);
  }
};