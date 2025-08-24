#pragma once

#include <afterhours/src/plugins/animation.h>
#include <afterhours/src/plugins/ui/components.h>
#include <afterhours/src/plugins/ui/context.h>
#include <afterhours/src/plugins/ui/systems.h>
#include <afterhours/src/plugins/window_manager.h>
#include <algorithm>
#include <unordered_set>

#include "../game_state_manager.h"
#include "animation_key.h"

namespace ui_game {

template <typename InputAction>
struct UpdateUISlideIn : afterhours::ui::SystemWithUIContext<> {
  /*
   Slide-in parameters overview:
   - baseDelay (seconds): Baseline wait before an element starts animating.
   - maxExtra (seconds): Extra wait scaled by vertical position; total delay =
   baseDelay + normY * maxExtra.
   - normY (0..1): Element Y normalized to screen height (top=0, bottom=1) to
   stagger lower items later.
   - Main menu scaling: When Screen::Main, baseDelay and maxExtra are multiplied
   by 1.25f to slow only the main menu.
   - Animation sequence: Hold(delay) -> overshoot to 1.1 (0.18s, EaseOutQuad) ->
   settle to 1.0 (0.08s, EaseOutQuad).
   - limit: If an element’s right edge is beyond 25% of screen width, slide-in
   is skipped (applies mainly to left-side stack).
   - off_left / tx: Start fully off-screen left; tx interpolates to 0 as the
   animation value approaches 1. Opacity matches the same value. Units: seconds
   for time, pixels for positions/offsets.
  */
  afterhours::ui::UIContext<InputAction> *context;
  afterhours::window_manager::Resolution resolution;
  GameStateManager::Screen last_screen = GameStateManager::Screen::None;
  std::unordered_set<size_t> triggered_ids;

  virtual void once(float) override {
    this->context = afterhours::EntityHelper::get_singleton_cmp<
        afterhours::ui::UIContext<InputAction>>();
#if __WIN32 
#else
    this->include_derived_children = true;
#endif

    if (auto *resEnt = afterhours::EntityHelper::get_singleton_cmp<
            afterhours::window_manager::ProvidesCurrentResolution>()) {
      resolution = resEnt->current_resolution;
    }
  }

#if __WIN32 
  virtual void for_each_with(afterhours::Entity &entity,
#else
  virtual void for_each_with_derived(afterhours::Entity &entity,
#endif
                                     afterhours::ui::UIComponent &component,
                                     const float) override {
    auto current_screen = GameStateManager::get().active_screen;
    if (current_screen != last_screen) {
      triggered_ids.clear();
      last_screen = current_screen;
    }
    if (!component.was_rendered_to_screen)
      return;

    auto rect = component.rect();
    float rightEdge = rect.x + rect.width;
    float limit = resolution.width * 0.25f;
    if (rightEdge > limit)
      return;

    float normY = 0.0f;
    if (resolution.height > 0) {
      normY =
          std::clamp(component.rect().y / static_cast<float>(resolution.height),
                     0.0f, 1.0f);
    }
    float baseDelay = 0.02f;
    float maxExtra = 0.45f;
    float delay = baseDelay + normY * maxExtra;

    bool newly_triggered = false;
    if (!triggered_ids.contains(static_cast<size_t>(entity.id))) {
      afterhours::animation::anim(UIKey::SlideInAll,
                                  static_cast<size_t>(entity.id))
          .from(0.0f)
          .sequence({
              {.to_value = 0.0f,
               .duration = delay,
               .easing = afterhours::animation::EasingType::Hold},
              {.to_value = 1.1f,
               .duration = 0.18f,
               .easing = afterhours::animation::EasingType::EaseOutQuad},
              {.to_value = 1.0f,
               .duration = 0.08f,
               .easing = afterhours::animation::EasingType::EaseOutQuad},
          });
      triggered_ids.insert(static_cast<size_t>(entity.id));
      newly_triggered = true;
    }

    float slide_v = newly_triggered ? 0.0f : 1.0f;
    if (auto mv = afterhours::animation::get_value(
            UIKey::SlideInAll, static_cast<size_t>(entity.id));
        mv.has_value()) {
      slide_v = std::clamp(mv.value(), 0.0f, 1.0f);
    }

    auto &mods = entity.addComponentIfMissing<afterhours::ui::HasUIModifiers>();
    auto rect_now = entity.get<afterhours::ui::UIComponent>().rect();
    float off_left = -(rect_now.x + rect_now.width + 20.0f);
    float tx = (1.0f - std::min(slide_v, 1.0f)) * off_left;
    mods.translate_x = tx;
    mods.translate_y = 0.0f;
    entity.addComponentIfMissing<afterhours::ui::HasOpacity>().value =
        std::clamp(slide_v, 0.0f, 1.0f);
  }
};

} // namespace ui_game
