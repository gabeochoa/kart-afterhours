#pragma once

#include "car_affectors.h"
#include "components.h"
#include "game_state_manager.h"
#include "makers.h"
#include "map_system.h"
#include "query.h"
#include "round_settings.h"
#include "shader_library.h"
#include <afterhours/ah.h>

// TODO feels like we will need pathfinding at some point
struct AITargetSelection : PausableSystem<AIControlled, Transform> {

  virtual void for_each_with(Entity &entity, AIControlled &ai,
                             Transform &transform, float dt) override {
    (void)dt;

    auto round_type = RoundManager::get().active_round_type;

    switch (round_type) {
    case RoundType::Lives:
    case RoundType::Kills:
      default_ai_target(ai, transform);
      break;
    case RoundType::Hippo:
      hippo_ai_target(ai, transform);
      break;
    case RoundType::CatAndMouse:
      cat_mouse_ai_target(entity, ai, transform);
      break;
    default:
      log_error("Invalid round type in AITargetSelection: {}",
                static_cast<size_t>(round_type));
      default_ai_target(ai, transform);
      break;
    }
  }

private:
  void default_ai_target(AIControlled &ai, Transform &transform) {
    // Check if we're close enough to current target to pick a new one
    float distance_to_target = distance_sq(transform.pos(), ai.target);
    if (distance_to_target > 100.0f) {
      return;
    }

    auto opt_entity = EQ().whereHasComponent<PlayerID>().gen_first();
    if (opt_entity.valid()) {
      ai.target = opt_entity->get<Transform>().pos();
    } else {
      float screen_width = raylib::GetScreenWidth();
      float screen_height = raylib::GetScreenHeight();
      ai.target = vec_rand_in_box(Rectangle{0, 0, screen_width, screen_height});
    }
  }

  void hippo_ai_target(AIControlled &ai, Transform &transform) {
    // Find all hippo items that haven't been collected
    auto hippo_items = EQ().whereHasComponent<HippoItem>()
                           .whereLambda([](const Entity &e) {
                             return !e.get<HippoItem>().collected;
                           })
                           .gen();

    if (hippo_items.empty()) {
      // No hippos available, use default targeting
      default_ai_target(ai, transform);
      return;
    }

    // Find the closest hippo item
    vec2 closest_hippo_pos =
        hippo_items[0].get().template get<Transform>().pos();
    float closest_distance = distance_sq(transform.pos(), closest_hippo_pos);

    for (const auto &hippo_ref : hippo_items) {
      const auto &hippo = hippo_ref.get();
      vec2 hippo_pos = hippo.template get<Transform>().pos();
      float distance = distance_sq(transform.pos(), hippo_pos);

      if (distance < closest_distance) {
        closest_distance = distance;
        closest_hippo_pos = hippo_pos;
      }
    }

    // Get AI difficulty and add random offset based on difficulty
    auto ai_entity = EQ().whereHasComponent<AIControlled>()
                         .whereLambda([&ai](const Entity &e) {
                           return &e.get<AIControlled>() == &ai;
                         })
                         .gen_first();

    AIDifficulty::Difficulty difficulty = AIDifficulty::Difficulty::Medium;
    if (ai_entity.valid() && ai_entity->has<AIDifficulty>()) {
      difficulty = ai_entity->get<AIDifficulty>().difficulty;
    }

    float distance_to_hippo =
        sqrt(distance_sq(transform.pos(), closest_hippo_pos));

    float offset_range = 0.0f;
    switch (difficulty) {
    case AIDifficulty::Difficulty::Easy:
      offset_range = 200.0f;
      break;
    case AIDifficulty::Difficulty::Medium:
      offset_range = 100.0f;
      break;
    case AIDifficulty::Difficulty::Hard:
      offset_range = 50.0f;
      break;
    case AIDifficulty::Difficulty::Expert:
      offset_range = 0.0f;
      break;
    default:
      offset_range = 100.0f;
      break;
    }

    float distance_factor = std::min(1.0f, distance_to_hippo / 300.0f);
    float actual_offset_range = offset_range * distance_factor;

    vec2 target_pos = closest_hippo_pos;
    if (actual_offset_range > 0.0f) {
      unsigned int seed =
          ai_entity->id +
          static_cast<unsigned int>(closest_hippo_pos.x * 1000) +
          static_cast<unsigned int>(closest_hippo_pos.y * 1000);

      seed = seed * 1103515245 + 12345;
      float rand_x = (static_cast<float>(seed & 0x7FFF) / 32767.0f - 0.5f) *
                     actual_offset_range;

      seed = seed * 1103515245 + 12345;
      float rand_y = (static_cast<float>(seed & 0x7FFF) / 32767.0f - 0.5f) *
                     actual_offset_range;

      target_pos += vec2{rand_x, rand_y};
    }

    ai.target = target_pos;
  }

  void cat_mouse_ai_target(Entity &entity, AIControlled &ai,
                           Transform &transform) {
    if (!entity.has<HasCatMouseTracking>()) {
      default_ai_target(ai, transform);
      return;
    }

    // Check if cat and mouse game is properly initialized
    auto &cat_mouse_settings =
        RoundManager::get().get_active_rt<RoundCatAndMouseSettings>();
    if (cat_mouse_settings.state !=
        RoundCatAndMouseSettings::GameState::InGame) {
      default_ai_target(ai, transform);
      return;
    }

    auto &cat_mouse_tracking = entity.get<HasCatMouseTracking>();

    if (cat_mouse_tracking.is_cat) {
      cat_targeting(ai, transform);
    } else {
      mouse_targeting(ai, transform);
    }
  }

  void cat_targeting(AIControlled &ai, Transform &transform) {
    auto mice = EntityQuery()
                    .whereHasComponent<Transform>()
                    .whereHasComponent<HasCatMouseTracking>()
                    .whereLambda([](const Entity &e) {
                      return !e.get<HasCatMouseTracking>().is_cat;
                    })
                    .gen();

    if (mice.empty()) {
      log_warn("No mice found for cat AI");
      return;
    }

    vec2 closest_mouse_pos = mice[0].get().get<Transform>().pos();
    float closest_distance = distance_sq(transform.pos(), closest_mouse_pos);

    for (const auto &mouse_ref : mice) {
      const auto &mouse = mouse_ref.get();
      vec2 mouse_pos = mouse.get<Transform>().pos();
      float distance = distance_sq(transform.pos(), mouse_pos);

      if (distance < closest_distance) {
        closest_distance = distance;
        closest_mouse_pos = mouse_pos;
      }
    }

    ai.target = closest_mouse_pos;
  }

  void mouse_targeting(AIControlled &ai, Transform &transform) {
    auto cats = EntityQuery()
                    .whereHasComponent<Transform>()
                    .whereHasComponent<HasCatMouseTracking>()
                    .whereLambda([](const Entity &e) {
                      return e.get<HasCatMouseTracking>().is_cat;
                    })
                    .gen();

    if (cats.empty()) {
      log_warn("No cats found for mouse AI");
      return;
    }

    vec2 closest_cat_pos = cats[0].get().get<Transform>().pos();
    float closest_distance = distance_sq(transform.pos(), closest_cat_pos);

    for (const auto &cat_ref : cats) {
      const auto &cat = cat_ref.get();
      vec2 cat_pos = cat.get<Transform>().pos();
      float distance = distance_sq(transform.pos(), cat_pos);

      if (distance < closest_distance) {
        closest_distance = distance;
        closest_cat_pos = cat_pos;
      }
    }

    // Calculate direction away from cat
    vec2 away_from_cat = transform.pos() - closest_cat_pos;
    if (vec_mag(away_from_cat) < 0.1f) {
      away_from_cat = vec2(1.0f, 0.0f);
    }
    away_from_cat = vec_norm(away_from_cat);

    // Use current velocity direction if moving, otherwise use away from cat
    vec2 move_direction = away_from_cat;
    if (vec_mag(transform.velocity) > 1.0f) {
      move_direction = vec_norm(transform.velocity);
    }

    vec2 target_pos = transform.pos() + move_direction * 100.0f;

    ai.target = target_pos;
  }
};

struct AIVelocity : PausableSystem<AIControlled, Transform> {

  virtual void for_each_with(Entity &entity, AIControlled &ai,
                             Transform &transform, float dt) override {

    if (ai.target.x == 0 && ai.target.y == 0) {
      return;
    }

    vec2 dir = vec_norm(transform.pos() - ai.target);
    float target_ang = to_degrees(atan2(dir.y, dir.x)) - 90;

    float steer = 0.f;
    float accel = 5.f;

    // Calculate angle difference and determine steering direction
    float angle_diff = target_ang - transform.angle;

    // Normalize angle difference to [-180, 180]
    while (angle_diff > 180.0f)
      angle_diff -= 360.0f;
    while (angle_diff < -180.0f)
      angle_diff += 360.0f;

    if (angle_diff < -1.0f) {
      steer = -1.f;
    } else if (angle_diff > 1.0f) {
      steer = 1.f;
    }

    float steering_multiplier = affector_steering_multiplier(transform);

    if (transform.speed() > 0.01) {
      const auto minRadius = Config::get().minimum_steering_radius.data;
      const auto maxRadius = Config::get().maximum_steering_radius.data;
      const auto speed_percentage =
          transform.speed() / Config::get().max_speed.data;

      const auto rad = std::lerp(minRadius, maxRadius, speed_percentage);

      transform.angle += steer * Config::get().steering_sensitivity.data * dt *
                         rad * steering_multiplier;
      transform.angle = std::fmod(transform.angle + 360.f, 360.f);
    }

    float distance_to_target_sq = distance_sq(transform.pos(), ai.target);

    vec2 to_target_dir = vec_norm(ai.target - transform.pos());
    vec2 forward_dir{
        std::sin(transform.as_rad()),
        -std::cos(transform.as_rad()),
    };
    float ahead_dot =
        (forward_dir.x * to_target_dir.x) + (forward_dir.y * to_target_dir.y);

    const float ahead_threshold = std::cos(1.0f * (M_PI / 180.0f));
    if (ahead_dot > ahead_threshold && distance_to_target_sq > 400.0f &&
        !transform.is_reversing() && transform.accel_mult <= 1.f) {
      entity.addComponentIfMissing<WantsBoost>();
    }

    auto max_movement_limit = (transform.accel_mult > 1.f)
                                  ? (Config::get().max_speed.data * 2.f)
                                  : Config::get().max_speed.data;

    float accel_multiplier = affector_acceleration_multiplier(transform);

    float mvt =
        std::max(-max_movement_limit,
                 std::min(max_movement_limit,
                          transform.speed() + (accel * transform.accel_mult *
                                               accel_multiplier)));

    // Apply difficulty-based speed multiplier
    float difficulty_multiplier = 1.0f;
    if (entity.has<AIDifficulty>()) {
      auto difficulty = entity.get<AIDifficulty>().difficulty;
      switch (difficulty) {
      case AIDifficulty::Difficulty::Easy:
        difficulty_multiplier = 0.7f; // Slower for easy AI
        break;
      case AIDifficulty::Difficulty::Medium:
        difficulty_multiplier = 0.85f; // Slightly slower for medium AI
        break;
      case AIDifficulty::Difficulty::Hard:
      case AIDifficulty::Difficulty::Expert:
        difficulty_multiplier = 1.0f;
        break;
      }
    }
    mvt *= difficulty_multiplier;

    // Apply speed multiplier for cat and mouse mode
    if (RoundManager::get().active_round_type == RoundType::CatAndMouse) {
      auto &cat_mouse_settings =
          RoundManager::get().get_active_rt<RoundCatAndMouseSettings>();
      mvt *= cat_mouse_settings.speed_multiplier;
    }

    transform.velocity += vec2{
        std::sin(transform.as_rad()) * mvt * dt,
        -std::cos(transform.as_rad()) * mvt * dt,
    };

    transform.velocity =
        transform.velocity * affector_speed_multiplier(transform);
  }
};

// boost systems live in systems.h for both players and ai