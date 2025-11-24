#pragma once

#include "components.h"
#include "game_state_manager.h"
#include "makers.h"
#include "query.h"
#include "round_settings.h"
#include <afterhours/ah.h>

struct ProcessHippoCollection : afterhours::System<Transform, HasHippoCollection> {
  virtual void for_each_with(afterhours::Entity &, Transform &transform,
                             HasHippoCollection &hippo_collection,
                             float) override {
    if (RoundManager::get().active_round_type != RoundType::Hippo) {
      return;
    }
    auto hippo_items = EQ().whereHasComponent<HippoItem>()
                           .whereOverlaps(transform.rect())
                           .gen();
    for (const auto &item_ref : hippo_items) {
      afterhours::Entity &item = item_ref.get();
      HippoItem &hippo_item = item.get<HippoItem>();
      if (hippo_item.collected)
        continue;
      hippo_item.collected = true;
      hippo_collection.collect_hippo();
      item.cleanup = true;
    }
  }
};

struct SpawnHippoItems : PausableSystem<> {
  bool spawn_counter_reset = false;
  float game_start_time = 0.0f;
  virtual void once(float) override {
    if (RoundManager::get().active_round_type != RoundType::Hippo) {
      return;
    }
    auto &settings = RoundManager::get().get_active_settings();
    if (!spawn_counter_reset &&
        settings.state == RoundSettings::GameState::InGame) {
      auto &hippo_settings =
          RoundManager::get().get_active_rt<RoundHippoSettings>();
      hippo_settings.reset_spawn_counter();
      spawn_counter_reset = true;
      game_start_time = hippo_settings.current_round_time;
    }
    if (settings.state != RoundSettings::GameState::InGame) {
      return;
    }
    auto &hippo_settings =
        RoundManager::get().get_active_rt<RoundHippoSettings>();
    int total_hippos = hippo_settings.total_hippos;
    if (hippo_settings.data.hippos_spawned_total >= total_hippos) {
      return;
    }
    auto existing_items = EQ().whereHasComponent<HippoItem>().gen();
    if (existing_items.size() >= MAX_HIPPO_ITEMS_ON_SCREEN) {
      return;
    }
    float elapsed_time = game_start_time - hippo_settings.current_round_time;
    float time_per_hippo = game_start_time / total_hippos;
    int hippo_index = hippo_settings.data.hippos_spawned_total;
    float target_spawn_time = hippo_index * time_per_hippo;
    if (elapsed_time < target_spawn_time) {
      return;
    }
    auto *resolution_provider = afterhours::EntityHelper::get_singleton_cmp<
        afterhours::window_manager::ProvidesCurrentResolution>();
    if (!resolution_provider) {
      return;
    }
    float screen_width = resolution_provider->width();
    float screen_height = resolution_provider->height();
    vec2 spawn_pos = vec_rand_in_box(
        Rectangle{50, 50, screen_width - 100, screen_height - 100});
    make_hippo_item(spawn_pos);
    hippo_settings.data.hippos_spawned_total++;
  }
};

struct CheckHippoWinFFA : PausableSystem<> {

  void cleanup_remaining_hippos(RoundHippoSettings &) {
    auto remaining_hippos = EQ().whereHasComponent<HippoItem>().gen();
    for (const auto &hippo_ref : remaining_hippos) {
      hippo_ref.get().cleanup = true;
    }
  }

  virtual void once(float dt) override {
    if (RoundManager::get().active_round_type != RoundType::Hippo) {
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
      return; // Team mode is handled by CheckHippoWinTeam
    }

    auto &hippo_settings =
        RoundManager::get().get_active_rt<RoundHippoSettings>();
    if (hippo_settings.current_round_time <= 0) {
      return;
    }

    hippo_settings.current_round_time -= dt;

    if (hippo_settings.current_round_time > 0) {
      return;
    }

    cleanup_remaining_hippos(hippo_settings);

    auto players_with_hippos =
        afterhours::EntityQuery().whereHasComponent<HasHippoCollection>().gen();
    if (players_with_hippos.empty()) {
      GameStateManager::get().end_game();
      return;
    }

    // FFA mode: Original logic
    auto max_hippos_it = std::ranges::max_element(
        players_with_hippos, std::less<>{},
        [](const afterhours::RefEntity &entity_ref) {
          return entity_ref.get().get<HasHippoCollection>().get_hippo_count();
        });
    int max_hippos =
        max_hippos_it == players_with_hippos.end()
            ? 0
            : max_hippos_it->get().get<HasHippoCollection>().get_hippo_count();

    afterhours::RefEntities winners;
    std::ranges::copy_if(
        players_with_hippos, std::back_inserter(winners),
        [max_hippos](const afterhours::RefEntity &entity_ref) {
          return entity_ref.get().get<HasHippoCollection>().get_hippo_count() ==
                 max_hippos;
        });

    hippo_settings.current_round_time = 0;
    GameStateManager::get().end_game(winners);
  }
};

struct CheckHippoWinTeam : PausableSystem<> {

  void cleanup_remaining_hippos(RoundHippoSettings &) {
    auto remaining_hippos = EQ().whereHasComponent<HippoItem>().gen();
    for (const auto &hippo_ref : remaining_hippos) {
      hippo_ref.get().cleanup = true;
    }
  }

  virtual void once(float dt) override {
    if (RoundManager::get().active_round_type != RoundType::Hippo) {
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
      return; // FFA mode is handled by CheckHippoWinFFA
    }

    auto &hippo_settings =
        RoundManager::get().get_active_rt<RoundHippoSettings>();
    if (hippo_settings.current_round_time <= 0) {
      return;
    }

    hippo_settings.current_round_time -= dt;

    if (hippo_settings.current_round_time > 0) {
      return;
    }

    cleanup_remaining_hippos(hippo_settings);

    auto players_with_hippos =
        afterhours::EntityQuery().whereHasComponent<HasHippoCollection>().gen();
    if (players_with_hippos.empty()) {
      GameStateManager::get().end_game();
      return;
    }

    // Team mode: Aggregate hippos by team
    std::map<int, int> team_hippos;

    for (const auto &entity_ref : players_with_hippos) {
      afterhours::Entity &entity = entity_ref.get();
      int team_id = -1; // Default to no team

      if (entity.has<TeamID>()) {
        team_id = entity.get<TeamID>().team_id;
      }

      int hippos = entity.get<HasHippoCollection>().get_hippo_count();
      team_hippos[team_id] += hippos;
    }

    // Find team with most hippos
    int max_team_hippos = 0;
    int winning_team = -1;

    for (const auto &[team_id, hippos] : team_hippos) {
      if (hippos > max_team_hippos) {
        max_team_hippos = hippos;
        winning_team = team_id;
      }
    }

    // Collect all players from winning team
    afterhours::RefEntities winners;
    for (const auto &entity_ref : players_with_hippos) {
      afterhours::Entity &entity = entity_ref.get();
      if (entity.has<TeamID>() &&
          entity.get<TeamID>().team_id == winning_team) {
        winners.push_back(entity_ref);
      }
    }

    hippo_settings.current_round_time = 0;
    GameStateManager::get().end_game(winners);
  }
};
