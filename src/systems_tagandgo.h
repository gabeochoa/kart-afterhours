#pragma once

#include "components.h"
#include "game_state_manager.h"
#include "query.h"
#include "round_settings.h"
#include <afterhours/ah.h>

struct UpdateTagAndGoTimers : PausableSystem<HasTagAndGoTracking> {
  virtual void for_each_with(Entity &, HasTagAndGoTracking &taggerTracking,
                             float dt) override {
    if (!GameStateManager::get().is_game_active()) {
      return;
    }
    auto &settings = RoundManager::get().get_active_settings();
    if (settings.state != RoundSettings::GameState::InGame) {
      return;
    }
    if (!taggerTracking.is_tagger) {
      taggerTracking.time_as_not_it += dt;
    }
  }
};

struct HandleTagAndGoTagTransfer : System<Transform, HasTagAndGoTracking> {
  virtual void for_each_with(Entity &, Transform &transform,
                             HasTagAndGoTracking &taggerTracking,
                             float) override {
    if (!GameStateManager::get().is_game_active()) {
      return;
    }
    if (!taggerTracking.is_tagger) {
      return;
    }
    auto runners = EntityQuery()
                       .whereHasComponent<Transform>()
                       .whereHasComponent<HasTagAndGoTracking>()
                       .whereLambda([](const Entity &e) {
                         return !e.get<HasTagAndGoTracking>().is_tagger;
                       })
                       .gen();
    auto &tag_settings =
        RoundManager::get().get_active_rt<RoundTagAndGoSettings>();
    float current_time = static_cast<float>(raylib::GetTime());
    auto colliding_runner_it = std::ranges::find_if(
        runners, [&](const afterhours::RefEntity &runner_ref) {
          const Entity &runner = runner_ref.get();
          const Transform &runnerTransform = runner.get<Transform>();
          const HasTagAndGoTracking &runnerTracking =
              runner.get<HasTagAndGoTracking>();
          float effective_cooldown = tag_settings.get_tag_cooldown();
          if (!raylib::CheckCollisionRecs(transform.rect(),
                                          runnerTransform.rect()))
            return false;
          if (current_time - runnerTracking.last_tag_time < effective_cooldown)
            return false;
          return true;
        });
    if (colliding_runner_it == runners.end()) {
      return;
    }
    Entity &runner = colliding_runner_it->get();
    HasTagAndGoTracking &runnerTracking = runner.get<HasTagAndGoTracking>();
    taggerTracking.is_tagger = false;
    runnerTracking.is_tagger = true;
    taggerTracking.last_tag_time = current_time;
    runnerTracking.last_tag_time = current_time;
    return;
  }
};

struct InitializeTagAndGoGame : PausableSystem<> {
  bool initialized = false;
  virtual void once(float) override {
    if (RoundManager::get().active_round_type != RoundType::TagAndGo) {
      return;
    }
    if (!GameStateManager::get().is_game_active()) {
      initialized = false;
      return;
    }
    if (initialized) {
      return;
    }
    auto initial_tagger =
        EntityQuery().whereHasComponent<HasTagAndGoTracking>().gen_random();
    if (!initial_tagger) {
      return;
    }
    auto &settings = RoundManager::get().get_active_settings();
    auto &tag_settings =
        RoundManager::get().get_active_rt<RoundTagAndGoSettings>();
    settings.reset_countdown();
    tag_settings.reset_round_time();
    initial_tagger->get<HasTagAndGoTracking>().is_tagger = true;
    initialized = true;
  }
};

struct CheckTagAndGoWinFFA : PausableSystem<> {
  virtual void once(float dt) override {
    if (RoundManager::get().active_round_type != RoundType::TagAndGo) {
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
      return; // Team mode is handled by CheckTagAndGoWinTeam
    }

    auto &tag_settings =
        RoundManager::get().get_active_rt<RoundTagAndGoSettings>();
    if (tag_settings.current_round_time <= 0) {
      return;
    }
    tag_settings.current_round_time -= dt;
    if (tag_settings.current_round_time > 0) {
      return;
    }
    auto players_with_tracking =
        EntityQuery().whereHasComponent<HasTagAndGoTracking>().gen();
    if (players_with_tracking.empty()) {
      GameStateManager::get().end_game();
      return;
    }

    // FFA mode: Original logic
    auto winner_it = std::ranges::max_element(
        players_with_tracking, std::less<>{},
        [](const afterhours::RefEntity &entity_ref) {
          return entity_ref.get().get<HasTagAndGoTracking>().time_as_not_it;
        });
    tag_settings.state = RoundSettings::GameState::GameOver;
    tag_settings.current_round_time = 0;
    GameStateManager::get().end_game({winner_it->get()});
  }
};

struct CheckTagAndGoWinTeam : PausableSystem<> {
  virtual void once(float dt) override {
    if (RoundManager::get().active_round_type != RoundType::TagAndGo) {
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
      return; // FFA mode is handled by CheckTagAndGoWinFFA
    }

    auto &tag_settings =
        RoundManager::get().get_active_rt<RoundTagAndGoSettings>();
    if (tag_settings.current_round_time <= 0) {
      return;
    }
    tag_settings.current_round_time -= dt;
    if (tag_settings.current_round_time > 0) {
      return;
    }
    auto players_with_tracking =
        EntityQuery().whereHasComponent<HasTagAndGoTracking>().gen();
    if (players_with_tracking.empty()) {
      GameStateManager::get().end_game();
      return;
    }

    // Team mode: Aggregate time_as_not_it by team
    std::map<int, float> team_times;

    for (const auto &entity_ref : players_with_tracking) {
      Entity &entity = entity_ref.get();
      int team_id = -1; // Default to no team

      if (entity.has<TeamID>()) {
        team_id = entity.get<TeamID>().team_id;
      }

      float time_not_it = entity.get<HasTagAndGoTracking>().time_as_not_it;
      team_times[team_id] += time_not_it;
    }

    // Find team with most total time not it
    float max_team_time = 0.0f;
    int winning_team = -1;

    for (const auto &[team_id, time] : team_times) {
      if (time > max_team_time) {
        max_team_time = time;
        winning_team = team_id;
      }
    }

    // Collect all players from winning team
    RefEntities winners;
    for (const auto &entity_ref : players_with_tracking) {
      Entity &entity = entity_ref.get();
      if (entity.has<TeamID>() &&
          entity.get<TeamID>().team_id == winning_team) {
        winners.push_back(entity_ref);
      }
    }

    tag_settings.state = RoundSettings::GameState::GameOver;
    tag_settings.current_round_time = 0;
    GameStateManager::get().end_game(winners);
  }
};

struct ScaleTaggerSize : System<Transform, HasTagAndGoTracking> {

  void reset_to_normal_size(Entity &entity, Transform &transform) {
    transform.size = CarSizes::NORMAL_CAR_SIZE;
    if (entity.has<afterhours::texture_manager::HasSprite>()) {
      auto &sprite = entity.get<afterhours::texture_manager::HasSprite>();
      sprite.scale = CarSizes::NORMAL_SPRITE_SCALE;
    }
  }

  void update_size(Entity &entity, Transform &transform,
                   HasTagAndGoTracking &taggerTracking) {
    transform.size =
        taggerTracking.is_tagger
            ? CarSizes::NORMAL_CAR_SIZE * CarSizes::TAG_SIZE_MULTIPLIER
            : CarSizes::NORMAL_CAR_SIZE;
    if (entity.has<afterhours::texture_manager::HasSprite>()) {
      auto &sprite = entity.get<afterhours::texture_manager::HasSprite>();
      sprite.scale = taggerTracking.is_tagger ? CarSizes::TAG_SPRITE_SCALE
                                              : CarSizes::NORMAL_SPRITE_SCALE;
    }
  }

  virtual void for_each_with(Entity &entity, Transform &transform,
                             HasTagAndGoTracking &taggerTracking,
                             float) override {
    if (RoundManager::get().active_round_type != RoundType::TagAndGo) {
      reset_to_normal_size(entity, transform);
      return;
    }
    update_size(entity, transform, taggerTracking);
  }
};
