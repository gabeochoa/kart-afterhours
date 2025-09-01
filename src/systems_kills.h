#pragma once

#include "components.h"
#include "game_state_manager.h"
#include "query.h"
#include "round_settings.h"
#include <afterhours/ah.h>

struct CheckKillsWinCondition : PausableSystem<> {
  virtual void once(float dt) override {
    if (RoundManager::get().active_round_type != RoundType::Kills) {
      return;
    }
    if (!GameStateManager::get().is_game_active()) {
      return;
    }
    auto &settings = RoundManager::get().get_active_settings();
    if (settings.state != RoundSettings::GameState::InGame) {
      return;
    }
    auto &kills_settings =
        RoundManager::get().get_active_rt<RoundKillsSettings>();
    if (kills_settings.current_round_time > 0) {
      kills_settings.current_round_time -= dt;
      if (kills_settings.current_round_time <= 0) {
        auto entities_with_kills =
            EntityQuery().whereHasComponent<HasKillCountTracker>().gen();
        if (entities_with_kills.empty()) {
          GameStateManager::get().end_game();
          return;
        }
        auto max_kills_it = std::ranges::max_element(
            entities_with_kills, std::less<>{},
            [](const afterhours::RefEntity &entity_ref) {
              return entity_ref.get().get<HasKillCountTracker>().kills;
            });
        int max_kills =
            max_kills_it == entities_with_kills.end()
                ? 0
                : max_kills_it->get().get<HasKillCountTracker>().kills;
        afterhours::RefEntities winners;
        std::ranges::copy_if(
            entities_with_kills, std::back_inserter(winners),
            [max_kills](const afterhours::RefEntity &entity_ref) {
              return entity_ref.get().get<HasKillCountTracker>().kills ==
                     max_kills;
            });
        if (winners.empty()) {
          GameStateManager::get().end_game();
          return;
        }
        kills_settings.current_round_time = 0;
        GameStateManager::get().end_game(winners);
      }
    }
  }
};
