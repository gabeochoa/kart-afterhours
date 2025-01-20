
#pragma once

#include "game.h"

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

  Screen character_creation(Entity &entity, UIContext<InputAction> &context);
  Screen round_settings(Entity &entity, UIContext<InputAction> &context);
  Screen main_screen(Entity &entity, UIContext<InputAction> &context);
  Screen settings_screen(Entity &entity, UIContext<InputAction> &context);
  Screen about_screen(Entity &entity, UIContext<InputAction> &context);

  void exit_game() { running = false; }

  virtual void for_each_with(Entity &entity, UIContext<InputAction> &context,
                             float) override {
    switch (active_screen) {
    case Screen::None:
      break;
    case Screen::CharacterCreation:
      active_screen = character_creation(entity, context);
      break;
    case Screen::About:
      active_screen = about_screen(entity, context);
      break;
    case Screen::Settings:
      active_screen = settings_screen(entity, context);
      break;
    case Screen::Main:
      active_screen = main_screen(entity, context);
      break;
    case Screen::RoundSettings:
      active_screen = round_settings(entity, context);
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
