
#pragma once

#include "components.h"
#include "query.h"
#include "settings.h"

using namespace afterhours::ui;
using namespace afterhours::ui::imm;
struct ScheduleMainMenuUI : System<afterhours::ui::UIContext<InputAction>> {

  const vec2 button_size = vec2{100, 50};

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

  Padding control_group_padding = Padding{
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
  window_manager::ProvidesCurrentResolution
      *current_resolution_provider; // non owning ptr
                                    // eventually std::observer_ptr?
  std::vector<std::string> resolution_strs;
  int resolution_index = 0;
  bool fs_enabled = false;

  void update_resolution_cache() {
    resolution_provider = EntityHelper::get_singleton_cmp<
        window_manager::ProvidesAvailableWindowResolutions>();

    resolution_strs.clear();

    std::vector<std::string> temp;
    std::ranges::transform(resolution_provider->fetch_data(),
                           std::back_inserter(temp),
                           [](const auto &rez) { return std::string(rez); });
    resolution_strs = std::move(temp);
    resolution_index = static_cast<int>(resolution_provider->current_index());
  }

  ScheduleMainMenuUI() {}

  ~ScheduleMainMenuUI() {}

  void once(float) override {

    current_resolution_provider = EntityHelper::get_singleton_cmp<
        window_manager::ProvidesCurrentResolution>();

    if (active_screen == Screen::Settings) {
      update_resolution_cache();
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

    ComponentConfig play_button_config;
    play_button_config.padding = button_padding;
    play_button_config.label = "play";

    if (imm::button(context, mk(button_group.ent()),
                    std::move(play_button_config))
        //
    ) {
      active_screen = Screen::None;
    }

    ComponentConfig about_button_config;
    about_button_config.padding = button_padding;
    about_button_config.label = "about";

    if (imm::button(context, mk(button_group.ent()),
                    std::move(about_button_config))
        //
    ) {
      active_screen = Screen::About;
    }

    ComponentConfig settings_button_config;
    settings_button_config.padding = button_padding;
    settings_button_config.label = "settings";

    if (imm::button(context, mk(button_group.ent()),
                    std::move(settings_button_config))
        //
    ) {
      active_screen = Screen::Settings;
    }

    ComponentConfig exit_button_config;
    exit_button_config.padding = button_padding;
    exit_button_config.label = "exit";

    if (imm::button(context, mk(button_group.ent()),
                    std::move(exit_button_config))
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

    ComponentConfig control_group_config;
    control_group_config.size = {screen_pct(1.f), screen_pct(1.f)};
    control_group_config.padding = control_group_padding;
    control_group_config.is_absolute = true;
    control_group_config.debug_name = "control_group";

    auto control_group =
        imm::div(context, mk(elem.ent()), std::move(control_group_config));

    {
      float master_volume = Settings::get().get_master_volume();
      auto label =
          fmt::format("Master Volume\n {:2.0f}", master_volume * 100.f);

      ComponentConfig master_volume_config;
      master_volume_config.size = {pixels(300.f), pixels(50.f)};
      master_volume_config.label = std::move(label);

      if (auto result = slider(context, mk(control_group.ent()), master_volume,
                               std::move(master_volume_config));
          result) {
        master_volume = result.as<float>();
        Settings::get().update_master_volume(master_volume);
      }
    }

    {
      float music_volume = Settings::get().get_music_volume();
      auto label = fmt::format("Music Volume\n {:2.0f}", music_volume * 100.f);

      ComponentConfig music_volume_config;
      music_volume_config.size = {pixels(300.f), pixels(50.f)};
      music_volume_config.label = std::move(label);

      if (auto result = slider(context, mk(control_group.ent()), music_volume,
                               std::move(music_volume_config));
          result) {
        music_volume = result.as<float>();
        Settings::get().update_music_volume(music_volume);
      }
    }

    {
      float sfx_volume = Settings::get().get_sfx_volume();
      auto label = fmt::format("SFX Volume\n {:2.0f}", sfx_volume * 100.f);

      ComponentConfig sfx_volume_config;
      sfx_volume_config.size = {pixels(300.f), pixels(50.f)};
      sfx_volume_config.label = std::move(label);

      if (auto result = slider(context, mk(control_group.ent()), sfx_volume,
                               std::move(sfx_volume_config));
          result) {
        sfx_volume = result.as<float>();
        Settings::get().update_sfx_volume(sfx_volume);
      }
    }

    {
      ComponentConfig resolution_config;
      resolution_config.label = "Resolution";

      if (imm::dropdown(context, mk(control_group.ent()), resolution_strs,
                        resolution_index, std::move(resolution_config))) {
        resolution_provider->on_data_changed(
            static_cast<size_t>(resolution_index));
      }
    }

    if (imm::checkbox(context, mk(control_group.ent()),
                      Settings::get().get_fullscreen_enabled())) {
      Settings::get().toggle_fullscreen();
    }

    ComponentConfig back_button_config;
    back_button_config.padding = button_padding;
    back_button_config.label = "back";

    if (imm::button(context, mk(control_group.ent()),
                    std::move(back_button_config))
        //
    ) {
      active_screen = Screen::Main;
    }
  }

  void about_screen(Entity &entity, UIContext<InputAction> &context) {
    if (!current_resolution_provider)
      return;

    auto elem = imm::div(context, mk(entity));
    {
      elem.ent()
          .get<UIComponent>()
          .enable_font(get_font_name(FontID::EQPro), 75.f)
          .set_desired_width(screen_pct(1.f))
          .set_desired_height(screen_pct(1.f))
          .make_absolute();
      elem.ent().get<ui::UIComponentDebug>().set(
          ui::UIComponentDebug::Type::custom, "about_screen");
    }

    ComponentConfig about_screen_config;
    about_screen_config.size = {screen_pct(1.f), screen_pct(1.f)};
    about_screen_config.padding = control_group_padding;
    about_screen_config.is_absolute = true;
    about_screen_config.debug_name = "control_group";

    auto about_group =
        imm::div(context, mk(elem.ent()), std::move(about_screen_config));

    raylib::Texture2D sheet =
        EntityHelper::get_singleton_cmp<HasTexture>()->texture;
    window_manager::Resolution rez =
        current_resolution_provider->current_resolution;
    const auto width = static_cast<float>(rez.width);
    const auto height = static_cast<float>(rez.height);
    const auto scale = 5.f;
    auto x_pos = width * 0.2f;
    const auto num_icon = 3;
    const auto x_spacing = (width - x_pos * 2.f) / static_cast<float>(num_icon);

    for (int i = 0; i < num_icon; i++) {
      Rectangle frame = idx_to_sprite_frame(i, 4);
      raylib::DrawTexturePro(sheet, frame,
                             Rectangle{
                                 x_pos,
                                 height * 0.2f,
                                 frame.width * scale,
                                 frame.height * scale,
                             },
                             vec2{frame.width / 2.f, frame.height / 2.f}, 0,
                             raylib::RAYWHITE);
      x_pos += x_spacing;
    }

    ComponentConfig back_button_config;
    back_button_config.padding = button_padding;
    back_button_config.label = "back";

    if (imm::button(context, mk(about_group.ent()),
                    std::move(back_button_config))
        //
    ) {
      active_screen = Screen::Main;
    }
  }

  virtual void for_each_with(Entity &entity, UIContext<InputAction> &context,
                             float) override {
    switch (active_screen) {
    case Screen::None:
      break;
    case Screen::About:
      about_screen(entity, context);
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
  bool enabled = false;
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
          fmt::format("Max Speed\n {:.2f} m/s", Config::get().max_speed.data);
      auto pct = Config::get().max_speed.get_pct();

      ComponentConfig max_speed_slider_config;
      max_speed_slider_config.label = std::move(max_speed_label);
      max_speed_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(max_speed_slider_config));
          result) {
        Config::get().max_speed.set_pct(result.as<float>());
      }
    }

    // Breaking Acceleration
    {
      auto max_breaking_acceleration_label =
          fmt::format("Breaking \nPower \n -{:.2f} m/s^2",
                      Config::get().breaking_acceleration.data);
      auto pct = Config::get().breaking_acceleration.get_pct();

      ComponentConfig breaking_acceleration_slider_config;
      breaking_acceleration_slider_config.label =
          std::move(max_breaking_acceleration_label);
      breaking_acceleration_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(breaking_acceleration_slider_config));
          result) {
        Config::get().breaking_acceleration.set_pct(result.as<float>());
      }
    }

    // Forward Acceleration
    {
      auto forward_acceleration_label =
          fmt::format("Forward \nAcceleration \n {:.2f} m/s^2",
                      Config::get().forward_acceleration.data);
      auto pct = Config::get().forward_acceleration.get_pct();

      ComponentConfig forward_acceleration_slider_config;
      forward_acceleration_slider_config.label =
          std::move(forward_acceleration_label);
      forward_acceleration_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(forward_acceleration_slider_config));
          result) {
        Config::get().forward_acceleration.set_pct(result.as<float>());
      }
    }

    // Reverse Acceleration
    {
      auto reverse_acceleration_label =
          fmt::format("Reverse \nAcceleration \n {:.2f} m/s^2",
                      Config::get().reverse_acceleration.data);
      auto pct = Config::get().reverse_acceleration.get_pct();

      ComponentConfig reverse_acceleration_slider_config;
      reverse_acceleration_slider_config.label =
          std::move(reverse_acceleration_label);
      reverse_acceleration_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(reverse_acceleration_slider_config));
          result) {
        Config::get().reverse_acceleration.set_pct(result.as<float>());
      }
    }

    // Boost Acceleration
    {
      auto boost_acceleration_label =
          fmt::format("Boost \nAcceleration \n {:.2f} m/s^2",
                      Config::get().boost_acceleration.data);
      auto pct = Config::get().boost_acceleration.get_pct();

      ComponentConfig boost_acceleration_slider_config;
      boost_acceleration_slider_config.label =
          std::move(boost_acceleration_label);
      boost_acceleration_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(boost_acceleration_slider_config));
          result) {
        Config::get().boost_acceleration.set_pct(result.as<float>());
      }
    }

    // Boost Decay percentage
    {
      auto boost_decay_label =
          fmt::format("Boost \nDecay \n {:.2f} decay%/frame",
                      Config::get().boost_decay_percent.data);
      auto pct = Config::get().boost_decay_percent.get_pct();

      ComponentConfig boost_decay_slider_config;
      boost_decay_slider_config.label = std::move(boost_decay_label);
      boost_decay_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(boost_decay_slider_config));
          result) {
        Config::get().boost_decay_percent.set_pct(result.as<float>());
      }
    }

    // Skid Threshold
    {
      auto skid_threshold_label = fmt::format(
          "Skid \nThreshold \n {:.2f} %", Config::get().skid_threshold.data);
      auto pct = Config::get().skid_threshold.get_pct();

      ComponentConfig skid_threshold_slider_config;
      skid_threshold_slider_config.label = std::move(skid_threshold_label);
      skid_threshold_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(skid_threshold_slider_config));
          result) {
        Config::get().skid_threshold.set_pct(result.as<float>());
      }
    }

    // Steering Sensitivity
    {
      auto steering_sensitivity_label =
          fmt::format("Steering \nSensitivity \n {:.2f} %",
                      Config::get().steering_sensitivity.data);
      auto pct = Config::get().steering_sensitivity.get_pct();

      ComponentConfig steering_sensitivity_slider_config;
      steering_sensitivity_slider_config.label =
          std::move(steering_sensitivity_label);
      steering_sensitivity_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(steering_sensitivity_slider_config));
          result) {
        Config::get().steering_sensitivity.set_pct(result.as<float>());
      }
    }
  }
};
