#pragma once

// fmt 9 dropped implicit enum formatting, and afterhours' autolayout.h logs a
// ui::Axis through our own log_error macro. A formatter specialization has to
// be visible before that point of instantiation, so this pulls layout_types.h
// in directly rather than waiting for rl.h to reach autolayout.h.
//
// Lives here instead of in afterhours because the submodule is shared with the
// other ~10 projects, which are still on fmt 8.

#include <afterhours/src/plugins/ui/layout_types.h>
#include <fmt/format.h>
#include <string_view>

template <>
struct fmt::formatter<afterhours::ui::Axis> : fmt::formatter<std::string_view> {
  auto format(afterhours::ui::Axis axis, format_context &ctx) const {
    std::string_view name = "Unknown";
    switch (axis) {
    case afterhours::ui::Axis::X:
      name = "X-Axis";
      break;
    case afterhours::ui::Axis::Y:
      name = "Y-Axis";
      break;
    case afterhours::ui::Axis::left:
      name = "left";
      break;
    case afterhours::ui::Axis::top:
      name = "top";
      break;
    case afterhours::ui::Axis::right:
      name = "right";
      break;
    case afterhours::ui::Axis::bottom:
      name = "bottom";
      break;
    }
    return fmt::formatter<std::string_view>::format(name, ctx);
  }
};
