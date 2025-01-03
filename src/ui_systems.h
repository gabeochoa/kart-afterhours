
#pragma once

#include "components.h"
#include "query.h"

using namespace afterhours::ui;
using namespace afterhours::ui::imm;
struct ScheduleMainMenuUI : System<afterhours::ui::UIContext<InputAction>> {

  Padding button_group_padding = Padding{
      .top = screen_pct(0.4f),
      .left = screen_pct(0.4f),
      .bottom = pixels(0.f),
      .right = pixels(0.f),
  };

  Padding button_padding = Padding{
      .top = pixels(button_size.y / 10.f),
      .left = pixels(0.f),
      .bottom = pixels(button_size.y / 10.f),
      .right = pixels(0.f),
  };

  virtual void for_each_with(Entity &entity, UIContext<InputAction> &context,
                             float) override {

    auto elem = imm::div(context, mk(entity));
    {
      elem.ent()
          .get<UIComponent>()
          .set_desired_width(screen_pct(1.f))
          .set_desired_height(screen_pct(1.f))
          .make_absolute();
      elem.ent().get<ui::UIComponentDebug>().set(
          ui::UIComponentDebug::Type::custom, "main_screen");
    }

    auto button_group = imm::div(context, mk(elem.ent()));
    {
      button_group.ent()
          .get<UIComponent>()
          .set_desired_width(screen_pct(1.f))
          .set_desired_height(screen_pct(1.f))
          .set_desired_padding(button_group_padding)
          .make_absolute();
      button_group.ent().get<ui::UIComponentDebug>().set(
          ui::UIComponentDebug::Type::custom, "button_group");
    }

    auto play_button = imm::button(context, mk(button_group.ent()));
    {
      play_button.ent().get<UIComponent>().set_desired_padding(button_padding);

      play_button.ent().get<HasLabel>().label = "play";
    }

    if (play_button) {
      elem.ent().get<ui::UIComponent>().should_hide = true;
    }

    auto about_button = imm::button(context, mk(button_group.ent()));
    {
      about_button.ent().get<UIComponent>().set_desired_padding(button_padding);
      about_button.ent().get<HasLabel>().label = "about";
    }
    if (about_button) {
      elem.ent().get<ui::UIComponent>().should_hide = true;
    }

    auto settings_button = imm::button(context, mk(button_group.ent()));
    {
      settings_button.ent().get<UIComponent>().set_desired_padding(
          button_padding);
      settings_button.ent().get<HasLabel>().label = "settings";
    }
    if (settings_button) {
      elem.ent().get<ui::UIComponent>().should_hide = true;
    }

    auto exit_button = imm::button(context, mk(button_group.ent()));
    {
      exit_button.ent().get<UIComponent>().set_desired_padding(button_padding);
      exit_button.ent().get<HasLabel>().label = "exit";
    }
    if (exit_button) {
      running = false;
    }

    //
  }
};

struct ScheduleDebugUI : System<afterhours::ui::UIContext<InputAction>> {
  bool enabled = false;
  float enableCooldown = 0.f;
  float enableCooldownReset = 0.2f;

  virtual bool should_run(float dt) override {
    enableCooldown -= dt;

    if (enableCooldown < 0) {
      enableCooldown = enableCooldownReset;
      input::PossibleInputCollector<InputAction> inpc =
          input::get_input_collector<InputAction>();
      for (auto &actions_done : inpc.inputs()) {
        if (actions_done.action == InputAction::ToggleUIDebug) {
          enabled = !enabled;
          break;
        }
      }
    }
    return enabled;
  }

  virtual void for_each_with(Entity &entity, UIContext<InputAction> &context,
                             float) override {

    if (!enabled) {
      return;
    }

    auto elem = imm::div(context, mk(entity));
    elem.ent()
        .get<UIComponent>()
        // TODO use config once it exists
        .set_flex_direction(FlexDirection::Row);

    // Max speed
    {
      auto max_speed_label =
          fmt::format("Max Speed\n {:.2f} m/s", config.max_speed.data);
      float pct = config.max_speed.get_pct();
      if (auto result = slider(context, mk(elem.ent()), pct, max_speed_label);
          result) {
        config.max_speed.set_pct(result.as<float>());
      }
    }

    // Skid Threshold
    {
      auto label =
          fmt::format("Skid Threshold \n {:.2f} %", config.skid_threshold.data);
      float pct = config.skid_threshold.get_pct();
      if (auto result = slider(context, mk(elem.ent()), pct, label); result) {
        config.skid_threshold.set_pct(result.as<float>());
      }
    }

    // Steering Sensitivity
    {
      auto label = fmt::format("Steering Sensitivity \n {:.2f} %",
                               config.steering_sensitivity.data);
      float pct = config.steering_sensitivity.get_pct();
      if (auto result = slider(context, mk(elem.ent()), pct, label); result) {
        config.steering_sensitivity.set_pct(result.as<float>());
      }
    }
  }
};
