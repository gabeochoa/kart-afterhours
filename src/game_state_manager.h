#pragma once

#include "components.h"
#include "library.h"
#include "round_settings.h"
#include <afterhours/ah.h>
#include <optional>

SINGLETON_FWD(GameStateManager)
struct GameStateManager {
  SINGLETON(GameStateManager)

  enum struct GameState {
    Menu,
    Playing,
    Paused,
  } current_state = GameState::Menu;

  enum struct Screen {
    None, // No Main Menu (ie game active)
    Main,
    CharacterCreation,
    MapSelection,
    About,
    RoundSettings,
    Settings,
    RoundEnd,
  } active_screen = Screen::Main;

  std::optional<Screen> next_screen = std::nullopt;

  void start_game() {
    RoundManager::get().reset_for_new_round();
    current_state = GameState::Playing;
    active_screen = Screen::None;
    log_info("Game started!");
  }

  void end_game(const afterhours::RefEntities &winners = {}) {
    // Remove any existing WasWinnerLastRound components from all entities
    for (auto &existing_winner_ref :
         EntityQuery().whereHasComponent<WasWinnerLastRound>().gen()) {
      existing_winner_ref.get().removeComponentIfExists<WasWinnerLastRound>();
    }

    // Add the component to all winners
    for (auto &winner : winners) {
      winner.get().addComponentIfMissing<WasWinnerLastRound>();
    }

    current_state = GameState::Menu;
    active_screen = Screen::RoundEnd;
  }

  void pause_game() {
    if (current_state == GameState::Playing) {
      current_state = GameState::Paused;
    }
  }

  void unpause_game() {
    if (current_state == GameState::Paused) {
      current_state = GameState::Playing;
    }
  }

  void set_screen(Screen screen) { active_screen = screen; }

  void set_next_screen(Screen screen) { next_screen = screen; }

  // Call this at the start of each frame to apply queued screen changes
  void update_screen() {
    if (next_screen.has_value()) {
      active_screen = next_screen.value();
      next_screen = {};
    }
  }

  [[nodiscard]] bool is_game_active() const {
    return current_state == GameState::Playing;
  }
  [[nodiscard]] bool is_menu_active() const {
    return current_state == GameState::Menu;
  }
  [[nodiscard]] bool is_paused() const {
    return current_state == GameState::Paused;
  }
};