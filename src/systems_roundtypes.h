#pragma once

#include "components.h"
#include "game_state_manager.h"
#include "makers.h"
#include "map_system.h"
#include "query.h"
#include "round_settings.h"
#include <afterhours/ah.h>

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
      if (!hippo_item.collected) {
        hippo_item.collected = true;
        hippo_collection.collect_hippo();
        item.cleanup = true;
      }
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
    if (elapsed_time >= target_spawn_time) {
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
    } else if (players_with_lives.empty()) {
      GameStateManager::get().end_game();
    }
  }
};

struct UpdateCatMouseTimers : PausableSystem<HasCatMouseTracking> {
  virtual void for_each_with(Entity &, HasCatMouseTracking &catMouseTracking,
                             float dt) override {
    if (!GameStateManager::get().is_game_active()) {
      return;
    }
    auto &settings = RoundManager::get().get_active_settings();
    if (settings.state != RoundSettings::GameState::InGame) {
      return;
    }
    if (!catMouseTracking.is_cat) {
      catMouseTracking.time_as_mouse += dt;
    }
  }
};

struct HandleCatMouseTagTransfer : System<Transform, HasCatMouseTracking> {
  virtual void for_each_with(Entity &entity, Transform &transform,
                             HasCatMouseTracking &catMouseTracking,
                             float) override {
    if (!GameStateManager::get().is_game_active()) {
      return;
    }
    if (!catMouseTracking.is_cat) {
      return;
    }
    auto mice = EntityQuery()
                    .whereHasComponent<Transform>()
                    .whereHasComponent<HasCatMouseTracking>()
                    .whereLambda([](const Entity &e) {
                      return !e.get<HasCatMouseTracking>().is_cat;
                    })
                    .gen();
    for (auto &mouse_ref : mice) {
      Entity &mouse = mouse_ref.get();
      Transform &mouseTransform = mouse.get<Transform>();
      HasCatMouseTracking &mouseTracking = mouse.get<HasCatMouseTracking>();
      if (raylib::CheckCollisionRecs(transform.rect(), mouseTransform.rect())) {
        float current_time = static_cast<float>(raylib::GetTime());
        auto &cat_mouse_settings =
            RoundManager::get().get_active_rt<RoundCatAndMouseSettings>();
        if (current_time - mouseTracking.last_tag_time <
            cat_mouse_settings.tag_cooldown_time) {
          return;
        }
        catMouseTracking.is_cat = false;
        mouseTracking.is_cat = true;
        catMouseTracking.last_tag_time = current_time;
        mouseTracking.last_tag_time = current_time;
        return;
      }
    }
  }
};

struct InitializeCatMouseGame : PausableSystem<> {
  bool initialized = false;
  virtual void once(float) override {
    if (RoundManager::get().active_round_type != RoundType::CatAndMouse) {
      return;
    }
    if (!GameStateManager::get().is_game_active()) {
      initialized = false;
      return;
    }
    if (initialized) {
      return;
    }
    auto initial_cat =
        EntityQuery().whereHasComponent<HasCatMouseTracking>().gen_random();
    if (!initial_cat) {
      return;
    }
    auto &settings = RoundManager::get().get_active_settings();
    auto &cat_mouse_settings =
        RoundManager::get().get_active_rt<RoundCatAndMouseSettings>();
    settings.reset_countdown();
    cat_mouse_settings.reset_round_time();
    initial_cat->get<HasCatMouseTracking>().is_cat = true;
    initialized = true;
  }
};

struct CheckCatMouseWinCondition : PausableSystem<> {
  virtual void once(float dt) override {
    if (RoundManager::get().active_round_type != RoundType::CatAndMouse) {
      return;
    }
    if (!GameStateManager::get().is_game_active()) {
      return;
    }
    auto &settings = RoundManager::get().get_active_settings();
    if (settings.state != RoundSettings::GameState::InGame) {
      return;
    }
    auto &cat_mouse_settings =
        RoundManager::get().get_active_rt<RoundCatAndMouseSettings>();
    if (cat_mouse_settings.current_round_time > 0) {
      cat_mouse_settings.current_round_time -= dt;
      if (cat_mouse_settings.current_round_time <= 0) {
        auto players_with_tracking =
            EntityQuery().whereHasComponent<HasCatMouseTracking>().gen();
        if (players_with_tracking.empty()) {
          GameStateManager::get().end_game();
          return;
        }
        auto winner = std::max_element(
            players_with_tracking.begin(), players_with_tracking.end(),
            [](const auto &a, const auto &b) {
              return a.get().template get<HasCatMouseTracking>().time_as_mouse <
                     b.get().template get<HasCatMouseTracking>().time_as_mouse;
            });
        cat_mouse_settings.state = RoundSettings::GameState::GameOver;
        cat_mouse_settings.current_round_time = 0;
        GameStateManager::get().end_game({winner->get()});
      }
    }
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
        int max_kills = 0;
        for (const auto &entity_ref : entities_with_kills) {
          int kills = entity_ref.get().get<HasKillCountTracker>().kills;
          if (kills > max_kills)
            max_kills = kills;
        }
        afterhours::RefEntities winners;
        for (auto &entity_ref : entities_with_kills) {
          if (entity_ref.get().get<HasKillCountTracker>().kills == max_kills) {
            winners.push_back(entity_ref.get());
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

struct CheckHippoWinCondition : PausableSystem<> {
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
    if (hippo_settings.current_round_time > 0) {
      hippo_settings.current_round_time -= dt;
      if (hippo_settings.current_round_time <= 0) {
        auto remaining_hippos = EQ().whereHasComponent<HippoItem>().gen();
        for (const auto &hippo_ref : remaining_hippos)
          hippo_ref.get().cleanup = true;
        auto players_with_hippos =
            EntityQuery().whereHasComponent<HasHippoCollection>().gen();
        if (players_with_hippos.empty()) {
          GameStateManager::get().end_game();
          return;
        }
        int max_hippos = 0;
        for (const auto &entity_ref : players_with_hippos) {
          int hippos =
              entity_ref.get().get<HasHippoCollection>().get_hippo_count();
          if (hippos > max_hippos)
            max_hippos = hippos;
        }
        afterhours::RefEntities winners;
        for (auto &entity_ref : players_with_hippos) {
          if (entity_ref.get().get<HasHippoCollection>().get_hippo_count() ==
              max_hippos) {
            winners.push_back(entity_ref.get());
          }
        }
        hippo_settings.current_round_time = 0;
        GameStateManager::get().end_game(winners);
      }
    }
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
    if (settings.state == RoundSettings::GameState::Countdown) {
      settings.countdown_before_start -= dt;
      if (settings.countdown_before_start <= 0) {
        settings.countdown_before_start = 0;
        settings.state = RoundSettings::GameState::InGame;
        auto round_type = RoundManager::get().active_round_type;
        switch (round_type) {
        case RoundType::CatAndMouse:
          break;
        case RoundType::Kills:
          break;
        case RoundType::Hippo:
          break;
        case RoundType::Lives:
          break;
        default:
          break;
        }
      }
    }
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
    float current_time = RoundManager::get().get_current_round_time();
    if (current_time > 0) {
      std::string timer_text = std::to_string(current_time);
      const float text_width =
          raylib::MeasureText(timer_text.c_str(), static_cast<int>(text_size));
      raylib::DrawText(timer_text.c_str(),
                       static_cast<int>(timer_x - text_width / 2.0f),
                       static_cast<int>(timer_y), static_cast<int>(text_size),
                       raylib::WHITE);
    }
  }
};

struct ScaleCatSize : System<Transform, HasCatMouseTracking> {
  virtual void for_each_with(Entity &entity, Transform &transform,
                             HasCatMouseTracking &catMouseTracking,
                             float) override {
    if (RoundManager::get().active_round_type != RoundType::CatAndMouse) {
      transform.size = CarSizes::NORMAL_CAR_SIZE;
      if (entity.has<afterhours::texture_manager::HasSprite>()) {
        auto &sprite = entity.get<afterhours::texture_manager::HasSprite>();
        sprite.scale = CarSizes::NORMAL_SPRITE_SCALE;
      }
      return;
    }
    if (catMouseTracking.is_cat) {
      transform.size =
          CarSizes::NORMAL_CAR_SIZE * CarSizes::CAT_SIZE_MULTIPLIER;
    } else {
      transform.size = CarSizes::NORMAL_CAR_SIZE;
    }
    if (entity.has<afterhours::texture_manager::HasSprite>()) {
      auto &sprite = entity.get<afterhours::texture_manager::HasSprite>();
      sprite.scale = catMouseTracking.is_cat ? CarSizes::CAT_SPRITE_SCALE
                                             : CarSizes::NORMAL_SPRITE_SCALE;
    }
  }
};
