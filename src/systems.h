
#pragma once

#include "components.h"
#include "query.h"

struct RenderEntities : System<Transform> {

  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             float) const override {
    if (entity.has<HasTexture>())
      return;
    if (entity.has<HasAnimation>())
      return;

    auto entitiy_color = entity.has_child_of<HasColor>()
                             ? entity.get_with_child<HasColor>().color
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
    hasEntityIDBasedColor.color = hasEntityIDBasedColor.default_;
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

struct RenderSprites : System<Transform, HasSprite> {

  raylib::Texture2D sheet;

  virtual void once(float) {
    sheet = EQ().whereHasComponent<HasTexture>()
                .gen_first_enforce()
                .get<HasTexture>()
                .texture;
  }

  virtual void for_each_with(const Entity &, const Transform &transform,
                             const HasSprite &hasSprite, float) const override {

    raylib::DrawTexturePro(sheet, hasSprite.frame,
                           Rectangle{
                               transform.center().x,
                               transform.center().y,
                               hasSprite.frame.width * hasSprite.scale,
                               hasSprite.frame.height * hasSprite.scale,
                           },
                           vec2{transform.size.x / 2.f,
                                transform.size.y / 2.f}, // transform.center(),
                           transform.angle, hasSprite.colorTint);
  }
};

struct AnimationUpdateCurrentFrame : System<HasAnimation> {

  virtual void for_each_with(Entity &entity, HasAnimation &hasAnimation,
                             float dt) override {
    hasAnimation.frame_time -= dt;
    if (hasAnimation.frame_time > 0) {
      return;
    }
    hasAnimation.frame_time = hasAnimation.frame_dur;

    if (hasAnimation.cur_frame >= hasAnimation.total_frames) {
      if (hasAnimation.once) {
        entity.cleanup = true;
        return;
      }
      hasAnimation.cur_frame = 0;
    }

    auto [i, j] =
        idx_to_next_sprite_location((int)hasAnimation.cur_frame_position.x,
                                    (int)hasAnimation.cur_frame_position.y);

    hasAnimation.cur_frame_position = vec2{(float)i, (float)j};
    hasAnimation.cur_frame++;
  }
};

struct RenderAnimation : System<Transform, HasAnimation> {

  raylib::Texture2D sheet;

  virtual void once(float) override {
    sheet = EntityHelper::get_singleton_cmp<HasTexture>()->texture;
  }

  virtual void for_each_with(const Entity &, const Transform &transform,
                             const HasAnimation &hasAnimation,
                             float) const override {

    auto [i, j] = hasAnimation.cur_frame_position;
    Rectangle frame = idx_to_sprite_frame((int)i, (int)j);

    raylib::DrawTexturePro(
        sheet, frame,
        Rectangle{
            transform.center().x,
            transform.center().y,
            frame.width * hasAnimation.scale,
            frame.height * hasAnimation.scale,
        },
        vec2{frame.width / 2.f, frame.height / 2.f}, // transform.center(),
        transform.angle + hasAnimation.rotation, raylib::RAYWHITE);
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

  virtual void for_each_with(Entity &entity, PlayerID &playerID, Transform &,
                             CanShoot &canShoot, float dt) override {

    magic_enum::enum_for_each<InputAction>([&](auto val) {
      constexpr InputAction action = val;
      canShoot.pass_time(action, dt);
    });

    input::PossibleInputCollector<InputAction> inpc =
        input::get_input_collector<InputAction>();
    if (!inpc.has_value()) {
      return;
    }

    for (auto &actions_done : inpc.inputs()) {
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

  virtual void for_each_with(Entity &, Transform &transform,
                             CanWrapAround &canWrap, float) override {

    float width = (float)resolution.width;
    float height = (float)resolution.height;

    float padding = canWrap.padding;

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
          std::fabs(dot) < (config.skid_threshold.data / 100.f);

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
    if (is_point_inside(transform.pos(), screen))
      return;

    float size =
        std::max(5.f, //
                 std::lerp(20.f, 5.f,
                           (distance_sq(transform.pos(), rect_center(screen)) /
                            (screen.width * screen.height))));

    raylib::DrawCircleV(calc(screen, transform.pos()), size,
                        entity.has<HasColor>() ? entity.get<HasColor>().color
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

  void resolve_collision(Transform &a, Transform &b) {
    vec2 collisionNormal = vec_norm(b.position - a.position);
    float impulse = calculate_impulse(a, b, collisionNormal);
    a.velocity += collisionNormal * impulse / a.mass;
    b.velocity -= collisionNormal * impulse / b.mass;
  }

  float calculate_impulse(Transform &a, Transform &b, vec2 collisionNormal) {
    vec2 relativeVelocity = b.velocity - a.velocity;
    float impulse =
        vec_dot(-relativeVelocity, collisionNormal) / (1 / a.mass + 1 / b.mass);

    // scale it down
    return impulse * 0.1f;
  }

  virtual void for_each_with(Entity &entity, Transform &transform, CanShoot &,
                             float) override {

    // skip any already resolved
    if (ids.contains(entity.id))
      return;

    auto can_shoot = EQ().whereHasComponent<CanShoot>()
                         .whereNotID(entity.id)
                         .whereOverlaps(transform.rect())
                         .gen();

    for (Entity &other : can_shoot) {
      Transform &b = other.get<Transform>();
      resolve_collision(transform, b);
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

    const auto is_reversing = transform.speed_dot_angle < 0.f;

    for (auto &actions_done : inpc.inputs()) {
      if (actions_done.id != playerID.id) {
        continue;
      }

      switch (actions_done.action) {
      case InputAction::Accel:
        transform.accel = is_reversing ? -1.f : 2.f;
        break;
      case InputAction::Brake:
        transform.accel = is_reversing ? -5.f : -1.75f;
        break;
      case InputAction::Left:
        steer = -actions_done.amount_pressed;
        break;
      case InputAction::Right:
        steer = actions_done.amount_pressed;
        break;
      default:
        break;
      }
    }

    const auto minRadius = 10.f;
    const auto maxRadius = 300.f;
    const auto rad = std::lerp(minRadius, maxRadius,
                               transform.speed() / config.max_speed.data);

    transform.angle += steer * config.steering_sensitivity.data * dt * rad;
    transform.angle = std::fmod(transform.angle + 360.f, 360.f);

    const auto mvt = std::max(
        -config.max_speed.data,
        std::min(config.max_speed.data, transform.speed() + transform.accel));

    transform.velocity += vec2{
        std::sin(transform.as_rad()) * mvt * dt,
        -std::cos(transform.as_rad()) * mvt * dt,
    };

    transform.speed_dot_angle =
        transform.velocity.x * std::sin(transform.as_rad()) +
        transform.velocity.y * -std::cos(transform.as_rad());

    if (is_reversing) {
      transform.speed_dot_angle = -transform.speed_dot_angle;
    }
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
                          transform.speed() / config.max_speed.data);

    transform.angle = ang;

    float mvt =
        std::max(-config.max_speed.data,
                 std::min(config.max_speed.data, transform.speed() + accel));

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
      CanDamage &cd = damager.get<CanDamage>();
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
      transform.position = get_spawn_position((int)entity.get<PlayerID>().id,
                                              // TODO use current resolution
                                              raylib::GetRenderWidth(),
                                              raylib::GetRenderHeight());
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
                              ? entity.get_with_child<HasColor>().color
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
