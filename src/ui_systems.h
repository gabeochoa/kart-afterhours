
#pragma once

#include "components.h"
#include "query.h"
#include "settings.h"

#include "round_settings.h"

using namespace afterhours::ui;
using namespace afterhours::ui::imm;

struct ScheduleMainMenuUI : System<afterhours::ui::UIContext<InputAction>> {

  const vec2 button_size = vec2{100, 50};

  enum struct Screen {
    None,
    Main,
    CharacterCreation,
    About,
    RoundSettings,
    Settings,
  } active_screen = Screen::RoundSettings;

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
  window_manager::ProvidesAvailableWindowResolutions *resolution_provider{
      nullptr}; // non owning ptr
                // eventually std::observer_ptr?
  window_manager::ProvidesCurrentResolution *current_resolution_provider{
      nullptr}; // non owning ptr
                // eventually std::observer_ptr?
  std::vector<std::string> resolution_strs;
  size_t resolution_index{0};
  bool fs_enabled{false};
  bool ui_visible{true};

  // character creators
  std::vector<RefEntity> players;
  std::vector<RefEntity> ais;
  input::PossibleInputCollector<InputAction> inpc;

  // TODO load last used settings

  ScheduleMainMenuUI() {}

  ~ScheduleMainMenuUI() {}

  void update_resolution_cache();
  virtual void once(float) override;
  virtual bool should_run(float) override;

  void character_selector_column(Entity &parent,
                                 UIContext<InputAction> &context, size_t index,
                                 size_t num_slots);

  void character_creation(Entity &entity, UIContext<InputAction> &context);
  void round_settings(Entity &entity, UIContext<InputAction> &context);
  void main_screen(Entity &entity, UIContext<InputAction> &context);
  void settings_screen(Entity &entity, UIContext<InputAction> &context);
  void about_screen(Entity &entity, UIContext<InputAction> &context);

  void exit_game() {

    // TODO move this into a globals or something?
    // running = false;
    exit(0);
  }

  virtual void for_each_with(Entity &entity, UIContext<InputAction> &context,
                             float) override {
    switch (active_screen) {
    case Screen::None:
      break;
    case Screen::CharacterCreation:
      character_creation(entity, context);
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
    case Screen::RoundSettings:
      round_settings(entity, context);
      break;
    }
  }
};

struct ScheduleDebugUI : System<afterhours::ui::UIContext<InputAction>> {
  bool enabled = false;
  float enableCooldown = 0.f;
  float enableCooldownReset = 0.2f;

  virtual bool should_run(float dt) override;
  virtual void for_each_with(Entity &entity, UIContext<InputAction> &context,
                             float) override;
};
