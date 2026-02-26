#pragma once

#include <afterhours/src/plugins/ui/component_config.h>

namespace animation_control {
inline bool disabled = false;

using afterhours::ui::imm::ComponentConfig;

inline ComponentConfig &apply_slide_in(ComponentConfig &config) {
  if (!disabled) {
    config.with_opacity(0.0f).with_translate(-2000.0f, 0.0f);
  }
  return config;
}
}
