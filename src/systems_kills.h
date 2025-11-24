#pragma once

#include "components.h"
#include "game_state_manager.h"
#include "query.h"
#include "round_settings.h"
#include <afterhours/ah.h>

struct CheckKillsWinFFA : PausableSystem<> {
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
    if (settings.team_mode_enabled) {
      return; // Team mode is handled by CheckKillsWinTeam
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
        RefEntities winners;
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

struct CheckKillsWinTeam : PausableSystem<> {
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
    if (!settings.team_mode_enabled) {
      return; // FFA mode is handled by CheckKillsWinFFA
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

        // Team mode: Aggregate kills by team
        std::map<int, int> team_kills;

        for (const auto &entity_ref : entities_with_kills) {
          Entity &entity = entity_ref.get();
          int team_id = -1; // Default to no team

          if (entity.has<TeamID>()) {
            team_id = entity.get<TeamID>().team_id;
          }

          int kills = entity.get<HasKillCountTracker>().kills;
          team_kills[team_id] += kills;
        }

        // Find team with most kills
        int max_team_kills = 0;
        int winning_team = -1;

        for (const auto &[team_id, kills] : team_kills) {
          if (kills > max_team_kills) {
            max_team_kills = kills;
            winning_team = team_id;
          }
        }

        // Collect all players from winning team
        RefEntities winners;
        for (const auto &entity_ref : entities_with_kills) {
          Entity &entity = entity_ref.get();
          if (entity.has<TeamID>() &&
              entity.get<TeamID>().team_id == winning_team) {
            winners.push_back(entity_ref);
          }
        }

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
