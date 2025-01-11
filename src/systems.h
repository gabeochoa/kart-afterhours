
#pragma once

#include "components.h"
#include "makers.h"
#include "query.h"

struct UpdateSpriteTransform
    : System<Transform, afterhours::texture_manager::HasSprite> {

  virtual void for_each_with(Entity &, Transform &transform,
                             afterhours::texture_manager::HasSprite &hasSprite,
                             float) override {
    hasSprite.update_transform(transform.position, transform.size,
                               transform.angle);
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

    // we need to add a new player

    for (int i = 0; i < (int)maxGamepadID.count() + 1; i++) {
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

struct Shoot : System<PlayerID, Transform, CanShoot> {
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

struct UpdateCollidingEntities : System<Transform, CanShoot> {

  std::set<int> ids;

  virtual void once(float) override { ids.clear(); }

  void positional_correction(Transform &a, Transform &b,
                             const vec2 &collisionNormal,
                             float penetrationDepth) {
    float correctionMagnitude =
        std::max(penetrationDepth, 0.0f) / (1.0f / a.mass + 1.0f / b.mass);
    vec2 correction = collisionNormal * correctionMagnitude;

    a.position -= correction / a.mass;
    b.position += correction / b.mass;
  }

  void resolve_collision(Transform &a, Transform &b, const float dt) {
    vec2 collisionNormal = vec_norm(b.position - a.position);

    // Calculate normal impulse
    float impulse = calculate_impulse(a, b, collisionNormal);
    vec2 impulseVector =
        collisionNormal * impulse * Config::get().collision_scalar.data * dt;

    // Apply normal impulse
    if (a.mass > 0.0f)
      a.velocity -= impulseVector / a.mass;
    if (b.mass > 0.0f)
      b.velocity += impulseVector / b.mass;

    // Calculate and apply friction impulse
    vec2 relativeVelocity = b.velocity - a.velocity;
    vec2 tangent =
        vec_norm(relativeVelocity -
                 collisionNormal * vec_dot(relativeVelocity, collisionNormal));

    float frictionImpulseMagnitude =
        vec_dot(relativeVelocity, tangent) / (1.0f / a.mass + 1.0f / b.mass);
    float frictionCoefficient = std::sqrt(a.friction * b.friction);
    frictionImpulseMagnitude =
        std::clamp(frictionImpulseMagnitude, -impulse * frictionCoefficient,
                   impulse * frictionCoefficient);

    vec2 frictionImpulse = tangent * frictionImpulseMagnitude *
                           Config::get().collision_scalar.data * dt;

    if (a.mass > 0.0f)
      a.velocity -= frictionImpulse / a.mass;
    if (b.mass > 0.0f)
      b.velocity += frictionImpulse / b.mass;

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
    float baseRestitution = std::min(a.restitution, b.restitution);

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
    impulse /= (1.0f / a.mass + 1.0f / b.mass);

    return impulse;
  }

  virtual void for_each_with(Entity &entity, Transform &transform, CanShoot &,
                             float dt) override {

    // skip any already resolved
    if (ids.contains(entity.id))
      return;

    auto can_shoot = EQ().whereHasComponent<CanShoot>()
                         .whereNotID(entity.id)
                         .whereOverlaps(transform.rect())
                         .gen();

    for (Entity &other : can_shoot) {
      Transform &b = other.get<Transform>();
      resolve_collision(transform, b, dt);
      ids.insert(other.id);
    }
  }
};

struct VelFromInput : System<PlayerID, Transform> {
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

struct AIVelocity : System<AIControlled, Transform> {

  virtual void for_each_with(Entity &, AIControlled &ai, Transform &transform,
                             float dt) override {

    // bool needs_target = (ai.target.x == 0 && ai.target.y == 0) ||
    // distance_sq(ai.target, transform.pos()) < 10.f;
    auto opt_entity = EQ().whereHasComponent<PlayerID>().gen_first();
    if (opt_entity.valid()) {
      ai.target = opt_entity->get<Transform>().pos();
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

struct Move : System<Transform> {

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

struct ProcessDamage : System<Transform, HasHealth> {

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
      damager.cleanup = true;
    }
  }
};

struct ProcessDeath : System<Transform, HasHealth> {

  virtual void for_each_with(Entity &entity, Transform &transform,
                             HasHealth &hasHealth, float) override {
    if (hasHealth.amount > 0) {
      return;
    }

    make_explosion_anim(entity);

    // TODO find a better place to do this
    if (entity.has<PlayerID>()) {
      transform.position = get_spawn_position(
          static_cast<size_t>(entity.get<PlayerID>().id),
          // TODO use current resolution
          raylib::GetRenderWidth(), raylib::GetRenderHeight());
    }

    if (entity.has<HasMultipleLives>()) {
      entity.get<HasMultipleLives>().num_lives_remaining -= 1;
      if (entity.get<HasMultipleLives>().num_lives_remaining) {
        hasHealth.amount = hasHealth.max_amount;
        return;
      }
    }

    entity.cleanup = true;
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

struct RenderHealthAndLives : System<Transform, HasHealth, HasMultipleLives> {

  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             const HasHealth &hasHealth,
                             const HasMultipleLives &hasMultipleLives,
                             float) const override {
    // Calculate the percentage of the height by the percentage of the health
    // max against its current value Scaling factors for bar length and height
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
};
