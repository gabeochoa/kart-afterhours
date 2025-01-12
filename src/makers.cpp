

#include "makers.h"

#include "components.h"

using afterhours::texture_manager::HasAnimation;
using afterhours::texture_manager::idx_to_sprite_frame;

void make_explosion_anim(Entity &parent) {
  const Transform &parent_transform = parent.get<Transform>();

  auto &poof = EntityHelper::createEntity();
  poof.addComponent<Transform>(parent_transform.pos(), vec2{10.f, 10.f});

  const Transform &transform = poof.get<Transform>();
  poof.addComponent<HasAnimation>(transform.position, transform.size,
                                  transform.angle, vec2{0, 3},
                                  9,          // total_frames
                                  1.f / 20.f, // frame_dur
                                  true,       // once
                                  2.f,        // scale
                                  0, 0, raylib::RAYWHITE);
}

void make_poof_anim(Entity &parent, const Weapon &wp, float angle_offset) {
  const Transform &parent_transform = parent.get<Transform>();

  vec2 off;
  float angle = 0;
  switch (wp.firing_direction) {
  case Weapon::FiringDirection::Forward:
    // TODO
    off = {0.f, 0.f};
    angle = 0.f;
    break;
  case Weapon::FiringDirection::Left:
    off = {
        -20.f,
        10.f,
    };
    angle = -90.f;
    break;
  case Weapon::FiringDirection::Right:
    off = {
        20.f,
        10.f,
    };
    angle = 90.f;
    break;
  case Weapon::FiringDirection::Back:
    // TODO
    off = {0.f, 0.f};
    angle = 180.f;
    break;
  }

  auto &poof = EntityHelper::createEntity();
  poof.addComponent<TracksEntity>(parent.id, off);
  poof.addComponent<Transform>(parent_transform.pos() + off, vec2{10.f, 10.f})
      .set_angle(parent_transform.angle + angle_offset);
  const Transform &transform = poof.get<Transform>();
  poof.addComponent<HasAnimation>(transform.position, transform.size,
                                  transform.angle, vec2{0, 0},
                                  14,         // total_frames
                                  1.f / 20.f, // frame_dur
                                  true,       // once
                                  1.f, 0, angle, raylib::RAYWHITE);
}

void make_bullet(Entity &parent, const Weapon &wp, float angle_offset) {
  const Transform &transform = parent.get<Transform>();

  auto angle = 0.f;
  switch (wp.firing_direction) {
  case Weapon::FiringDirection::Forward:
    break;
  case Weapon::FiringDirection::Left:
    angle = -90.f;
    break;
  case Weapon::FiringDirection::Right:
    angle = 90.f;
    break;
  case Weapon::FiringDirection::Back:
    angle = 180.f;
    break;
  }

  if (wp.config.spread > 0.f) {
    std::mt19937_64 rng;
    const auto timeSeed =
        std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::seed_seq ss{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed >> 32)};
    rng.seed(ss);
    std::uniform_real_distribution<float> unif(-wp.config.spread,
                                               wp.config.spread);
    angle_offset += wp.config.size.x * unif(rng);
  }

  vec2 spawn_bias{0, wp.config.size.y};
  auto bullet_spawn_pos = transform.pos() + spawn_bias;

  auto &bullet = EntityHelper::createEntity();
  bullet.addComponent<Transform>(bullet_spawn_pos, wp.config.size)
      .set_angle(angle_offset);

  bullet.addComponent<CanDamage>(parent.id, wp.config.base_damage);
  bullet.addComponent<HasLifetime>(wp.config.life_time_seconds);

  // If the config doesn't want wrapping, this will make the bullet go into the
  // "void"
  auto wrap_padding =
      wp.config.can_wrap_around ? 0.f : std::numeric_limits<float>::max();
  bullet.addComponent<CanWrapAround>(wrap_padding);

  bullet.addComponent<HasEntityIDBasedColor>(
      parent.id, parent.get<HasColor>().color(), raylib::RED);

  const auto rad = transform.as_rad() + to_radians(angle + angle_offset);
  auto &bullet_transform = bullet.get<Transform>();
  bullet_transform.velocity =
      vec2{std::sin(rad) * wp.config.speed, -std::cos(rad) * wp.config.speed};
  bullet_transform.accel = wp.config.acceleration;
  bullet_transform.render_out_of_bounds =
      wp.config.can_wrap_around && wp.config.render_out_of_bounds;
  bullet_transform.cleanup_out_of_bounds = !wp.config.can_wrap_around;
}

Entity &make_car(size_t id) {
  auto &entity = EntityHelper::createEntity();

  entity.addComponent<HasMultipleLives>(3);
  auto &transform = entity.addComponent<Transform>(
      get_spawn_position((size_t)id,
                         // TODO use current resolution
                         raylib::GetRenderWidth(), raylib::GetRenderHeight()),
      vec2{15.f, 25.f});
  transform.mass = 1000.f;
  transform.restitution = .1f;
  transform.friction = 0.75f;

  entity.addComponent<CanWrapAround>();
  entity.addComponent<HasHealth>(MAX_HEALTH);
  entity.addComponent<TireMarkComponent>();
  entity.addComponent<HasColor>([&entity]() -> raylib::Color {
    return EntityHelper::get_singleton_cmp<ManagesAvailableColors>()
        ->get_next_available(entity.id);
  });
  entity.addComponent<afterhours::texture_manager::HasSprite>(
      transform.position, transform.size, transform.angle,
      idx_to_sprite_frame(0, 1), 1.f, entity.get<HasColor>().color());

  entity.addComponent<CanShoot>()
      .register_weapon(InputAction::ShootLeft, Weapon::FiringDirection::Forward,
                       Weapon::Type::Shotgun)
      .register_weapon(InputAction::ShootRight,
                       Weapon::FiringDirection::Forward,
                       Weapon::Type::MachineGun);
  // .register_weapon(InputAction::ShootLeft, Weapon::FiringDirection::Left,
  // Weapon::Type::Cannon)
  //.register_weapon(InputAction::ShootRight, Weapon::FiringDirection::Right,
  //                 Weapon::Type::Cannon);

  return entity;
}

void make_player(input::GamepadID id) {
  auto &entity = make_car(id);
  entity.addComponent<PlayerID>(id);

  const auto player_id_text = "[Player " + std::to_string(id) + "]";
  const auto player_label_pos_offset = vec2{-.1f, 0.f};
  LabelInfo player_text_label_info(player_id_text, player_label_pos_offset,
                                   LabelInfo::LabelType::StaticText);

  const auto velocity_unit_text = " m/s";
  const auto velocity_label_pos_offset = vec2{2.25f, 1.0f};
  LabelInfo velocity_text_label_info(velocity_unit_text,
                                     velocity_label_pos_offset,
                                     LabelInfo::LabelType::VelocityText);

  const auto acceleration_unit_text = " m/s^2";
  const auto acceleration_label_pos_offset = vec2{2.25f, 2.0f};
  LabelInfo acceleration_text_label_info(
      acceleration_unit_text, acceleration_label_pos_offset,
      LabelInfo::LabelType::AccelerationText);

  std::vector<LabelInfo> player_labels{std::move(player_text_label_info),
                                       std::move(velocity_text_label_info),
                                       std::move(acceleration_text_label_info)};

  entity.addComponent<HasLabels>(std::move(player_labels));
}

void make_ai() {
  // force merge because we are creating entities not inside a system
  // and theres an ent query inside
  size_t num_players = EntityQuery(true /* force merge*/)
                           .whereHasComponent<PlayerID>()
                           .gen_count();
  size_t num_ais = EntityQuery(true /* force merge*/)
                       .whereHasComponent<AIControlled>()
                       .gen_count();
  auto &entity = make_car(num_players + num_ais);
  entity.addComponent<AIControlled>();
}
