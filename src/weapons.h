
#pragma once

#include "components.h"
#include "rl.h"
#include "sound_library.h"

struct Weapon {
  using FireFn = std::function<void(Entity &, Weapon &)>;

  enum struct Type {
    Cannon,
    Shotgun,
    Sniper,
    MachineGun,
  } type;

  struct Config {
    float cooldownReset;
    FireFn on_shoot;

    float knockback_amt = 0.25f;
    int base_damage = 1;

    vec2 size{10.f, 10.f};
    float speed{5.f};
    float acceleration{0.f};
    float life_time_seconds{10.f};
    float spread{0.f};
    bool can_wrap_around{true};
    bool render_out_of_bounds{false};
  } config;

  enum struct FiringDirection {
    Forward,
    Left,
    Right,
    Back,
  } firing_direction = FiringDirection::Forward;

  float cooldown;

  Weapon(const Type &type_, const Config &config_, const FiringDirection &fd)
      : type(type_), config(config_), firing_direction(fd), cooldown(0.f) {}

  virtual ~Weapon() {}

  virtual bool fire(float) {
    if (cooldown <= 0) {
      cooldown = config.cooldownReset;
      return true;
    }
    return false;
  }

  virtual bool pass_time(float dt) {
    if (cooldown <= 0) {
      return true;
    }
    cooldown -= dt;
    return false;
  }

  void apply_recoil(Transform &transform, const float knockback_amt) const {
    vec2 recoil = {std::cos(transform.as_rad()), std::sin(transform.as_rad())};

    recoil = vec_norm(vec2{-recoil.y, recoil.x});
    transform.velocity += (recoil * knockback_amt);
  }
};

void make_poof_anim(Entity &, const Weapon &, float angle_offset = 0);
void make_bullet(Entity &, const Weapon &, float angle_offset = 0);

struct Cannon : Weapon {

  Cannon(const Weapon::FiringDirection &fd)
      : Weapon(Weapon::Type::Cannon, //
               Weapon::Config{
                   .cooldownReset = 1.f,
                   .on_shoot =
                       [](Entity &parent, Weapon &wp) {
                         if (auto opt = EntityQuery({.force_merge = true})
                                            .whereHasComponent<SoundEmitter>()
                                            .gen_first();
                             opt.valid()) {
                           auto &ent = opt.asE();
                           auto &req =
                               ent.addComponentIfMissing<PlaySoundRequest>();
                           req.policy = PlaySoundRequest::Policy::Enum;
                           req.file = SoundFile::Weapon_Canon_Shot;
                         }
                         make_poof_anim(parent, wp);
                         make_bullet(parent, wp);
                         wp.apply_recoil(parent.get<Transform>(),
                                         wp.config.knockback_amt);
                       },
                   .knockback_amt = 0.25f,
                   .base_damage = kill_shots_to_base_dmg(3),
               },
               fd) {}
};

struct Sniper : Weapon {

  Sniper(const Weapon::FiringDirection &fd)
      : Weapon(Weapon::Type::Sniper, //
               Weapon::Config{
                   .cooldownReset = 3.f,
                   .on_shoot =
                       [](Entity &parent, Weapon &wp) {
                         if (auto opt = EntityQuery({.force_merge = true})
                                            .whereHasComponent<SoundEmitter>()
                                            .gen_first();
                             opt.valid()) {
                           auto &ent = opt.asE();
                           auto &req =
                               ent.addComponentIfMissing<PlaySoundRequest>();
                           req.policy = PlaySoundRequest::Policy::Enum;
                           req.file = SoundFile::Weapon_Sniper_Shot;
                         }
                         make_poof_anim(parent, wp);
                         make_bullet(parent, wp);
                         wp.apply_recoil(parent.get<Transform>(),
                                         wp.config.knockback_amt);
                       },
                   .knockback_amt = 0.50f,
                   .base_damage = kill_shots_to_base_dmg(1),
               },
               fd) {}
};

struct Shotgun : Weapon {

  Shotgun(const Weapon::FiringDirection &fd)
      : Weapon(Weapon::Type::Shotgun, //
               Weapon::Config{
                   .cooldownReset = 3.f,
                   .on_shoot =
                       [](Entity &parent, Weapon &wp) {
                         if (auto opt = EntityQuery({.force_merge = true})
                                            .whereHasComponent<SoundEmitter>()
                                            .gen_first();
                             opt.valid()) {
                           auto &ent = opt.asE();
                           auto &req =
                               ent.addComponentIfMissing<PlaySoundRequest>();
                           req.policy = PlaySoundRequest::Policy::Enum;
                           req.file = SoundFile::Weapon_Shotgun_Shot;
                         }
                         // TODO more poofs
                         make_poof_anim(parent, wp);
                         make_bullet(parent, wp, -15);
                         make_bullet(parent, wp, -5);
                         make_bullet(parent, wp, 5);
                         make_bullet(parent, wp, 15);
                         wp.apply_recoil(parent.get<Transform>(),
                                         wp.config.knockback_amt);
                       },
                   .knockback_amt = 0.50f,
                   // This is per bullet
                   .base_damage = kill_shots_to_base_dmg(4),
               },
               fd) {}
};

struct MachineGun : Weapon {

  MachineGun(const Weapon::FiringDirection &fd)
      : Weapon(Weapon::Type::MachineGun, //
               Weapon::Config{
                   .cooldownReset = 0.2f,
                   .on_shoot =
                       [](Entity &parent, Weapon &wp) {
                         if (auto opt = EntityQuery({.force_merge = true})
                                            .whereHasComponent<SoundEmitter>()
                                            .gen_first();
                             opt.valid()) {
                           auto &ent = opt.asE();
                           auto &req =
                               ent.addComponentIfMissing<PlaySoundRequest>();
                           req.policy = PlaySoundRequest::Policy::PrefixRandom;
                           req.prefix =
                               "SPAS-12_-_FIRING_-_Pump_Action_-_Take_1_-"
                               "_20m_In_Front_-_AB_-_MKH8020_";
                         }
                         make_poof_anim(parent, wp);
                         make_bullet(parent, wp);
                         wp.apply_recoil(parent.get<Transform>(),
                                         wp.config.knockback_amt);
                       },
                   .knockback_amt = 0.1f,
                   .base_damage = kill_shots_to_base_dmg(12),
                   .size = vec2{10., 10.f},
                   .speed = ::Config::get().machine_gun_fire_rate.data,
                   .acceleration = 2.f,
                   .life_time_seconds = 1.f,
                   .spread = 1.f,
                   .can_wrap_around = false,
                   .render_out_of_bounds = false},
               fd) {}
};

struct CanShoot : BaseComponent {
  std::map<InputAction, std::unique_ptr<Weapon>> weapons;

  CanShoot() = default;

  auto &register_weapon(InputAction action,
                        const Weapon::FiringDirection &direction,
                        const Weapon::Type &type) {
    switch (type) {
    case Weapon::Type::Cannon: {
      weapons[action] = std::make_unique<Cannon>(direction);
    } break;
    case Weapon::Type::Sniper: {
      weapons[action] = std::make_unique<Sniper>(direction);
    } break;
    case Weapon::Type::Shotgun: {
      weapons[action] = std::make_unique<Shotgun>(direction);
    } break;
    case Weapon::Type::MachineGun: {
      weapons[action] = std::make_unique<MachineGun>(direction);
    } break;
    default:
      log_warn(
          "Trying to register weapon of type {} but not supported by CanShoot",
          magic_enum::enum_name<Weapon::Type>(type));
      break;
    }
    return *this;
  }

  bool pass_time(InputAction action, float dt) {
    // TODO add warning
    if (!weapons.contains(action))
      return false;
    return weapons[action]->pass_time(dt);
  }

  bool fire(Entity &parent, InputAction action, float dt) {
    // TODO add warning
    if (!weapons.contains(action))
      return false;
    if (weapons[action]->fire(dt)) {
      weapons[action]->config.on_shoot(parent, *weapons[action]);
      return true;
    }
    return false;
  }
};

// TODO add a macro that can do these for enums we love
constexpr static auto WEAPON_LIST = magic_enum::enum_values<Weapon::Type>();
constexpr static auto WEAPON_STRING_LIST =
    magic_enum::enum_names<Weapon::Type>();
constexpr static size_t WEAPON_COUNT = magic_enum::enum_count<Weapon::Type>();
using WeaponSet = std::bitset<WEAPON_COUNT>;

constexpr static std::array<std::pair<int, int>, WEAPON_COUNT>
    WEAPON_ICON_COORDS = {
        /* Cannon      */ std::pair{0, 5},
        /* Shotgun     */ std::pair{1, 5},
        /* Sniper      */ std::pair{2, 5},
        /* MachineGun  */ std::pair{3, 5},
};

inline afterhours::texture_manager::Rectangle
weapon_icon_frame(Weapon::Type t) {
  auto idx = static_cast<size_t>(t);
  auto [col, row] = WEAPON_ICON_COORDS[idx];
  return afterhours::texture_manager::idx_to_sprite_frame(col, row);
}
