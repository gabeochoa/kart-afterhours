#pragma once
#include "game.h"

#include "ui_key.h"

namespace ui_game {

template <typename InputAction>
struct UpdateUISlideIn : afterhours::ui::SystemWithUIContext<> {
  afterhours::ui::UIContext<InputAction> *context;
  afterhours::window_manager::Resolution resolution;

  virtual void once(float) override {
    this->context = afterhours::EntityHelper::get_singleton_cmp<
        afterhours::ui::UIContext<InputAction>>();
    this->include_derived_children = true;

    if (auto *resEnt = afterhours::EntityHelper::get_singleton_cmp<
            afterhours::window_manager::ProvidesCurrentResolution>()) {
      resolution = resEnt->current_resolution;
    }
  }

  virtual void for_each_with_derived(afterhours::Entity &entity,
                                     afterhours::ui::UIComponent &component,
                                     const float) override {
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

    afterhours::animation::one_shot(
        UIKey::SlideInAll, static_cast<size_t>(entity.id), [delay](auto h) {
          h.from(0.0f).sequence({
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
        });

    float slide_v = 1.0f;
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
