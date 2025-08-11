
#pragma once

#include "game.h"

#include "components.h"
#include "game_state_manager.h"
#include "query.h"
#include "settings.h"

#include "round_settings.h"

struct MapConfig;

using namespace afterhours::ui;
using namespace afterhours::ui::imm;
using Screen = GameStateManager::Screen;

struct SetupGameStylingDefaults
    : System<afterhours::ui::UIContext<InputAction>> {

  virtual void once(float) override {
    auto &styling_defaults = UIStylingDefaults::get();

    styling_defaults.set_component_config(
        ComponentType::Button,
        ComponentConfig{}
            .with_padding(Padding{.top = screen_pct(5.f / 720.f),
                                  .left = pixels(0.f),
                                  .bottom = screen_pct(5.f / 720.f),
                                  .right = pixels(0.f)})
            .with_size(ComponentSize{screen_pct(200.f / 1280.f),
                                     screen_pct(50.f / 720.f)})
            .with_color_usage(Theme::Usage::Primary));

    styling_defaults.set_component_config(
        ComponentType::Slider,
        ComponentConfig{}
            .with_size(ComponentSize{screen_pct(200.f / 1280.f),
                                     screen_pct(50.f / 720.f)})
            .with_color_usage(Theme::Usage::Secondary));

    styling_defaults.set_component_config(
        ComponentType::Checkbox,
        ComponentConfig{}
            .with_size(ComponentSize{screen_pct(200.f / 1280.f),
                                     screen_pct(50.f / 720.f)})
            .with_color_usage(Theme::Usage::Primary));

    styling_defaults.set_component_config(
        ComponentType::CheckboxNoLabel,
        ComponentConfig{}
            .with_size(ComponentSize{screen_pct(200.f / 1280.f),
                                     screen_pct(50.f / 720.f)})
            .with_color_usage(Theme::Usage::Primary));

    styling_defaults.set_component_config(
        ComponentType::Dropdown,
        ComponentConfig{}
            .with_size(ComponentSize{screen_pct(200.f / 1280.f),
                                     screen_pct(50.f / 720.f)})
            .with_color_usage(Theme::Usage::Primary));

    styling_defaults.set_component_config(
        ComponentType::NavigationBar,
        ComponentConfig{}
            .with_size(ComponentSize{screen_pct(200.f / 1280.f),
                                     screen_pct(50.f / 720.f)})
            .with_color_usage(Theme::Usage::Primary));
  }
};

struct ScheduleMainMenuUI : System<afterhours::ui::UIContext<InputAction>> {

  Screen get_active_screen() { return GameStateManager::get().active_screen; }

  void set_active_screen(Screen screen) {
    GameStateManager::get().set_screen(screen);
  }

  // settings cache stuff for now
  window_manager::ProvidesAvailableWindowResolutions *resolution_provider{
      nullptr}; // non owning ptr
                // eventually std::observer_ptr?
  window_manager::ProvidesCurrentResolution *current_resolution_provider{
      nullptr}; // non owning ptr
                // eventually std::observer_ptr?
  std::vector<std::string> resolution_strs;
  size_t resolution_index{0};
  bool ui_visible{true};

  // character creators
  std::vector<RefEntity> players;
  std::vector<RefEntity> ais;
  input::PossibleInputCollector<InputAction> inpc;

  std::vector<Screen> navigation_stack;

  void update_resolution_cache();
  void character_selector_column(Entity &parent,
                                 UIContext<InputAction> &context,
                                 const size_t index, const size_t num_slots);
  void round_end_player_column(Entity &parent, UIContext<InputAction> &context,
                               const size_t index,
                               const std::vector<OptEntity> &round_players,
                               const std::vector<OptEntity> &round_ais,
                               std::optional<int> ranking = std::nullopt);
  std::map<EntityID, int>
  get_cat_mouse_rankings(const std::vector<OptEntity> &round_players,
                         const std::vector<OptEntity> &round_ais);
  void render_round_end_stats(UIContext<InputAction> &context, Entity &parent,
                              const OptEntity &car, raylib::Color bg_color);
  void render_lives_stats(UIContext<InputAction> &context, Entity &parent,
                          const OptEntity &car, raylib::Color bg_color);
  void render_kills_stats(UIContext<InputAction> &context, Entity &parent,
                          const OptEntity &car, raylib::Color bg_color);
  void render_score_stats(UIContext<InputAction> &context, Entity &parent,
                          const OptEntity &car, raylib::Color bg_color);
  void render_hippo_stats(UIContext<InputAction> &context, Entity &parent,
                          const OptEntity &car, raylib::Color bg_color);
  void render_cat_mouse_stats(UIContext<InputAction> &context, Entity &parent,
                              const OptEntity &car, raylib::Color bg_color);
  void render_unknown_stats(UIContext<InputAction> &context, Entity &parent,
                            const OptEntity &car, raylib::Color bg_color);
  void navigate_back();
  void navigate_to_screen(Screen screen);
  void start_game_with_random_animation();

  Screen character_creation(Entity &entity, UIContext<InputAction> &context);
  Screen map_selection(Entity &entity, UIContext<InputAction> &context);
  Screen round_settings(Entity &entity, UIContext<InputAction> &context);
  Screen main_screen(Entity &entity, UIContext<InputAction> &context);
  Screen settings_screen(Entity &entity, UIContext<InputAction> &context);
  Screen about_screen(Entity &entity, UIContext<InputAction> &context);
  Screen round_end_screen(Entity &entity, UIContext<InputAction> &context);

  void render_map_preview(
      UIContext<InputAction> &context, Entity &preview_box,
      int effective_preview_index, int selected_map_index,
      const std::vector<std::pair<int, MapConfig>> &compatible_maps,
      bool overriding_preview, int prev_preview_index);

  void render_round_settings_preview(UIContext<InputAction> &context,
                                     Entity &parent);

  void exit_game() { running = false; }

  virtual void once(float) override;
  virtual bool should_run(float) override;
  virtual void for_each_with(Entity &entity, UIContext<InputAction> &context,
                             float) override;
};

struct ScheduleDebugUI : System<afterhours::ui::UIContext<InputAction>> {
  bool enabled = false;
  float enableCooldown = 0.f;
  float enableCooldownReset = 0.2f;

  virtual bool should_run(float dt) override;
  virtual void for_each_with(Entity &entity,
                             afterhours::ui::UIContext<InputAction> &context,
                             float) override;
};

struct SchedulePauseUI : System<afterhours::ui::UIContext<InputAction>> {
  input::PossibleInputCollector<InputAction> inpc;

  void exit_game() { running = false; }

  virtual bool should_run(float) override;
  virtual void for_each_with(Entity &entity,
                             afterhours::ui::UIContext<InputAction> &context,
                             float) override;
};
