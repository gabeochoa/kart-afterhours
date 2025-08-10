#pragma once

#include <afterhours/src/plugins/animation.h>
#include <algorithm>
#include <cstddef>

enum struct UIKey : size_t {
  MapShuffle,
  MapCard,
  MapCardPulse,
  MapPreviewFade,
};

namespace ui_anims {

inline auto make_map_card_slide(size_t i) {
  return [i](auto h) {
    float delay = 0.05f * static_cast<float>(i);
    h.from(0.0f).sequence({
        {.to_value = 0.0f,
         .duration = delay,
         .easing = afterhours::animation::EasingType::Hold},
        {.to_value = 1.0f,
         .duration = 0.25f,
         .easing = afterhours::animation::EasingType::EaseOutQuad},
    });
  };
}

} // namespace ui_anims
