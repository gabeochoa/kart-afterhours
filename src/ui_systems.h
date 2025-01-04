
#pragma once

#include "components.h"
#include "query.h"
#include "settings.h"

using namespace afterhours::ui;
using namespace afterhours::ui::imm;
struct ScheduleMainMenuUI : System<afterhours::ui::UIContext<InputAction>> {

  enum struct Screen {
    None,
    Main,
    About,
    Settings,
  } active_screen = Screen::Main;

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

  // settings cache stuff for now
  window_manager::ProvidesAvailableWindowResolutions
      *resolution_provider; // non owning ptr
                            // eventually std::observer_ptr?
  std::vector<std::string> resolution_strs;
  int resolution_index = 0;
  float master_volume = 0.5f;
  float music_volume = 0.5f;
  float sfx_volume = 0.5f;

  ScheduleMainMenuUI() {}
  ~ScheduleMainMenuUI() {}

  void once(float dt) override {

    if (active_screen == Screen::Settings) {
      resolution_provider =
          &(EQ().whereHasComponent<
                    window_manager::ProvidesAvailableWindowResolutions>()
                .gen_first_enforce()
                .get<window_manager::ProvidesAvailableWindowResolutions>());

      resolution_strs.clear();
      for (auto &rez : resolution_provider->fetch_data()) {
        resolution_strs.push_back(std::string(rez));
      }
    }
  }

  void main_screen(Entity &entity, UIContext<InputAction> &context) {
    auto elem = imm::div(context, mk(entity));
    {
      elem.ent()
          .get<UIComponent>()
          .enable_font(get_font_name(FontID::EQPro), 75.f)
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

    if (imm::button(context, mk(button_group.ent()),
                    ComponentConfig{
                        .padding = button_padding,
                        .label = "play",
                    })
        //
    ) {
      active_screen = Screen::None;
    }

    if (imm::button(context, mk(button_group.ent()),
                    ComponentConfig{
                        .padding = button_padding,
                        .label = "about",
                    })
        //
    ) {
      active_screen = Screen::About;
    }

    if (imm::button(context, mk(button_group.ent()),
                    ComponentConfig{
                        .padding = button_padding,
                        .label = "settings",
                    })
        //
    ) {
      active_screen = Screen::Settings;
    }

    if (imm::button(context, mk(button_group.ent()),
                    ComponentConfig{
                        .padding = button_padding,
                        .label = "exit",
                    })
        //
    ) {
      running = false;
    }
  }

  void settings_screen(Entity &entity, UIContext<InputAction> &context) {
    auto elem = imm::div(context, mk(entity));
    {
      elem.ent()
          .get<UIComponent>()
          .enable_font(get_font_name(FontID::EQPro), 75.f)
          .set_desired_width(screen_pct(1.f))
          .set_desired_height(screen_pct(1.f))
          .make_absolute();
      elem.ent().get<ui::UIComponentDebug>().set(
          ui::UIComponentDebug::Type::custom, "main_screen");
    }

    auto control_group = imm::div(context, mk(elem.ent()));
    {
      control_group.ent()
          .get<UIComponent>()
          .set_desired_width(screen_pct(1.f))
          .set_desired_height(screen_pct(1.f))
          .set_desired_padding(button_group_padding)
          .make_absolute();
      control_group.ent().get<ui::UIComponentDebug>().set(
          ui::UIComponentDebug::Type::custom, "control_group");
    }

    {
      auto label =
          fmt::format("Master Volume\n {:2.0f}", master_volume * 100.f);
      if (auto result = slider(context, mk(control_group.ent()), master_volume,
                               ComponentConfig{.label = label});
          result) {
        master_volume = result.as<float>();
        Settings::get().update_master_volume(master_volume);
      }
    }

    {
      auto label = fmt::format("Music Volume\n {:2.0f}", music_volume * 100.f);
      if (auto result = slider(context, mk(control_group.ent()), music_volume,
                               ComponentConfig{.label = label});
          result) {
        music_volume = result.as<float>();
        Settings::get().update_music_volume(music_volume);
      }
    }

    {
      auto label = fmt::format("SFX Volume\n {:2.0f}", sfx_volume * 100.f);
      if (auto result = slider(context, mk(control_group.ent()), sfx_volume,
                               ComponentConfig{.label = label});
          result) {
        sfx_volume = result.as<float>();
        Settings::get().update_sfx_volume(sfx_volume);
      }
    }

    {
      if (imm::dropdown(context, mk(control_group.ent()), resolution_strs,
                        resolution_index)) {
        resolution_provider->on_data_changed(resolution_index);
      }
    }

    if (imm::button(context, mk(control_group.ent()),
                    ComponentConfig{
                        .padding = button_padding,
                        .label = "back",
                    })
        //
    ) {
      active_screen = Screen::Main;
    }
  }

  virtual void for_each_with(Entity &entity, UIContext<InputAction> &context,
                             float) override {
    switch (active_screen) {
    case Screen::None:
    case Screen::About:
      break;
    case Screen::Settings:
      settings_screen(entity, context);
      break;
    case Screen::Main:
      main_screen(entity, context);
      break;
    }
  }
};

struct ScheduleDebugUI : System<afterhours::ui::UIContext<InputAction>> {
  bool enabled = true;
  float enableCooldown = 0.f;
  float enableCooldownReset = 0.2f;

  virtual bool should_run(float dt) override {
    enableCooldown -= dt;

    if (enableCooldown < 0) {
      enableCooldown = enableCooldownReset;
      input::PossibleInputCollector<InputAction> inpc =
          input::get_input_collector<InputAction>();

      bool debug_pressed =
          std::ranges::any_of(inpc.inputs(), [](const auto &actions_done) {
            return actions_done.action == InputAction::ToggleUIDebug;
          });
      if (debug_pressed) {
        enabled = !enabled;
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
      if (auto result = slider(context, mk(elem.ent()), pct,
                               ComponentConfig{.label = max_speed_label});
          result) {
        config.max_speed.set_pct(result.as<float>());
      }
    }

    // Skid Threshold
    {
      auto label =
          fmt::format("Skid Threshold \n {:.2f} %", config.skid_threshold.data);
      float pct = config.skid_threshold.get_pct();
      if (auto result = slider(context, mk(elem.ent()), pct,
                               ComponentConfig{.label = label});
          result) {
        config.skid_threshold.set_pct(result.as<float>());
      }
    }

    // Steering Sensitivity
    {
      auto label = fmt::format("Steering Sensitivity \n {:.2f} %",
                               config.steering_sensitivity.data);
      float pct = config.steering_sensitivity.get_pct();
      if (auto result = slider(context, mk(elem.ent()), pct,
                               ComponentConfig{.label = label});
          result) {
        config.steering_sensitivity.set_pct(result.as<float>());
      }
    }
  }
};
