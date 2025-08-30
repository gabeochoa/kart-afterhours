#pragma once

#include <afterhours/src/entity.h>
#include <afterhours/src/entity_helper.h>
#include <afterhours/src/plugins/animation.h>
#include <afterhours/src/plugins/ui/components.h>
#include <afterhours/src/plugins/ui/context.h>
#include <afterhours/src/plugins/ui/systems.h>
#include <cstddef>

using namespace afterhours;

// Re-export in game space for convenience
namespace ui_game {
namespace ui_anim {
enum struct Key : size_t { UIWiggle };
}
struct UIWiggleConfig {
  float hover_focus_scale = 1.03f;
  float press_scale = 0.97f;
  float hover_focus_duration = 0.16f;
  float press_duration = 0.08f;
};

inline UIWiggleConfig &ui_wiggle_config() {
  static UIWiggleConfig cfg;
  return cfg;
}

inline void set_ui_wiggle_config(const UIWiggleConfig &cfg) {
  ui_wiggle_config() = cfg;
}

template <typename InputAction>
struct UpdateUIWiggle : afterhours::ui::SystemWithUIContext<> {
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
                             float) override {
    if (!component.was_rendered_to_screen)
      return;
    const auto &cfg = ui_wiggle_config();

    float target = 1.0f;
    bool is_hot = context->is_hot(entity.id);
    bool is_active = context->is_active(entity.id);
    bool is_focused = has_focus_or_in_focus_cluster(entity.id);
    if (is_active)
      target = cfg.press_scale;
    else if (is_hot || is_focused)
      target = cfg.hover_focus_scale;

    auto handle =
        afterhours::animation::anim(ui_anim::Key::UIWiggle, (size_t)entity.id);
    float current = handle.value();
    if (current <= 0.f)
      current = 1.0f;
    if (std::abs(current - target) > 0.001f && !handle.is_active()) {
      float duration =
          is_active ? cfg.press_duration : cfg.hover_focus_duration;
      afterhours::animation::anim(ui_anim::Key::UIWiggle, (size_t)entity.id)
          .from(current)
          .to(target, duration, afterhours::animation::EasingType::EaseOutQuad);
    }

    float scale = afterhours::animation::clamp_value(
        ui_anim::Key::UIWiggle, (size_t)entity.id, cfg.press_scale,
        cfg.hover_focus_scale);

    entity.addComponentIfMissing<afterhours::ui::HasUIModifiers>().scale =
        scale;

    process_derived_children(entity);
  }

private:
  bool has_focus_or_in_focus_cluster(afterhours::EntityID entity_id) {
    if (context->has_focus(entity_id)) {
      return true;
    }

    auto entity = afterhours::EntityHelper::getEntityForID(entity_id);
    if (!entity.has_value()) {
      return false;
    }

    afterhours::Entity &e = entity.asE();

    if (e.has<afterhours::ui::FocusClusterRoot>()) {
      return context->visual_focus_id == entity_id;
    }

    if (e.has<afterhours::ui::InFocusCluster>()) {
      return is_in_focused_cluster(e);
    }

    return is_child_of_focused_cluster(e);
  }

  bool is_in_focused_cluster(afterhours::Entity &cluster_member) {
    if (!cluster_member.has<afterhours::ui::UIComponent>()) {
      return false;
    }

    auto &ui_component = cluster_member.get<afterhours::ui::UIComponent>();
    if (ui_component.parent < 0) {
      return false;
    }

    auto parent = afterhours::EntityHelper::getEntityForID(ui_component.parent);
    if (!parent.has_value()) {
      return false;
    }

    afterhours::Entity &parent_entity = parent.asE();
    if (parent_entity.has<afterhours::ui::FocusClusterRoot>()) {
      return context->visual_focus_id == parent_entity.id;
    }

    if (parent_entity.has<afterhours::ui::InFocusCluster>()) {
      return is_in_focused_cluster(parent_entity);
    }

    return false;
  }

  bool is_child_of_focused_cluster(afterhours::Entity &entity) {
    if (!entity.has<afterhours::ui::UIComponent>()) {
      return false;
    }

    auto &ui_component = entity.get<afterhours::ui::UIComponent>();
    if (ui_component.parent < 0) {
      return false;
    }

    auto parent = afterhours::EntityHelper::getEntityForID(ui_component.parent);
    if (!parent.has_value()) {
      return false;
    }

    afterhours::Entity &parent_entity = parent.asE();

    if (parent_entity.has<afterhours::ui::FocusClusterRoot>()) {
      return context->visual_focus_id == parent_entity.id;
    }

    if (parent_entity.has<afterhours::ui::InFocusCluster>()) {
      return is_in_focused_cluster(parent_entity);
    }

    return is_child_of_focused_cluster(parent_entity);
  }

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
      if (!child.has<afterhours::ui::UIComponent>()) {
        continue;
      }

      auto &child_component = child.get<afterhours::ui::UIComponent>();
      if (!child_component.was_rendered_to_screen) {
        continue;
      }

      const auto &cfg = ui_wiggle_config();

      float target = 1.0f;
      bool is_hot = context->is_hot(child.id);
      bool is_active = context->is_active(child.id);
      bool is_focused = has_focus_or_in_focus_cluster(child.id);
      if (is_active)
        target = cfg.press_scale;
      else if (is_hot || is_focused)
        target = cfg.hover_focus_scale;

      auto handle =
          afterhours::animation::anim(ui_anim::Key::UIWiggle, (size_t)child.id);
      float current = handle.value();
      if (current <= 0.f)
        current = 1.0f;
      if (std::abs(current - target) > 0.001f && !handle.is_active()) {
        float duration =
            is_active ? cfg.press_duration : cfg.hover_focus_duration;
        afterhours::animation::anim(ui_anim::Key::UIWiggle, (size_t)child.id)
            .from(current)
            .to(target, duration,
                afterhours::animation::EasingType::EaseOutQuad);
      }

      float scale = afterhours::animation::clamp_value(
          ui_anim::Key::UIWiggle, (size_t)child.id, cfg.press_scale,
          cfg.hover_focus_scale);

      child.addComponentIfMissing<afterhours::ui::HasUIModifiers>().scale =
          scale;

      child.addComponentIfMissing<afterhours::HasColor>(
          afterhours::colors::UI_WHITE);
      child.get<afterhours::HasColor>().set(afterhours::colors::UI_WHITE);

      process_derived_children(child);
    }
  }
};
} // namespace ui_game
