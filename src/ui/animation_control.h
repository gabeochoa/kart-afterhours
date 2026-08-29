#pragma once

#include <afterhours/src/plugins/animation.h>
#include <afterhours/src/plugins/ui/component_config.h>

namespace animation_control {

// Our slide-in and wiggle are hand-rolled systems, not afterhours animation
// tracks, so `disable_animations` has to reach both. We delegate to the
// engine's flag rather than keeping our own: afterhours registers its own
// handler for that command in register_builtin_handlers, which runs before
// our app commands and consumes it first, so a second flag would silently
// never be set.
inline bool disabled() { return afterhours::animation::is_instant(); }

using afterhours::ui::imm::ComponentConfig;

inline ComponentConfig &apply_slide_in(ComponentConfig &config) {
  if (!disabled()) {
    config.with_opacity(0.0f).with_translate(-2000.0f, 0.0f);
  }
  return config;
}
} // namespace animation_control
