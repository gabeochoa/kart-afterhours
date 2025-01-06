
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
    resolution_index = resolution_provider->current_index();
  }

  ScheduleMainMenuUI() {}

  ~ScheduleMainMenuUI() {}

  void once(float dt) override {

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

    auto control_group =
        imm::div(context, mk(elem.ent()),
                 ComponentConfig{
                     .size = {screen_pct(1.f), screen_pct(1.f)},
                     .padding = control_group_padding,
                     .is_absolute = true,
                     .debug_name = "control_group",
                 });

    {
      float master_volume = Settings::get().get_master_volume();
      auto label =
          fmt::format("Master Volume\n {:2.0f}", master_volume * 100.f);
      if (auto result = slider(context, mk(control_group.ent()), master_volume,
                               ComponentConfig{
                                   .size = {pixels(300.f), pixels(50.f)},
                                   .label = label,
                               });
          result) {
        master_volume = result.as<float>();
        Settings::get().update_master_volume(master_volume);
      }
    }

    {
      float music_volume = Settings::get().get_music_volume();
      auto label = fmt::format("Music Volume\n {:2.0f}", music_volume * 100.f);
      if (auto result =
              slider(context, mk(control_group.ent()), music_volume,
                     ComponentConfig{.size = {pixels(300.f), pixels(50.f)},
                                     .label = label});
          result) {
        music_volume = result.as<float>();
        Settings::get().update_music_volume(music_volume);
      }
    }

    {
      float sfx_volume = Settings::get().get_sfx_volume();
      auto label = fmt::format("SFX Volume\n {:2.0f}", sfx_volume * 100.f);
      if (auto result =
              slider(context, mk(control_group.ent()), sfx_volume,
                     ComponentConfig{.size = {pixels(300.f), pixels(50.f)},
                                     .label = label});
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

    if (imm::checkbox(context, mk(control_group.ent()),
                      Settings::get().get_fullscreen_enabled())) {
      Settings::get().toggle_fullscreen();
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

    auto about_group = imm::div(context, mk(elem.ent()),
                                ComponentConfig{
                                    .size = {screen_pct(1.f), screen_pct(1.f)},
                                    .padding = control_group_padding,
                                    .is_absolute = true,
                                    .debug_name = "control_group",
                                });

    raylib::Texture2D sheet =
        EntityHelper::get_singleton_cmp<HasTexture>()->texture;
    window_manager::Resolution rez =
        current_resolution_provider->current_resolution;
    float scale = 5.f;
    float x_pos = rez.width * 0.2f;
    int num_icon = 3;
    float x_spacing = (rez.width - x_pos * 2.f) / num_icon;

    for (size_t i = 0; i < num_icon; i++) {

      Rectangle frame = idx_to_sprite_frame(i, 4);
      raylib::DrawTexturePro(sheet, frame,
                             Rectangle{
                                 x_pos,
                                 rez.height * 0.2f,
                                 frame.width * scale,
                                 frame.height * scale,
                             },
                             vec2{frame.width / 2.f, frame.height / 2.f}, 0,
                             raylib::RAYWHITE);
      x_pos += x_spacing;
    }

    if (imm::button(context, mk(about_group.ent()),
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
      float pct = Config::get().max_speed.get_pct();
      if (auto result = slider(context, mk(elem.ent()), pct,
                               ComponentConfig{
                                   .label = max_speed_label,
                                   .skip_when_tabbing = true,
                               });
          result) {
        Config::get().max_speed.set_pct(result.as<float>());
      }
    }

    // Skid Threshold
    {
      auto label = fmt::format("Skid \nThreshold \n {:.2f} %",
                               Config::get().skid_threshold.data);
      float pct = Config::get().skid_threshold.get_pct();
      if (auto result = slider(context, mk(elem.ent()), pct,
                               ComponentConfig{
                                   .label = label,
                                   .skip_when_tabbing = true,
                               });
          result) {
        Config::get().skid_threshold.set_pct(result.as<float>());
      }
    }

    // Steering Sensitivity
    {
      auto label = fmt::format("Steering \nSensitivity \n {:.2f} %",
                               Config::get().steering_sensitivity.data);
      float pct = Config::get().steering_sensitivity.get_pct();
      if (auto result = slider(context, mk(elem.ent()), pct,
                               ComponentConfig{
                                   .label = label,
                                   .skip_when_tabbing = true,
                               });
          result) {
        Config::get().steering_sensitivity.set_pct(result.as<float>());
      }
    }
  }
};
