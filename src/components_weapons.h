#pragma once

#include "input_mapping.h"
#include "rl.h"

using namespace afterhours;

struct RecoilConfig {
  float knockback_amt{0.f};
  RecoilConfig() = default;
  explicit RecoilConfig(float amt) : knockback_amt(amt) {}
};

struct ProjectileConfig {
  vec2 size{10.f, 10.f};
  float speed{5.f};
  float acceleration{0.f};
  float life_time_seconds{10.f};
  float spread{0.f};
  bool can_wrap_around{true};
  bool render_out_of_bounds{false};
  int base_damage{1};
  std::vector<float> angle_offsets{0.f};

  ProjectileConfig() = default;
  ProjectileConfig(vec2 sz, float spd, float accel, float life_sec, float spr,
                   bool can_wrap, bool render_oob, int dmg,
                   std::vector<float> angles)
      : size(sz), speed(spd), acceleration(accel), life_time_seconds(life_sec),
        spread(spr), can_wrap_around(can_wrap),
        render_out_of_bounds(render_oob), base_damage(dmg),
        angle_offsets(std::move(angles)) {}

  struct Builder;
  static Builder builder();
};

struct ProjectileConfig::Builder {
  ProjectileConfig cfg{};
  Builder &with_size(vec2 v) {
    cfg.size = v;
    return *this;
  }
  Builder &with_speed(float v) {
    cfg.speed = v;
    return *this;
  }
  Builder &with_acceleration(float v) {
    cfg.acceleration = v;
    return *this;
  }
  Builder &with_lifetime(float v) {
    cfg.life_time_seconds = v;
    return *this;
  }
  Builder &with_spread(float v) {
    cfg.spread = v;
    return *this;
  }
  Builder &with_can_wrap(bool v) {
    cfg.can_wrap_around = v;
    return *this;
  }
  Builder &with_render_out_of_bounds(bool v) {
    cfg.render_out_of_bounds = v;
    return *this;
  }
  Builder &with_base_damage(int v) {
    cfg.base_damage = v;
    return *this;
  }
  Builder &with_angle_offsets(std::vector<float> v) {
    cfg.angle_offsets = std::move(v);
    return *this;
  }
  Builder &add_angle_offset(float v) {
    cfg.angle_offsets.push_back(v);
    return *this;
  }
  ProjectileConfig build() && { return std::move(cfg); }
  ProjectileConfig build() & { return std::move(cfg); }
};

inline ProjectileConfig::Builder ProjectileConfig::builder() {
  return Builder{};
}

struct WantsWeaponFire : BaseComponent {
  InputAction action;
  WantsWeaponFire() = default;
  WantsWeaponFire(WantsWeaponFire&&) = default;
  WantsWeaponFire& operator=(WantsWeaponFire&&) = default;
  explicit WantsWeaponFire(InputAction a) : action(a) {}
};

struct WeaponFired : BaseComponent {
  int weapon_type{0};
  int firing_direction{0};
  ProjectileConfig projectile;
  RecoilConfig recoil;
  InputAction action{};

  WeaponFired() = default;
  WeaponFired(WeaponFired&&) = default;
  WeaponFired& operator=(WeaponFired&&) = default;
  WeaponFired(InputAction act, int weapon_type_in, int firing_direction_in,
              ProjectileConfig proj, RecoilConfig rec)
      : weapon_type(weapon_type_in), firing_direction(firing_direction_in),
        projectile(std::move(proj)), recoil(std::move(rec)), action(act) {}
};