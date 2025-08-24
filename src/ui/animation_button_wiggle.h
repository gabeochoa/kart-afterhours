#pragma once

#include <afterhours/src/entity.h>
#include <afterhours/src/entity_helper.h>
#include <afterhours/src/plugins/animation.h>
#include <afterhours/src/plugins/ui/components.h>
#include <afterhours/src/plugins/ui/context.h>
#include <afterhours/src/plugins/ui/systems.h>
#include <cstddef>

// Re-export in game space for convenience
namespace ui_game {
namespace ui_button_anim {
enum struct Key : size_t { ButtonWiggle };
}
struct ButtonWiggleConfig {
  float hover_focus_scale = 1.03f;
  float press_scale = 0.97f;
  float hover_focus_duration = 0.16f;
  float press_duration = 0.08f;
};

inline ButtonWiggleConfig &button_wiggle_config() {
  static ButtonWiggleConfig cfg;
  return cfg;
}

inline void set_button_wiggle_config(const ButtonWiggleConfig &cfg) {
  button_wiggle_config() = cfg;
}

template <typename InputAction>
struct UpdateUIButtonWiggle
    : afterhours::ui::SystemWithUIContext<afterhours::ui::HasClickListener> {
  afterhours::ui::UIContext<InputAction> *context;
  virtual void once(float) override {
    this->context = afterhours::EntityHelper::get_singleton_cmp<
        afterhours::ui::UIContext<InputAction>>();
#if __WIN32
#else
    // this->include_derived_children = true;
#endif
  }
#if __WIN32
  virtual void for_each_with(afterhours::Entity &entity,
#else
  virtual void for_each_with(afterhours::Entity &entity,
#endif
                             afterhours::ui::UIComponent &component,
                             afterhours::ui::HasClickListener &,
                             float) override {
    if (!component.was_rendered_to_screen)
      return;
    const auto &cfg = button_wiggle_config();

    float target = 1.0f;
    bool is_hot = context->is_hot(entity.id);
    bool is_active = context->is_active(entity.id);
    bool is_focused = context->has_focus(entity.id);
    if (is_active)
      target = cfg.press_scale;
    else if (is_hot || is_focused)
      target = cfg.hover_focus_scale;

    auto handle = afterhours::animation::anim(ui_button_anim::Key::ButtonWiggle,
                                              (size_t)entity.id);
    float current = handle.value();
    if (current <= 0.f)
      current = 1.0f;
    if (std::abs(current - target) > 0.001f && !handle.is_active()) {
      float duration =
          is_active ? cfg.press_duration : cfg.hover_focus_duration;
      afterhours::animation::anim(ui_button_anim::Key::ButtonWiggle,
                                  (size_t)entity.id)
          .from(current)
          .to(target, duration, afterhours::animation::EasingType::EaseOutQuad);
    }

    float scale = afterhours::animation::clamp_value(
        ui_button_anim::Key::ButtonWiggle, (size_t)entity.id, cfg.press_scale,
        cfg.hover_focus_scale);

    entity.addComponentIfMissing<afterhours::ui::HasUIModifiers>().scale =
        scale;

    process_derived_children(entity);
  }

private:
  void process_derived_children(afterhours::Entity &parent) {
    if (!parent.has<afterhours::ui::UIComponent>()) {
      return;
    }

    auto &parent_component = parent.get<afterhours::ui::UIComponent>();
    for (auto child_id : parent_component.children) {
      auto child_entity = afterhours::EntityHelper::getEntityForID(child_id);
      if (!child_entity.has_value()) {
        continue;
      }

      Entity &child = child_entity.asE();
      if (!child.has<afterhours::ui::UIComponent>() ||
          !child.has<afterhours::ui::HasClickListener>()) {
        continue;
      }

      auto &child_component = child.get<afterhours::ui::UIComponent>();
      if (!child_component.was_rendered_to_screen) {
        continue;
      }

      const auto &cfg = button_wiggle_config();

      float target = 1.0f;
      bool is_hot = context->is_hot(child.id);
      bool is_active = context->is_active(child.id);
      bool is_focused = context->has_focus(child.id);
      if (is_active)
        target = cfg.press_scale;
      else if (is_hot || is_focused)
        target = cfg.hover_focus_scale;

      auto handle = afterhours::animation::anim(
          ui_button_anim::Key::ButtonWiggle, (size_t)child.id);
      float current = handle.value();
      if (current <= 0.f)
        current = 1.0f;
      if (std::abs(current - target) > 0.001f && !handle.is_active()) {
        float duration =
            is_active ? cfg.press_duration : cfg.hover_focus_duration;
        afterhours::animation::anim(ui_button_anim::Key::ButtonWiggle,
                                    (size_t)child.id)
            .from(current)
            .to(target, duration,
                afterhours::animation::EasingType::EaseOutQuad);
      }

      float scale = afterhours::animation::clamp_value(
          ui_button_anim::Key::ButtonWiggle, (size_t)child.id, cfg.press_scale,
          cfg.hover_focus_scale);

      child.addComponentIfMissing<afterhours::ui::HasUIModifiers>().scale =
          scale;

      process_derived_children(child);
    }
  }
};
} // namespace ui_game
