#pragma once

#include "components.h"
#include "game_state_manager.h"
#include "query.h"
#include "round_settings.h"
#include <afterhours/ah.h>

using namespace afterhours;

struct CheckLivesWinFFA : PausableSystem<> {
  virtual void once(float) override {
    if (RoundManager::get().active_round_type != RoundType::Lives) {
      return;
    }
    if (!GameStateManager::get().is_game_active()) {
      return;
    }

    auto &settings = RoundManager::get().get_active_settings();
    if (settings.team_mode_enabled) {
      return; // Team mode is handled by CheckLivesWinTeam
    }

    auto players_with_lives =
        EntityQuery()
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

struct CheckLivesWinTeam : PausableSystem<> {
  virtual void once(float) override {
    if (RoundManager::get().active_round_type != RoundType::Lives) {
      return;
    }
    if (!GameStateManager::get().is_game_active()) {
      return;
    }

    auto &settings = RoundManager::get().get_active_settings();
    if (!settings.team_mode_enabled) {
      return; // FFA mode is handled by CheckLivesWinFFA
    }

    auto players_with_lives =
        EntityQuery()
            .whereHasComponent<HasMultipleLives>()
            .whereLambda([](const Entity &e) {
              return e.get<HasMultipleLives>().num_lives_remaining > 0;
            })
            .gen();

    // Team mode: Check if any team has no remaining players
    std::map<int, int> team_lives_count;

    for (const auto &player_ref : players_with_lives) {
      Entity &player = player_ref.get();
      int team_id = -1; // Default to no team

      if (player.has<TeamID>()) {
        team_id = player.get<TeamID>().team_id;
      }

      team_lives_count[team_id]++;
    }

    // Check if any team has no remaining players
    bool team_a_has_players = team_lives_count[0] > 0;
    bool team_b_has_players = team_lives_count[1] > 0;

    if (!team_a_has_players || !team_b_has_players) {
      // One team has no remaining players - game over
      RefEntities winning_team;

      if (!team_a_has_players) {
        // Team B wins - collect all Team B players
        for (const auto &player_ref : players_with_lives) {
          Entity &player = player_ref.get();
          if (player.has<TeamID>() && player.get<TeamID>().team_id == 1) {
            winning_team.push_back(player_ref);
          }
        }
      } else {
        // Team A wins - collect all Team A players
        for (const auto &player_ref : players_with_lives) {
          Entity &player = player_ref.get();
          if (player.has<TeamID>() && player.get<TeamID>().team_id == 0) {
            winning_team.push_back(player_ref);
          }
        }
      }

      GameStateManager::get().end_game(winning_team);
      return;
    }
  }
};
