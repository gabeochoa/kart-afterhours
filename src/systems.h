
#pragma once

#include "afterhours/ah.h"
#include "components.h"
#include "game_state_manager.h"
#include "makers.h"
#include "query.h"
#include "round_settings.h"

template <typename... Components>
struct PausableSystem : afterhours::System<Components...> {
  virtual bool should_run(float) override {
    return !GameStateManager::get().is_paused();
  }
};

struct UpdateSpriteTransform
    : System<Transform, afterhours::texture_manager::HasSprite> {

  virtual void for_each_with(Entity &entity, Transform &transform,
                             afterhours::texture_manager::HasSprite &hasSprite,
                             float) override {
    hasSprite.update_transform(transform.position, transform.size,
                               transform.angle);

    if (entity.has_child_of<HasColor>()) {
      hasSprite.update_color(entity.get_with_child<HasColor>().color());
    }
  }
};

struct UpdateAnimationTransform
    : System<Transform, afterhours::texture_manager::HasAnimation> {

  virtual void
  for_each_with(Entity &, Transform &transform,
                afterhours::texture_manager::HasAnimation &hasAnimation,
                float) override {
    hasAnimation.update_transform(transform.position, transform.size,
                                  transform.angle);
  }
};

struct RenderFPS : System<window_manager::ProvidesCurrentResolution> {
  virtual ~RenderFPS() {}
  virtual void for_each_with(
      const Entity &,
      const window_manager::ProvidesCurrentResolution &pCurrentResolution,
      float) const override {
    raylib::DrawFPS((int)(pCurrentResolution.width() - 80), 0);
  }
};

struct UpdateRenderTexture : System<> {

  window_manager::Resolution resolution;

  virtual ~UpdateRenderTexture() {}

  void once(float) {
    const window_manager::ProvidesCurrentResolution *pcr =
        EntityHelper::get_singleton_cmp<
            window_manager::ProvidesCurrentResolution>();
    if (pcr->current_resolution != resolution) {
      log_info("Regenerating render texture");
      resolution = pcr->current_resolution;
      raylib::UnloadRenderTexture(mainRT);
      mainRT = raylib::LoadRenderTexture(resolution.width, resolution.height);
    }
  }
};

struct RenderRenderTexture : System<window_manager::ProvidesCurrentResolution> {
  virtual ~RenderRenderTexture() {}
  virtual void for_each_with(
      const Entity &,
      const window_manager::ProvidesCurrentResolution &pCurrentResolution,
      float) const override {
    auto resolution = pCurrentResolution.current_resolution;
    raylib::DrawTextureRec(mainRT.texture,
                           {
                               0,
                               0,
                               static_cast<float>(resolution.width),
                               -1.f * static_cast<float>(resolution.height),
                           },
                           {0, 0}, raylib::WHITE);
  }
};

struct RenderEntities : System<Transform> {

  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             float) const override {
    if (entity.has<afterhours::texture_manager::HasSpritesheet>())
      return;
    if (entity.has<afterhours::texture_manager::HasAnimation>())
      return;

    auto entitiy_color = entity.has_child_of<HasColor>()
                             ? entity.get_with_child<HasColor>().color()
                             : raylib::RAYWHITE;

    raylib::DrawRectanglePro(
        Rectangle{
            transform.center().x,
            transform.center().y,
            transform.size.x,
            transform.size.y,
        },
        vec2{transform.size.x / 2.f,
             transform.size.y / 2.f}, // transform.center(),
        transform.angle, entitiy_color);
  }
};

struct UpdateColorBasedOnEntityID : System<HasEntityIDBasedColor> {

  virtual void for_each_with(Entity &,
                             HasEntityIDBasedColor &hasEntityIDBasedColor,
                             float) override {

    const auto parent_is_alive = EQ() //
                                     .whereID(hasEntityIDBasedColor.id)
                                     .has_values();
    if (parent_is_alive)
      return;
    hasEntityIDBasedColor.set(hasEntityIDBasedColor.default_);
  }
};

struct MatchKartsToPlayers : System<input::ProvidesMaxGamepadID> {

  virtual void for_each_with(Entity &,
                             input::ProvidesMaxGamepadID &maxGamepadID,
                             float) override {

    auto existing_players = EQ().whereHasComponent<PlayerID>().gen();

    // we are good
    if (existing_players.size() + 1 == maxGamepadID.count())
      return;

    if (existing_players.size() > maxGamepadID.count() + 1) {
      // remove the player that left
      for (Entity &player : existing_players) {
        if (input::is_gamepad_available(player.get<PlayerID>().id))
          continue;
        player.cleanup = true;
      }
      return;
    }
    // TODO add +1 here to auto gen extra players
    for (int i = 0; i < (int)maxGamepadID.count(); i++) {
      bool found = false;
      for (Entity &player : existing_players) {
        if (i == player.get<PlayerID>().id) {
          found = true;
          break;
        }
      }
      if (!found) {
        make_player(i);
      }
    }
  }
};

struct RenderWeaponCooldown : System<Transform, CanShoot> {

  virtual void for_each_with(const Entity &, const Transform &transform,
                             const CanShoot &canShoot, float) const override {

    for (auto it = canShoot.weapons.begin(); it != canShoot.weapons.end();
         ++it) {
      const std::unique_ptr<Weapon> &weapon = it->second;

      vec2 center = transform.center();
      Rectangle body = transform.rect();

      float nw = body.width / 2.f;
      float nh = body.height / 2.f;

      Rectangle arm = Rectangle{
          center.x, //
          center.y, //
          nw,
          nh * (weapon->cooldown / weapon->config.cooldownReset),
      };

      raylib::DrawRectanglePro(arm,
                               {nw / 2.f, nh / 2.f}, // rotate around center
                               transform.angle, raylib::RED);
    }
  }
};

struct Shoot : PausableSystem<PlayerID, Transform, CanShoot> {
  input::PossibleInputCollector<InputAction> inpc;

  virtual void once(float) override {
    inpc = input::get_input_collector<InputAction>();
  }
  virtual void for_each_with(Entity &entity, PlayerID &playerID, Transform &,
                             CanShoot &canShoot, float dt) override {

    magic_enum::enum_for_each<InputAction>([&](auto val) {
      constexpr InputAction action = val;
      canShoot.pass_time(action, dt);
    });

    if (!inpc.has_value()) {
      return;
    }

    for (const auto &actions_done : inpc.inputs()) {
      if (actions_done.id != playerID.id)
        continue;

      canShoot.fire(entity, actions_done.action, dt);
    }
  }
};

struct WrapAroundTransform : System<Transform, CanWrapAround> {

  window_manager::Resolution resolution;

  virtual void once(float) override {
    resolution =
        EQ().whereHasComponent<
                afterhours::window_manager::ProvidesCurrentResolution>()
            .gen_first_enforce()
            .get<afterhours::window_manager::ProvidesCurrentResolution>()
            .current_resolution;
  }

  virtual void for_each_with(Entity &entity, Transform &transform,
                             CanWrapAround &canWrap, float) override {

    float width = (float)resolution.width;
    float height = (float)resolution.height;

    float padding = canWrap.padding;

    raylib::Rectangle screenRect{0, 0, width, height};
    const auto overlaps =
        EQ::WhereOverlaps::overlaps(screenRect, transform.rect());
    if (overlaps) {
      // Early return, no wrapping checks need to be done further.
      return;
    }

    // Non-overlapping logic

    // If it's not overlapping the screen rect and it doesn't want to be
    // rendered out of bounds
    if (!transform.render_out_of_bounds || transform.cleanup_out_of_bounds) {
      entity.cleanup = transform.cleanup_out_of_bounds;
      return;
    }

    if (transform.rect().x > width + padding) {
      transform.position.x = -padding;
    }

    if (transform.rect().x < 0 - padding) {
      transform.position.x = width + padding;
    }

    if (transform.rect().y < 0 - padding) {
      transform.position.y = height + padding;
    }

    if (transform.rect().y > height + padding) {
      transform.position.y = -padding;
    }
  }
};

struct SkidMarks : System<Transform, TireMarkComponent> {
  virtual void for_each_with(Entity &, Transform &transform,
                             TireMarkComponent &tire, float dt) override {

    tire.pass_time(dt);

    const auto should_skid = [&]() -> bool {
      if (transform.accel_mult > 2.f) {
        return true;
      }

      if (transform.speed() == 0.f) {
        return false;
      }

      // Normalize the velocity vector
      const auto velocity_normalized = transform.velocity / transform.speed();

      // Forward direction based on car's angle
      const auto angle_rads = to_radians(transform.angle - 90.f);
      ;
      const vec2 car_forward = {(float)std::cos(angle_rads),
                                (float)std::sin(angle_rads)};

      // Calculate the dot product
      const auto dot = vec_dot(velocity_normalized, car_forward);

      // The closer the dot product is close to 0,
      // the more the car is moving sideways.
      // (perpendicular to its heading)
      const auto is_moving_sideways =
          std::fabs(dot) < (Config::get().skid_threshold.data / 100.f);

      return is_moving_sideways;
    };

    if (should_skid()) {

      const auto pos = transform.center();

      tire.add_mark(pos, !tire.added_last_frame);
      tire.added_last_frame = true;
    } else {
      tire.added_last_frame = false;
    }
  }
};

struct RenderSkid : System<Transform, TireMarkComponent> {
  virtual void for_each_with(const Entity &, const Transform &,
                             const TireMarkComponent &tire,
                             float) const override {

    const auto single_tire = [&](vec2 off) {
      for (size_t i = 1; i < tire.points.size(); i++) {
        auto mp0 = tire.points[i - 1];
        auto mp1 = tire.points[i];
        if (distance_sq(mp0.position, mp1.position) > 100.f) {
          continue;
        }
        float pct = mp0.time / mp0.lifetime;
        raylib::DrawSplineSegmentLinear(
            mp0.position + off, mp1.position + off, 5.f,
            raylib::Color(20, 20, 20, (unsigned char)(255 * pct)));
      }
    };

    float x = 7.f;
    float y = 4.f;
    // i tried 4 tires but it was kinda too crowded
    single_tire(vec2{x, y});
    single_tire(vec2{-x, -y});
  }
};

struct RenderOOB : System<Transform> {
  window_manager::Resolution resolution;
  Rectangle screen;

  virtual void once(float) override {
    resolution = EntityHelper::get_singleton_cmp<
                     afterhours::window_manager::ProvidesCurrentResolution>()
                     ->current_resolution;

    screen = Rectangle{0, 0, (float)resolution.width, (float)resolution.height};
  }

  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             float) const override {
    if (is_point_inside(transform.pos(), screen) ||
        !transform.render_out_of_bounds) {
      return;
    }

    const auto size =
        std::max(5.f, //
                 std::lerp(20.f, 5.f,
                           (distance_sq(transform.pos(), rect_center(screen)) /
                            (screen.width * screen.height))));

    raylib::DrawCircleV(calc(screen, transform.pos()), size,
                        entity.has<HasColor>() ? entity.get<HasColor>().color()
                                               : raylib::PINK);
  }
};

struct UpdateTrackingEntities : System<Transform, TracksEntity> {
  virtual void for_each_with(Entity &, Transform &transform,
                             TracksEntity &tracker, float) override {

    OptEntity opte = EQ().whereID(tracker.id).gen_first();
    if (!opte.valid())
      return;
    transform.position = (opte->get<Transform>().pos() + tracker.offset);
    transform.angle = opte->get<Transform>().angle;
  }
};

struct UpdateCollidingEntities : PausableSystem<Transform> {

  std::set<int> ids;

  virtual void once(float) override { ids.clear(); }

  void positional_correction(Transform &a, Transform &b,
                             const vec2 &collisionNormal,
                             float penetrationDepth) {
    float correctionMagnitude =
        std::max(penetrationDepth, 0.0f) /
        (1.0f / a.collision_config.mass + 1.0f / b.collision_config.mass);
    vec2 correction = collisionNormal * correctionMagnitude;

    a.position -= correction / a.collision_config.mass;
    b.position += correction / b.collision_config.mass;
  }

  void resolve_collision(Transform &a, Transform &b, const float dt) {
    vec2 collisionNormal = vec_norm(b.position - a.position);

    // Calculate normal impulse
    float impulse = calculate_impulse(a, b, collisionNormal);
    vec2 impulseVector =
        collisionNormal * impulse * Config::get().collision_scalar.data * dt;

    // Apply normal impulse
    if (a.collision_config.mass > 0.0f &&
        a.collision_config.mass != std::numeric_limits<float>::max()) {
      a.velocity -= impulseVector / a.collision_config.mass;
    }

    if (b.collision_config.mass > 0.0f &&
        b.collision_config.mass != std::numeric_limits<float>::max()) {
      b.velocity += impulseVector / b.collision_config.mass;
    }

    // Calculate and apply friction impulse
    vec2 relativeVelocity = b.velocity - a.velocity;
    vec2 tangent =
        vec_norm(relativeVelocity -
                 collisionNormal * vec_dot(relativeVelocity, collisionNormal));

    float frictionImpulseMagnitude =
        vec_dot(relativeVelocity, tangent) /
        (1.0f / a.collision_config.mass + 1.0f / b.collision_config.mass);
    float frictionCoefficient =
        std::sqrt(a.collision_config.friction * b.collision_config.friction);
    frictionImpulseMagnitude =
        std::clamp(frictionImpulseMagnitude, -impulse * frictionCoefficient,
                   impulse * frictionCoefficient);

    vec2 frictionImpulse = tangent * frictionImpulseMagnitude *
                           Config::get().collision_scalar.data * dt;

    if (a.collision_config.mass > 0.0f &&
        a.collision_config.mass != std::numeric_limits<float>::max()) {
      a.velocity -= frictionImpulse / a.collision_config.mass;
    }

    if (b.collision_config.mass > 0.0f &&
        b.collision_config.mass != std::numeric_limits<float>::max()) {
      b.velocity += frictionImpulse / b.collision_config.mass;
    }

    // Positional correction
    float penetrationDepth = calculate_penetration_depth(a.rect(), b.rect());
    positional_correction(a, b, collisionNormal, penetrationDepth);
  }

  float calculate_penetration_depth(const Rectangle &a, const Rectangle &b) {
    // Calculate the overlap along the X axis
    float overlapX =
        std::min(a.x + a.width, b.x + b.width) - std::max(a.x, b.x);

    // Calculate the overlap along the Y axis
    float overlapY =
        std::min(a.y + a.height, b.y + b.height) - std::max(a.y, b.y);

    // If there’s no overlap, return 0
    if (overlapX <= 0.0f || overlapY <= 0.0f)
      return 0.0f;

    // Return the smaller overlap for the penetration depth
    return std::min(overlapX, overlapY);
  }

  float calculate_dynamic_restitution(const Transform &a, const Transform &b) {
    float baseRestitution = std::min(a.collision_config.restitution,
                                     b.collision_config.restitution);

    // Reduce restitution for high-speed collisions
    vec2 relativeVelocity = b.velocity - a.velocity;
    float speed = Vector2Length(relativeVelocity);

    if (speed >
        (Config::get().max_speed.data * .75f)) { // Adjust threshold as needed
      baseRestitution *= 0.5f; // Reduce bounce for high-speed collisions
    }

    return baseRestitution;
  }

  float calculate_impulse(const Transform &a, const Transform &b,
                          const vec2 &collisionNormal) {
    vec2 relativeVelocity = b.velocity - a.velocity;
    float velocityAlongNormal = vec_dot(relativeVelocity, collisionNormal);

    // Prevent objects from "sticking" or resolving collisions when moving apart
    if (velocityAlongNormal > 0.0f)
      return 0.0f;

    float restitution = calculate_dynamic_restitution(a, b);

    // Impulse calculation with restitution
    float impulse = -(1.0f + restitution) * velocityAlongNormal;
    impulse /=
        (1.0f / a.collision_config.mass + 1.0f / b.collision_config.mass);

    return impulse;
  }

  virtual void for_each_with(Entity &entity, Transform &transform,
                             float dt) override {

    // skip any already resolved
    if (ids.contains(entity.id)) {
      return;
    }

    const auto gets_absorbed = [](Entity &ent) {
      return ent.has<CollisionAbsorber>() &&
             ent.get<CollisionAbsorber>().absorber_type ==
                 CollisionAbsorber::AbsorberType::Absorbed;
    };

    if (gets_absorbed(entity)) {
      return;
    }

    auto can_collide = EQ().whereHasComponent<Transform>()
                           .whereNotID(entity.id)
                           .whereOverlaps(transform.rect())
                           .gen();

    for (Entity &other : can_collide) {
      Transform &b = other.get<Transform>();

      // If the other transform gets absorbed, but this is its parent, ignore
      // collision.
      if (gets_absorbed(other)) {
        if (other.get<CollisionAbsorber>().parent_id.value_or(-1) ==
            entity.id) {
          ids.insert(other.id);
          continue;
        }
      }

      resolve_collision(transform, b, dt);
      ids.insert(other.id);
    }
  }
};

struct VelFromInput : PausableSystem<PlayerID, Transform> {
  virtual void for_each_with(Entity &, PlayerID &playerID, Transform &transform,
                             float dt) override {
    input::PossibleInputCollector<InputAction> inpc =
        input::get_input_collector<InputAction>();
    if (!inpc.has_value()) {
      return;
    }

    transform.accel = 0.f;
    auto steer = 0.f;

    for (const auto &actions_done : inpc.inputs()) {
      if (actions_done.id != playerID.id) {
        continue;
      }

      switch (actions_done.action) {
      case InputAction::Accel:
        transform.accel = transform.is_reversing()
                              ? -Config::get().breaking_acceleration.data
                              : Config::get().forward_acceleration.data;
        break;
      case InputAction::Brake:
        transform.accel = transform.is_reversing()
                              ? Config::get().reverse_acceleration.data
                              : -Config::get().breaking_acceleration.data;
        break;
      case InputAction::Left:
        steer = -actions_done.amount_pressed;
        break;
      case InputAction::Right:
        steer = actions_done.amount_pressed;
        break;
      case InputAction::Boost:
        break;
      default:
        break;
      }
    }

    for (auto &actions_done : inpc.inputs_pressed()) {
      if (actions_done.id != playerID.id) {
        continue;
      }

      switch (actions_done.action) {
      case InputAction::Accel:
        break;
      case InputAction::Brake:
        break;
      case InputAction::Left:
        break;
      case InputAction::Right:
        break;
      case InputAction::Boost: {
        if (!transform.is_reversing() && transform.accel_mult <= 1.f) {
          transform.accel_mult = Config::get().boost_acceleration.data;
          const auto upfront_boost_speed = Config::get().max_speed.data * .2f;
          transform.velocity +=
              vec2{std::sin(transform.as_rad()) * upfront_boost_speed,
                   -std::cos(transform.as_rad()) * upfront_boost_speed};
        }
      } break;
      default:
        break;
      }
    }

    if (transform.speed() > 0.01) {
      const auto minRadius = Config::get().minimum_steering_radius.data;
      const auto maxRadius = Config::get().maximum_steering_radius.data;
      const auto speed_percentage =
          transform.speed() / Config::get().max_speed.data;

      const auto rad = std::lerp(minRadius, maxRadius, speed_percentage);

      transform.angle +=
          steer * Config::get().steering_sensitivity.data * dt * rad;
      transform.angle = std::fmod(transform.angle + 360.f, 360.f);
    }

    const auto decayed_accel_mult =
        transform.accel_mult -
        (transform.accel_mult * Config::get().boost_decay_percent.data * dt);
    transform.accel_mult = std::max(1.f, decayed_accel_mult);

    float mvt{0.f};
    if (transform.accel != 0.f) {
      mvt = std::clamp(
          transform.speed() + (transform.accel * transform.accel_mult),
          -Config::get().max_speed.data, Config::get().max_speed.data);
    } else {
      mvt = std::clamp(transform.speed(), -Config::get().max_speed.data,
                       Config::get().max_speed.data);
    }

    if (!transform.is_reversing()) {
      transform.velocity += vec2{
          std::sin(transform.as_rad()) * mvt * dt,
          -std::cos(transform.as_rad()) * mvt * dt,
      };
    } else {
      transform.velocity += vec2{
          -std::sin(transform.as_rad()) * mvt * dt,
          std::cos(transform.as_rad()) * mvt * dt,
      };
    }

    transform.speed_dot_angle =
        transform.velocity.x * std::sin(transform.as_rad()) +
        transform.velocity.y * -std::cos(transform.as_rad());
  }
};

struct AITargetSelection : PausableSystem<AIControlled, Transform> {

  virtual void for_each_with(Entity &entity, AIControlled &ai,
                             Transform &transform, float dt) override {
    (void)dt;

    // TODO make the ai have a difficulty slider
    auto round_type = RoundManager::get().active_round_type;

    switch (round_type) {
    case RoundType::Lives:
    case RoundType::Kills:
    case RoundType::Score:
      default_ai_target(ai, transform);
      break;
    case RoundType::CatAndMouse:
      cat_mouse_ai_target(entity, ai, transform);
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

    // Get screen bounds
    int screen_width = raylib::GetScreenWidth();
    int screen_height = raylib::GetScreenHeight();

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

  virtual void for_each_with(Entity &, AIControlled &ai, Transform &transform,
                             float dt) override {

    if (ai.target.x == 0 && ai.target.y == 0) {
      return;
    }

    vec2 dir = vec_norm(transform.pos() - ai.target);
    float ang = to_degrees(atan2(dir.y, dir.x)) - 90;

    float steer = 0.f;
    float accel = 5.f;

    if (ang < transform.angle) {
      steer = -1.f;
    } else if (ang > transform.angle) {
      steer = 1.f;
    }

    float minRadius = 10.0f;
    float maxRadius = 300.0f;
    float rad = std::lerp(minRadius, maxRadius,
                          transform.speed() / Config::get().max_speed.data);

    transform.angle = ang;

    auto max_movement_limit = (transform.accel_mult > 1.f)
                                  ? (Config::get().max_speed.data * 2.f)
                                  : Config::get().max_speed.data;

    float mvt =
        std::max(-max_movement_limit,
                 std::min(max_movement_limit, transform.speed() + accel));

    transform.angle += steer * dt * rad;

    transform.velocity += vec2{
        std::sin(transform.as_rad()) * mvt * dt,
        -std::cos(transform.as_rad()) * mvt * dt,
    };
  }
};

struct Move : PausableSystem<Transform> {

  virtual void for_each_with(Entity &, Transform &transform, float) override {
    transform.position += transform.velocity;
    transform.velocity =
        transform.velocity * (transform.accel != 0 ? 0.99f : 0.98f);
  }
};

struct DrainLife : System<HasLifetime> {

  virtual void for_each_with(Entity &entity, HasLifetime &hasLifetime,
                             float dt) override {

    hasLifetime.lifetime -= dt;
    if (hasLifetime.lifetime <= 0) {
      entity.cleanup = true;
    }
  }
};

struct ProcessDamage : PausableSystem<Transform, HasHealth> {

  virtual void for_each_with(Entity &entity, Transform &transform,
                             HasHealth &hasHealth, float dt) override {

    hasHealth.pass_time(dt);
    if (hasHealth.iframes > 0.f) {
      return;
    }

    auto can_damage = EQ().whereHasComponent<CanDamage>()
                          .whereNotID(entity.id)
                          .whereOverlaps(transform.rect())
                          .gen();

    for (Entity &damager : can_damage) {
      const CanDamage &cd = damager.get<CanDamage>();
      if (cd.id == entity.id)
        continue;
      hasHealth.amount -= cd.amount;
      hasHealth.iframes = hasHealth.iframesReset;

      // Track the entity that caused this damage for kill attribution
      hasHealth.last_damaged_by = cd.id;
      damager.cleanup = true;
    }
  }
};

struct ProcessCollisionAbsorption : System<Transform, CollisionAbsorber> {

  virtual void for_each_with(Entity &entity, Transform &transform,
                             CollisionAbsorber &collision_absorber,
                             float) override {
    // We let the absorbed things (e.g. bullets) manage cleaning themselves up,
    // rather than the other way around.
    if (collision_absorber.absorber_type ==
        CollisionAbsorber::AbsorberType::Absorber) {
      return;
    }

    const auto unrelated_absorber =
        [&collision_absorber](const Entity &collider) {
          const auto &other_absorber = collider.get<CollisionAbsorber>();
          const auto are_related = collision_absorber.parent_id.value_or(-1) ==
                                   other_absorber.parent_id.value_or(-2);

          if (are_related) {
            return false;
          }

          return other_absorber.absorber_type ==
                 CollisionAbsorber::AbsorberType::Absorber;
        };

    auto collided_with_absorber =
        EQ().whereHasComponent<CollisionAbsorber>()
            .whereNotID(entity.id)
            .whereNotID(collision_absorber.parent_id.value_or(-1))
            .whereOverlaps(transform.rect())
            .whereLambda(unrelated_absorber)
            .gen();

    if (!collided_with_absorber.empty()) {
      entity.cleanup = true;
    }
  }
};

struct ProcessDeath : PausableSystem<Transform, HasHealth> {

  virtual void for_each_with(Entity &entity, Transform &transform,
                             HasHealth &hasHealth, float) override {
    if (hasHealth.amount > 0) {
      return;
    }

    log_info("Entity {} died with health {}", entity.id, hasHealth.amount);
    make_explosion_anim(entity);

    // Handle player respawning
    if (entity.has<PlayerID>()) {
      transform.position =
          get_spawn_position(static_cast<size_t>(entity.get<PlayerID>().id));
    }

    // Handle kill attribution in kill-based rounds
    if (RoundManager::get().active_round_type == RoundType::Kills) {
      handle_kill_attribution(entity, hasHealth);
    }

    // Handle lives system
    if (entity.has<HasMultipleLives>()) {
      if (RoundManager::get().active_round_type == RoundType::Kills) {
        hasHealth.amount = hasHealth.max_amount;
        return;
      }

      entity.get<HasMultipleLives>().num_lives_remaining -= 1;
      if (entity.get<HasMultipleLives>().num_lives_remaining) {
        hasHealth.amount = hasHealth.max_amount;
        return;
      }
    }

    entity.cleanup = true;
  }

private:
  void handle_kill_attribution(const Entity &, const HasHealth &hasHealth) {
    if (!hasHealth.last_damaged_by.has_value()) {
      log_warn("Player died but we don't know why");
      return;
    }

    // Look up the entity that caused the damage
    auto damager_entities = EntityQuery({.force_merge = true})
                                .whereID(*hasHealth.last_damaged_by)
                                .gen();

    if (damager_entities.empty()) {
      log_warn("Player died but damager entity not found");
      return;
    }

    Entity &damager = damager_entities[0].get();

    if (!damager.has<PlayerID>()) {
      log_warn("Player died from environment damage - no kill awarded");
      return;
    }

    input::GamepadID killer_player_id = damager.get<PlayerID>().id;

    // Find the player with this ID and give them a kill
    auto killer_players = EntityQuery({.force_merge = true})
                              .whereHasComponent<PlayerID>()
                              .whereHasComponent<HasKillCountTracker>()
                              .whereLambda([killer_player_id](const Entity &e) {
                                return e.get<PlayerID>().id == killer_player_id;
                              })
                              .gen();

    if (!killer_players.empty()) {
      killer_players[0].get().template get<HasKillCountTracker>().kills++;
      log_info("Player {} got a kill!", killer_player_id);
    }
  }
};

struct RenderLabels : System<Transform, HasLabels> {
  virtual void for_each_with(const Entity &, const Transform &transform,
                             const HasLabels &hasLabels, float) const override {

    const auto get_label_display_for_type = [](const Transform &transform_in,
                                               const LabelInfo &label_info_in) {
      switch (label_info_in.label_type) {
      case LabelInfo::LabelType::StaticText:
        return label_info_in.label_text;
      case LabelInfo::LabelType::VelocityText:
        return (transform_in.is_reversing() ? "-" : "") +
               std::to_string(transform_in.speed()) + label_info_in.label_text;
      case LabelInfo::LabelType::AccelerationText:
        return std::to_string(transform_in.accel * transform_in.accel_mult) +
               label_info_in.label_text;
      }

      return label_info_in.label_text;
    };

    const auto width = transform.rect().width;
    const auto height = transform.rect().height;

    // Makes the label percentages scale from top-left of the object rect as (0,
    // 0)
    const auto base_x_offset = transform.pos().x - width;
    const auto base_y_offset = transform.pos().y - height;

    for (const auto &label_info : hasLabels.label_info) {
      const auto label_to_display =
          get_label_display_for_type(transform, label_info);
      const auto label_pos_offset = label_info.label_pos_offset;

      const auto x_offset = base_x_offset + (width * label_pos_offset.x);
      const auto y_offset = base_y_offset + (height * label_pos_offset.y);

      draw_text_ex(
          EntityHelper::get_singleton_cmp<ui::FontManager>()->get_active_font(),
          label_to_display.c_str(), vec2{x_offset, y_offset},
          transform.rect().height / 2.f, 1.f, raylib::RAYWHITE);
    }
  }
};

struct RenderPlayerHUD : System<Transform, HasHealth> {

  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             const HasHealth &hasHealth, float) const override {
    // Always render health bar
    const float scale_x = 2.f;
    const float scale_y = 1.25f;

    raylib::Color color = entity.has_child_of<HasColor>()
                              ? entity.get_with_child<HasColor>().color()
                              : raylib::GREEN;

    float health_as_percent = static_cast<float>(hasHealth.amount) /
                              static_cast<float>(hasHealth.max_amount);

    vec2 rotation_origin{0, 0};

    // Render the red background bar
    raylib::DrawRectanglePro(
        Rectangle{
            transform.pos().x - ((transform.size.x * scale_x) / 2.f) +
                5.f, // Center with scaling
            transform.pos().y -
                (transform.size.y + 10.0f),    // Slightly above the entity
            transform.size.x * scale_x,        // Adjust length
            (transform.size.y / 4.f) * scale_y // Adjust height
        },
        rotation_origin, 0.0f, raylib::RED);

    // Render the green health bar
    raylib::DrawRectanglePro(
        Rectangle{
            transform.pos().x - ((transform.size.x * scale_x) / 2.f) +
                5.f, // Start at the same position as red bar
            transform.pos().y -
                (transform.size.y + 10.0f), // Same vertical position as red bar
            (transform.size.x * scale_x) *
                health_as_percent, // Adjust length based on health percentage
            (transform.size.y / 4.f) * scale_y // Adjust height
        },
        rotation_origin, 0.0f, color);

    // Render round-specific information above health bar
    switch (RoundManager::get().active_round_type) {
    case RoundType::Lives:
      render_lives(entity, transform, color);
      break;
    case RoundType::Kills:
      render_kills(entity, transform, color);
      break;
    case RoundType::CatAndMouse:
      render_cat_indicator(entity, transform, color);
      break;
    default:
      break;
    }
  }

private:
  void render_lives(const Entity &entity, const Transform &transform,
                    raylib::Color color) const {
    if (!entity.has<HasMultipleLives>())
      return;

    const auto &hasMultipleLives = entity.get<HasMultipleLives>();
    float rad = 5.f;
    vec2 off{rad * 2 + 2, 0.f};
    for (int i = 0; i < hasMultipleLives.num_lives_remaining; i++) {
      raylib::DrawCircleV(
          transform.pos() -
              vec2{transform.size.x / 2.f, transform.size.y + 15.f + rad} +
              (off * (float)i),
          rad, color);
    }
  }

  void render_kills(const Entity &entity, const Transform &transform,
                    raylib::Color color) const {
    if (!entity.has<HasKillCountTracker>())
      return;

    const auto &hasKillCountTracker = entity.get<HasKillCountTracker>();
    std::string kills_text =
        std::to_string(hasKillCountTracker.kills) + " kills";
    float text_size = 12.f;

    raylib::DrawText(
        kills_text.c_str(), static_cast<int>(transform.pos().x - 30.f),
        static_cast<int>(transform.pos().y - transform.size.y - 25.f),
        static_cast<int>(text_size), color);
  }

  void render_cat_indicator(const Entity &entity, const Transform &transform,
                            raylib::Color) const {
    // TODO add color to entity
    if (!entity.has<HasCatMouseTracking>())
      return;

    const auto &catMouseTracking = entity.get<HasCatMouseTracking>();

    // Draw crown for cat
    if (catMouseTracking.is_cat) {
      // Draw a crown above the player who is "it"
      const float crown_size = 15.f;
      const float crown_y_offset = transform.size.y + 20.f;

      // Crown position (centered above the player)
      vec2 crown_pos = transform.pos() - vec2{crown_size / 2.f, crown_y_offset};

      // Draw crown using simple shapes
      raylib::Color crown_color = raylib::GOLD;

      // Crown base
      raylib::DrawRectangle(static_cast<int>(crown_pos.x),
                            static_cast<int>(crown_pos.y),
                            static_cast<int>(crown_size),
                            static_cast<int>(crown_size / 3.f), crown_color);

      // Crown points (3 triangles)
      float point_width = crown_size / 3.f;
      for (int i = 0; i < 3; i++) {
        float x = crown_pos.x + (i * point_width);
        raylib::DrawTriangle(
            vec2{x, crown_pos.y},
            vec2{x + point_width / 2.f, crown_pos.y - crown_size / 2.f},
            vec2{x + point_width, crown_pos.y}, crown_color);
      }

      // Crown jewels (small circles)
      raylib::DrawCircleV(crown_pos + vec2{crown_size / 2.f, crown_size / 6.f},
                          2.f, raylib::RED);
    }

    // Draw shield for players in cooldown (safe period)
    float current_time = static_cast<float>(raylib::GetTime());
    auto &cat_mouse_settings =
        RoundManager::get().get_active_rt<RoundCatAndMouseSettings>();
    if (current_time - catMouseTracking.last_tag_time <
        cat_mouse_settings.tag_cooldown_time) {
      // TODO: Add pulsing animation to shield to make it more obvious
      // TODO: Add countdown timer above shield showing remaining safe time
      // Draw a shield above the player who is safe
      const float shield_size = 12.f;
      const float shield_y_offset = transform.size.y + 35.f; // Above crown

      // Shield position (centered above the player)
      vec2 shield_pos =
          transform.pos() - vec2{shield_size / 2.f, shield_y_offset};

      // Draw shield using simple shapes
      raylib::Color shield_color = raylib::SKYBLUE;

      // Shield base (triangle pointing down)
      raylib::DrawTriangle(
          vec2{shield_pos.x + shield_size / 2.f, shield_pos.y}, // Top point
          vec2{shield_pos.x, shield_pos.y + shield_size},       // Bottom left
          vec2{shield_pos.x + shield_size,
               shield_pos.y + shield_size}, // Bottom right
          shield_color);

      // Shield border
      raylib::DrawTriangleLines(
          vec2{shield_pos.x + shield_size / 2.f, shield_pos.y}, // Top point
          vec2{shield_pos.x, shield_pos.y + shield_size},       // Bottom left
          vec2{shield_pos.x + shield_size,
               shield_pos.y + shield_size}, // Bottom right
          raylib::WHITE);
    }
  }
};

struct CheckLivesWinCondition : System<> {
  virtual void once(float) override {
    // Only check win conditions for Lives round type and when game is active
    if (RoundManager::get().active_round_type != RoundType::Lives) {
      return;
    }
    // Only run when game is active
    if (!GameStateManager::get().is_game_active()) {
      return;
    }
    // Count players with remaining lives
    auto players_with_lives =
        EntityQuery()
            .whereHasComponent<PlayerID>()
            .whereHasComponent<HasMultipleLives>()
            .whereLambda([](const Entity &e) {
              return e.get<HasMultipleLives>().num_lives_remaining > 0;
            })
            .gen();
    // If only one player has lives remaining, they win
    if (players_with_lives.size() == 1) {
      Entity &winner =
          players_with_lives[0].get(); // Fixed from players_with_lives[0]
      log_info("Player {} wins the Lives round!", winner.get<PlayerID>().id);
      GameStateManager::get().end_game();
    }
    // If no players have lives, it's a tie
    else if (players_with_lives.empty()) {
      log_info("All players eliminated - round is a tie!");
      GameStateManager::get().end_game();
    }
  }
};

struct UpdateCatMouseTimers : System<HasCatMouseTracking> {
  virtual void for_each_with(Entity &, HasCatMouseTracking &catMouseTracking,
                             float dt) override {
    if (!GameStateManager::get().is_game_active()) {
      return;
    }
    // Only increment mouse time for players who are not the cat
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

    // Only process if this entity is the cat
    if (!catMouseTracking.is_cat) {
      return;
    }

    // TODO: Add sound effect when cat tags mouse
    // TODO: Add particle effect or visual feedback when tag occurs
    // TODO: Consider different collision detection (maybe just front of car
    // hits back of car)

    // Find all other players (mice) that this cat can tag
    auto mice = EntityQuery()
                    .whereHasComponent<Transform>()
                    .whereHasComponent<HasCatMouseTracking>()
                    .whereLambda([](const Entity &e) {
                      return !e.get<HasCatMouseTracking>().is_cat;
                    })
                    .gen();

    // Check for collisions with mice
    for (auto &mouse_ref : mice) {
      Entity &mouse = mouse_ref.get();
      Transform &mouseTransform = mouse.get<Transform>();
      HasCatMouseTracking &mouseTracking = mouse.get<HasCatMouseTracking>();

      // Simple collision check (rectangular overlap)
      if (raylib::CheckCollisionRecs(transform.rect(), mouseTransform.rect())) {
        // Check if mouse has been tagged recently
        float current_time = static_cast<float>(raylib::GetTime());
        auto &cat_mouse_settings =
            RoundManager::get().get_active_rt<RoundCatAndMouseSettings>();
        if (current_time - mouseTracking.last_tag_time <
            cat_mouse_settings.tag_cooldown_time) {
          return; // Mouse is still in cooldown, can't be tagged
        }

        // Tag transfer: cat becomes mouse, mouse becomes cat
        catMouseTracking.is_cat = false;
        mouseTracking.is_cat = true;

        // Update tag times
        catMouseTracking.last_tag_time = current_time;
        mouseTracking.last_tag_time = current_time;

        std::string cat_id =
            entity.has<PlayerID>()
                ? "Player " + std::to_string(entity.get<PlayerID>().id)
                : "AI " + std::to_string(entity.id);
        std::string mouse_id =
            mouse.has<PlayerID>()
                ? "Player " + std::to_string(mouse.get<PlayerID>().id)
                : "AI " + std::to_string(mouse.id);

        log_info("{} tagged {}!", cat_id, mouse_id);
        return; // Only tag one mouse per frame
      }
    }
  }
};

struct InitializeCatMouseGame : System<> {
  bool initialized = false;
  // TODO: Add option to start with random cat vs. player with most kills from
  // previous round
  // TODO: Add option to start with player who was cat the longest in previous
  // round

  virtual void once(float) override {

    // Only run for Cat & Mouse round type
    if (RoundManager::get().active_round_type != RoundType::CatAndMouse) {
      return;
    }

    // Only run when game is active
    if (!GameStateManager::get().is_game_active()) {
      return;
    }

    // Only initialize once
    if (initialized) {
      return;
    }

    auto initial_cat =
        EntityQuery().whereHasComponent<HasCatMouseTracking>().gen_random();
    if (!initial_cat) {
      return;
    }

    // Initialize the game
    auto &cat_mouse_settings =
        RoundManager::get().get_active_rt<RoundCatAndMouseSettings>();

    cat_mouse_settings.state = RoundCatAndMouseSettings::GameState::Countdown;
    cat_mouse_settings.countdown_before_start = 3.0f; // Reset countdown
    cat_mouse_settings.reset_round_time();

    initial_cat->get<HasCatMouseTracking>().is_cat = true;

    std::string cat_id =
        initial_cat->has<PlayerID>()
            ? "Player " + std::to_string(initial_cat->get<PlayerID>().id)
            : "AI " + std::to_string(initial_cat->id);

    log_info("{} is the initial cat!", cat_id);

    initialized = true;
  }
};

struct CheckCatMouseWinCondition : System<> {
  virtual void once(float) override {
    // Only check win conditions for Cat & Mouse round type and when game is
    // active
    if (RoundManager::get().active_round_type != RoundType::CatAndMouse) {
      return;
    }
    // Only run when game is active
    if (!GameStateManager::get().is_game_active()) {
      return;
    }

    auto &cat_mouse_settings =
        RoundManager::get().get_active_rt<RoundCatAndMouseSettings>();

    // Check if time limit has been reached
    if (cat_mouse_settings.current_round_time > 0) {
      cat_mouse_settings.current_round_time -= raylib::GetFrameTime();
      if (cat_mouse_settings.current_round_time <= 0) {
        // Time's up! Find the player with the most mouse time (least cat time)
        auto players_with_tracking =
            EntityQuery().whereHasComponent<HasCatMouseTracking>().gen();

        if (players_with_tracking.empty()) {
          log_info("No players with tracking - round is a tie!");
          GameStateManager::get().end_game();
          return;
        }

        // Find the player with the most mouse time (least cat time)
        auto winner = std::max_element(
            players_with_tracking.begin(), players_with_tracking.end(),
            [](const auto &a, const auto &b) {
              return a.get().template get<HasCatMouseTracking>().time_as_mouse <
                     b.get().template get<HasCatMouseTracking>().time_as_mouse;
            });

        // Get winner identifier for logging
        std::string winner_id =
            winner->get().has<PlayerID>()
                ? "Player " + std::to_string(winner->get().get<PlayerID>().id)
                : "AI " + std::to_string(winner->get().id);

        log_info("{} wins the Cat & Mouse round with {:.1f}s mouse time!",
                 winner_id,
                 winner->get().get<HasCatMouseTracking>().time_as_mouse);

        cat_mouse_settings.state =
            RoundCatAndMouseSettings::GameState::GameOver;
        cat_mouse_settings.current_round_time = 0;

        // TODO: Add victory screen showing final mouse times for all players
        // TODO: Add option to continue playing (best of 3, etc.)
        GameStateManager::get().end_game();
      }
    }
  }
};

struct CheckKillsWinCondition : System<> {
  virtual void once(float) override {
    // Only check win conditions for Kills round type and when game is active
    if (RoundManager::get().active_round_type != RoundType::Kills) {
      return;
    }
    // Only run when game is active
    if (!GameStateManager::get().is_game_active()) {
      return;
    }

    auto &kills_settings =
        RoundManager::get().get_active_rt<RoundKillsSettings>();

    // Check if time limit has been reached
    if (kills_settings.current_round_time > 0) {
      kills_settings.current_round_time -= raylib::GetFrameTime();
      if (kills_settings.current_round_time <= 0) {
        // Time's up! Find the player with the most kills
        auto players_with_kills = EntityQuery()
                                      .whereHasComponent<PlayerID>()
                                      .whereHasComponent<HasKillCountTracker>()
                                      .gen();

        if (players_with_kills.empty()) {
          log_info("No players with kills - round is a tie!");
          GameStateManager::get().end_game();
          return;
        }

        // Find the player with the most kills
        auto winner = std::max_element(
            players_with_kills.begin(), players_with_kills.end(),
            [](const auto &a, const auto &b) {
              return a.get().template get<HasKillCountTracker>().kills <
                     b.get().template get<HasKillCountTracker>().kills;
            });

        log_info("Player {} wins the Kills round with {} kills!",
                 winner->get().get<PlayerID>().id,
                 winner->get().get<HasKillCountTracker>().kills);
        kills_settings.current_round_time = 0;
        GameStateManager::get().end_game();
      }
    }
  }
};

struct UpdateCatMouseCountdown : System<> {
  virtual void once(float dt) override {
    if (RoundManager::get().active_round_type != RoundType::CatAndMouse) {
      return;
    }

    if (!GameStateManager::get().is_game_active()) {
      return;
    }

    auto &cat_mouse_settings =
        RoundManager::get().get_active_rt<RoundCatAndMouseSettings>();

    if (cat_mouse_settings.state ==
        RoundCatAndMouseSettings::GameState::Countdown) {
      cat_mouse_settings.countdown_before_start -= dt;
      if (cat_mouse_settings.countdown_before_start <= 0) {
        cat_mouse_settings.countdown_before_start = 0;
        cat_mouse_settings.state = RoundCatAndMouseSettings::GameState::InGame;
        log_info("Cat & Mouse game starting!");
      }
    }
  }
};

struct RenderCatMouseTimer : System<window_manager::ProvidesCurrentResolution> {
  virtual void for_each_with(const Entity &,
                             const window_manager::ProvidesCurrentResolution &,
                             float) const override {

    if (RoundManager::get().active_round_type != RoundType::CatAndMouse) {
      return;
    }
    if (!GameStateManager::get().is_game_active()) {
      return;
    }

    auto &cat_mouse_settings =
        RoundManager::get().get_active_rt<RoundCatAndMouseSettings>();

    const int screen_width = raylib::GetScreenWidth();
    const int screen_height = raylib::GetScreenHeight();

    const float timer_x = screen_width * 0.5f;
    const float timer_y = screen_height * 0.07f;
    const float text_size = screen_height * 0.033f;
    const raylib::Color timer_color = raylib::WHITE;

    if (cat_mouse_settings.state ==
            RoundCatAndMouseSettings::GameState::InGame &&
        cat_mouse_settings.current_round_time > 0) {
      std::string timer_text;
      if (cat_mouse_settings.current_round_time >= 60.0f) {
        int minutes =
            truncate_to_minutes(cat_mouse_settings.current_round_time);
        int seconds =
            truncate_to_seconds(cat_mouse_settings.current_round_time);
        timer_text = std::format("{}:{:02d}", minutes, seconds);
      } else {
        timer_text =
            std::format("{:.1f}s", cat_mouse_settings.current_round_time);
      }
      const float text_width =
          raylib::MeasureText(timer_text.c_str(), static_cast<int>(text_size));
      raylib::DrawText(
          timer_text.c_str(), static_cast<int>(timer_x - text_width / 2.0f),
          static_cast<int>(timer_y), static_cast<int>(text_size), timer_color);
    }

    if (cat_mouse_settings.state ==
        RoundCatAndMouseSettings::GameState::Countdown) {
      std::string countdown_text = std::format(
          "Get Ready! {:.0f}", cat_mouse_settings.countdown_before_start);
      const float countdown_text_width = raylib::MeasureText(
          countdown_text.c_str(), static_cast<int>(text_size));
      raylib::DrawText(countdown_text.c_str(),
                       static_cast<int>(timer_x - countdown_text_width / 2.0f),
                       static_cast<int>(timer_y + screen_height * 0.056f),
                       static_cast<int>(text_size), raylib::YELLOW);
    }
  }
};
