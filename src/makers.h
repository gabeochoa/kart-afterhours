
#pragma once

#include "components.h"

void make_explosion_anim(Entity &parent) {
  const Transform &transform = parent.get<Transform>();

  auto &poof = EntityHelper::createEntity();
  poof.addComponent<Transform>(transform.pos(), vec2{10.f, 10.f});

  poof.addComponent<HasAnimation>(vec2{0, 3},
                                  9,          // total_frames
                                  1.f / 20.f, // frame_dur
                                  true,       // once
                                  2.f,        // scale
                                  0, 0);
}

void make_poof_anim(Entity &parent, const Weapon &wp, float angle_offset) {
  const Transform &transform = parent.get<Transform>();

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
  poof.addComponent<Transform>(transform.pos() + off, vec2{10.f, 10.f})
      .set_angle(transform.angle + angle_offset);
  poof.addComponent<HasAnimation>(vec2{0, 0},
                                  14,         // total_frames
                                  1.f / 20.f, // frame_dur
                                  true,       // once
                                  1.f, 0, angle);
}

void make_bullet(Entity &parent, const Weapon &wp, float angle_offset) {
  const Transform &transform = parent.get<Transform>();

  float angle = 0;
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

  auto &bullet = EntityHelper::createEntity();
  bullet
      .addComponent<Transform>(transform.pos() + vec2{0, 10.f},
                               vec2{10.f, 10.f})
      .set_angle(angle_offset);

  bullet.addComponent<CanDamage>(parent.id, wp.config.base_damage);
  bullet.addComponent<HasLifetime>(10.f);

  bullet.addComponent<CanWrapAround>(0.f);

  bullet.addComponent<HasEntityIDBasedColor>(
      parent.id, parent.get<HasColor>().color, raylib::RED);

  float rad = transform.as_rad() + to_radians(angle + angle_offset);
  bullet.get<Transform>().velocity =
      vec2{std::sin(rad) * 5.f, -std::cos(rad) * 5.f};
}

Entity &make_car(int id) {
  auto &entity = EntityHelper::createEntity();

  entity.addComponent<HasMultipleLives>(3);
  entity.addComponent<Transform>(
      get_spawn_position((size_t)id,
                         // TODO use current resolution
                         raylib::GetRenderWidth(), raylib::GetRenderHeight()),
      vec2{15.f, 25.f});

  entity.addComponent<CanWrapAround>();
  entity.addComponent<HasHealth>(MAX_HEALTH);
  entity.addComponent<TireMarkComponent>();
  auto tint = get_color_for_player((size_t)id);
  entity.addComponent<HasColor>(tint);
  entity.addComponent<HasSprite>(idx_to_sprite_frame(0, 1), 1.f, tint);

  entity.addComponent<CanShoot>()
      .register_weapon(InputAction::ShootLeft, Weapon::FiringDirection::Forward,
                       Weapon::Type::Shotgun)
      // .register_weapon(InputAction::ShootLeft, Weapon::FiringDirection::Left,
      // Weapon::Type::Cannon)
      .register_weapon(InputAction::ShootRight, Weapon::FiringDirection::Right,
                       Weapon::Type::Cannon);

  return entity;
}

void make_player(input::GamepadID id) {
  auto &entity = make_car(next_id++);
  entity.addComponent<PlayerID>(id);
}

void make_ai() {
  auto &entity = make_car(next_id++);
  entity.addComponent<AIControlled>();
}
