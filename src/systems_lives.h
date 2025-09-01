#pragma once

#include "components.h"
#include "game_state_manager.h"
#include "query.h"
#include "round_settings.h"
#include <afterhours/ah.h>

struct CheckLivesWinCondition : PausableSystem<> {
  virtual void once(float) override {
    if (RoundManager::get().active_round_type != RoundType::Lives) {
      return;
    }
    if (!GameStateManager::get().is_game_active()) {
      return;
    }
    auto players_with_lives =
        EntityQuery()
            .whereHasComponent<PlayerID>()
            .whereHasComponent<HasMultipleLives>()
            .whereLambda([](const Entity &e) {
              return e.get<HasMultipleLives>().num_lives_remaining > 0;
            })
            .gen();
    if (players_with_lives.size() == 1) {
      Entity &winner = players_with_lives[0].get();
      GameStateManager::get().end_game({winner});
      return;
    }
    if (players_with_lives.empty()) {
      GameStateManager::get().end_game();
      return;
    }
  }
};
