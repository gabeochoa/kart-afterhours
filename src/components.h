
#pragma once

#include <map>
#include <memory>

#include "input_mapping.h"
#include "math_util.h"
#include "max_health.h"
#include "rl.h"

// Note: This must be included after std includes
#include "config.h"

constexpr static vec2 get_spawn_position(size_t id, int width, int height) {
  constexpr std::array<vec2, input::MAX_GAMEPAD_ID> pct_location = {{
      vec2{0.1f, 0.5f},
      vec2{0.9f, 0.5f},
      //
      vec2{0.1f, 0.1f},
      vec2{0.9f, 0.1f},
      //
      vec2{0.1f, 0.9f},
      vec2{0.9f, 0.9f},
      //
      vec2{0.5f, 0.1f},
      vec2{0.5f, 0.9f},
  }};
  size_t index = (id) % pct_location.size();
  vec2 pct = pct_location[index];
  return vec2{
      pct.x * (float)width,
      pct.y * (float)height,
  };
}

using namespace afterhours;

#include "bitset_utils.h"
struct ManagesAvailableColors : BaseComponent {
  constexpr static std::array<raylib::Color, input::MAX_GAMEPAD_ID> colors = {{
      raylib::BLUE,
      raylib::ORANGE,
      raylib::PURPLE,
      raylib::SKYBLUE,
      raylib::DARKGREEN,
      raylib::BEIGE,
      raylib::MAROON,
      raylib::GOLD,
  }};

  std::bitset<input::MAX_GAMEPAD_ID> used;
  std::map<size_t, size_t> users;

  raylib::Color get_next_available(size_t id) {
    if (users.contains(id)) {
      return colors[users[id]];
    }

    int index = bitset_utils::get_first_disabled_bit(used);
    if (index == -1) {
      log_warn("no available colors");
      index = 0;
    }
    used[index] = true;
    users[id] = index;
    return colors[index];
  }
};

struct AIControlled : BaseComponent {
  vec2 target{0.f, 0.f};
};

struct HasEntityIDBasedColor : HasColor {
  EntityID id{-1};
  raylib::Color default_{raylib::RAYWHITE};
  HasEntityIDBasedColor(EntityID id_in, raylib::Color col, raylib::Color backup)
      : HasColor(col), id(id_in), default_(backup) {}
};

struct TracksEntity : BaseComponent {
  EntityID id{-1};
  vec2 offset = vec2{0, 0};
  TracksEntity(EntityID id_, vec2 off) : id(id_), offset(off) {}
};

struct Transform : BaseComponent {
  vec2 position{0.f, 0.f};
  vec2 velocity{0.f, 0.f};
  vec2 size{0.f, 0.f};
  float mass{1.f};
  float accel{0.f};
  float accel_mult{1.f};

  float angle{0.f};
  float angle_prev{0.f};
  float speed_dot_angle{0.f};
  bool render_out_of_bounds{true};
  bool cleanup_out_of_bounds{false};

  vec2 pos() const { return position; }
  void update(const vec2 &v) { position = v; }
  Transform(vec2 pos, vec2 sz) : position(pos), size(sz) {}
  raylib::Rectangle rect() const {
    return raylib::Rectangle{position.x, position.y, size.x, size.y};
  }

  auto &set_angle(float ang) {
    angle = ang;
    return *this;
  }

  float speed() const { return vec_mag(velocity); }

  bool is_reversing() const { return speed_dot_angle < 0.f && speed() > 0.f; };

  vec2 center() const {
    return {
        position.x + (size.x / 2.f),
        position.y + (size.y / 2.f),
    };
  }

  raylib::Rectangle focus_rect(int rw = 2) const {
    return raylib::Rectangle{position.x - (float)rw, position.y - (float)rw,
                             size.x + (2.f * (float)rw),
                             size.y + (2.f * (float)rw)};
  }

  float as_rad() const { return static_cast<float>(angle * (M_PI / 180.0f)); }
};

struct TireMarkComponent : BaseComponent {
  struct MarkPoint {
    vec2 position;
    float time;
    float lifetime;
    bool gap;
  };

  bool added_last_frame = false;
  std::vector<MarkPoint> points;

  void add_mark(vec2 pos, bool gap = false) {
    points.push_back(
        MarkPoint{.position = pos, .time = 10.f, .lifetime = 10.f, .gap = gap});
  }

  void pass_time(float dt) {
    for (auto it = points.begin(); it != points.end();) {
      it->time -= dt;
      if (it->time <= 0.0f) {
        it = points.erase(it);
      } else {
        ++it;
      }
    }
  }
};

struct Weapon {
  using FireFn = std::function<void(Entity &, Weapon &)>;

  enum struct Type {
    Cannon,
    Sniper,
    Shotgun,
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
      : Weapon(
            Weapon::Type::MachineGun, //
            Weapon::Config{.cooldownReset = 0.125f,
                           .on_shoot =
                               [](Entity &parent, Weapon &wp) {
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

struct CanDamage : BaseComponent {
  EntityID id;
  int amount;

  CanDamage(EntityID id_in, int amount_in) : id{id_in}, amount{amount_in} {}
};

struct HasLifetime : BaseComponent {
  float lifetime;
  HasLifetime(float life) : lifetime(life) {}
};

struct HasHealth : BaseComponent {
  int max_amount{0};
  int amount{0};

  float iframes = 0.5f;
  float iframesReset = 0.5f;

  void pass_time(float dt) {
    if (iframes > 0)
      iframes -= dt;
  }

  HasHealth(int max_amount_in)
      : max_amount{max_amount_in}, amount{max_amount_in} {}

  HasHealth(int max_amount_in, int amount_in)
      : max_amount{max_amount_in}, amount{amount_in} {}
};

struct PlayerID : BaseComponent {
  input::GamepadID id;
  PlayerID(input::GamepadID i) : id(i) {}
};

struct HasMultipleLives : BaseComponent {
  int num_lives_remaining;
  HasMultipleLives(int num_lives) : num_lives_remaining(num_lives) {}
};

/// @brief Used to make a component whose transform
/// can move during the game loop to wrap around the screen.
/// e.g. if you go passed the width of the screen (right-side),
/// you'll wrap around the the left-side.
/// This applies vertically as well.
///
/// @param padding - this will wait X pixels before wrapping
struct CanWrapAround : BaseComponent {
  float padding;
  CanWrapAround(float padd = 50.f) : padding(padd) {}
};

struct LabelInfo {
  enum class LabelType { StaticText, VelocityText, AccelerationText };

  LabelInfo() = default;

  LabelInfo(const std::string &label_text_in, const vec2 &label_pos_offset_in,
            LabelType label_type_in)
      : label_text{label_text_in}, label_pos_offset{label_pos_offset_in},
        label_type{label_type_in} {}

  std::string label_text;
  vec2 label_pos_offset;
  LabelType label_type;
};

struct HasLabels : public BaseComponent {
  std::vector<LabelInfo> label_info{};

  HasLabels() = default;

  HasLabels(std::vector<LabelInfo> labels) : label_info{std::move(labels)} {}
};
