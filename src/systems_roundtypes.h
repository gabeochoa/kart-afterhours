#pragma once

#include "components.h"
#include "game_state_manager.h"
#include "makers.h"
#include "map_system.h"
#include "query.h"
#include "round_settings.h"
#include "sound_library.h"
#include <afterhours/ah.h>
#include <fmt/format.h>

struct ProcessHippoCollection : System<Transform, HasHippoCollection> {
  virtual void for_each_with(Entity &, Transform &transform,
                             HasHippoCollection &hippo_collection,
                             float) override {
    if (RoundManager::get().active_round_type != RoundType::Hippo) {
      return;
    }
    auto hippo_items = EQ().whereHasComponent<HippoItem>()
                           .whereOverlaps(transform.rect())
                           .gen();
    for (const auto &item_ref : hippo_items) {
      Entity &item = item_ref.get();
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
    auto *resolution_provider = EntityHelper::get_singleton_cmp<
        window_manager::ProvidesCurrentResolution>();
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
  virtual void for_each_with(Entity &entity, Transform &transform,
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

struct CheckTagAndGoWinCondition : PausableSystem<> {
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

struct CheckHippoWinCondition : PausableSystem<> {

  void cleanup_remaining_hippos(RoundHippoSettings &hippo_settings) {
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
        EntityQuery().whereHasComponent<HasHippoCollection>().gen();
    if (players_with_hippos.empty()) {
      GameStateManager::get().end_game();
      return;
    }

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

struct UpdateRoundCountdown : PausableSystem<> {
  virtual void once(float dt) override {
    if (!RoundManager::get().uses_timer()) {
      return;
    }

    if (!GameStateManager::get().is_game_active()) {
      return;
    }

    auto &settings = RoundManager::get().get_active_settings();
    if (settings.state != RoundSettings::GameState::Countdown) {
      return;
    }

    settings.countdown_before_start -= dt;

    // play the sound a bit early so it feels more natural
    if (settings.countdown_before_start < 0.05f &&
        settings.countdown_before_start > 0.03f) {
      SoundLibrary::get().play(SoundFile::Round_Start);
    }

    if (settings.countdown_before_start > 0) {
      return;
    }

    settings.countdown_before_start = 0;
    settings.state = RoundSettings::GameState::InGame;
  }
};

struct RenderRoundTimer : System<window_manager::ProvidesCurrentResolution> {
  virtual void for_each_with(const Entity &,
                             const window_manager::ProvidesCurrentResolution &,
                             float) const override {
    if (!RoundManager::get().uses_timer()) {
      return;
    }
    if (!GameStateManager::get().is_game_active()) {
      return;
    }
    const int screen_width = raylib::GetScreenWidth();
    const int screen_height = raylib::GetScreenHeight();
    const float timer_x = screen_width * 0.5f;
    const float timer_y = screen_height * 0.07f;
    const float text_size = screen_height * 0.033f;
    const auto &settings = RoundManager::get().get_active_settings();

    if (settings.state == RoundSettings::GameState::Countdown &&
        settings.show_countdown_timer && settings.countdown_before_start > 0) {
      std::string countdown_text =
          fmt::format("Get Ready! {:.0f}", settings.countdown_before_start);
      const float text_width = raylib::MeasureText(countdown_text.c_str(),
                                                   static_cast<int>(text_size));
      raylib::DrawText(countdown_text.c_str(),
                       static_cast<int>(timer_x - text_width / 2.0f),
                       static_cast<int>(timer_y + screen_height * 0.056f),
                       static_cast<int>(text_size), raylib::YELLOW);
      return;
    }

    float current_time = RoundManager::get().get_current_round_time();
    if (current_time <= 0) {
      return;
    }
    std::string timer_text;
    if (current_time >= 60.0f) {
      int minutes = static_cast<int>(current_time) / 60;
      int seconds = static_cast<int>(current_time) % 60;
      timer_text = fmt::format("{}:{:02d}", minutes, seconds);
    } else {
      timer_text = fmt::format("{:.1f}s", current_time);
    }
    const float text_width =
        raylib::MeasureText(timer_text.c_str(), static_cast<int>(text_size));
    raylib::DrawText(
        timer_text.c_str(), static_cast<int>(timer_x - text_width / 2.0f),
        static_cast<int>(timer_y), static_cast<int>(text_size), raylib::WHITE);
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
