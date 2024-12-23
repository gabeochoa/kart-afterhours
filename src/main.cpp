
#include "std_include.h"

#include "rl.h"

#define AFTER_HOURS_INCLUDE_DERIVED_CHILDREN
#define AFTER_HOURS_ENTITY_HELPER
#define AFTER_HOURS_ENTITY_QUERY
#define AFTER_HOURS_SYSTEM
#include "afterhours/ah.h"
#define AFTER_HOURS_USE_RAYLIB
#include "afterhours/src/developer.h"
#include "afterhours/src/plugins/input_system.h"
#include "afterhours/src/plugins/window_manager.h"
#include <cassert>

//
using namespace afterhours;

typedef raylib::Vector2 vec2;
typedef raylib::Vector3 vec3;
typedef raylib::Vector4 vec4;
using raylib::Rectangle;

constexpr float distance_sq(const vec2 a, const vec2 b) {
  return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}

float dot(const vec2 &a, const vec2 &b) { return a.x * b.x + a.y * b.y; }

float vec_mag(vec2 v) { return sqrt(v.x * v.x + v.y * v.y); }
vec2 vec_norm(vec2 v) {
  float mag = vec_mag(v);
  if (mag == 0)
    return v;
  return vec2{
      v.x / mag,
      v.y / mag,
  };
}

namespace myutil {

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }
} // namespace myutil

struct RenderFPS : System<window_manager::ProvidesCurrentResolution> {
  virtual ~RenderFPS() {}
  virtual void for_each_with(
      const Entity &,
      const window_manager::ProvidesCurrentResolution &pCurrentResolution,
      float) const override {
    raylib::DrawFPS((int)(pCurrentResolution.width() - 80), 0);
  }
};

enum class InputAction {
  None,
  Accel,
  Left,
  Right,
  Brake,
  ShootLeft,
  ShootRight
};

using afterhours::input;

auto get_mapping() {
  std::map<InputAction, input::ValidInputs> mapping;

  mapping[InputAction::Accel] = {
      raylib::KEY_UP,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_RIGHT_Y,
          .dir = -1,
      },
  };

  mapping[InputAction::Brake] = {
      raylib::KEY_DOWN,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_RIGHT_Y,
          .dir = 1,
      },
  };

  mapping[InputAction::Left] = {
      raylib::KEY_LEFT,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_LEFT_X,
          .dir = -1,
      },
  };

  mapping[InputAction::Right] = {
      raylib::KEY_RIGHT,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_LEFT_X,
          .dir = 1,
      },
  };

  mapping[InputAction::ShootLeft] = {
      raylib::KEY_Q,
      raylib::GAMEPAD_BUTTON_LEFT_TRIGGER_1,
  };

  mapping[InputAction::ShootRight] = {
      raylib::KEY_E,
      raylib::GAMEPAD_BUTTON_RIGHT_TRIGGER_1,
  };

  return mapping;
}

struct HasColor : BaseComponent {
  raylib::Color color = raylib::WHITE;
  HasColor(raylib::Color col) : color(col) {}
};

struct HasEntityIDBasedColor : HasColor {
  EntityID id{-1};
  raylib::Color default_{raylib::RAYWHITE};
  HasEntityIDBasedColor(EntityID id_in, raylib::Color col, raylib::Color backup)
      : HasColor(col), id(id_in), default_(backup) {}
};

struct HasTexture : BaseComponent {
  raylib::Texture2D texture;
  HasTexture(const raylib::Texture2D tex) : texture(tex) {}
};

struct HasSprite : BaseComponent {
  Rectangle frame;
  float scale;
  raylib::Color colorTint;
  HasSprite(Rectangle frm, float scl, raylib::Color colorTintIn)
      : frame{frm}, scale{scl}, colorTint{colorTintIn} {}
};

struct HasAnimation : BaseComponent {
  vec2 start_position;
  vec2 cur_frame_position;
  int total_frames = 10;
  int cur_frame = 0;
  float rotation = 0;
  float frame_dur = 1.f / 20.f;
  float frame_time = 1.f / 20.f;
  bool once = false;
  float scale = 1.f;

  HasAnimation(vec2 start_position_, int total_frames_, float frame_dur_,
               bool once_, float scale_)
      : start_position(start_position_), cur_frame_position(start_position_),
        total_frames(total_frames_), frame_dur(frame_dur_),
        frame_time(frame_dur_), once(once_), scale(scale_) {}

  HasAnimation(const HasAnimation &other) {
    start_position = other.start_position;
    cur_frame_position = other.cur_frame_position;
    total_frames = other.total_frames;
    cur_frame = other.cur_frame;
    rotation = other.rotation;
    frame_dur = other.frame_dur;
    frame_time = other.frame_time;
    once = other.once;
    scale = other.scale;
  }
};

Rectangle idx_to_sprite_frame(int i, int j) {
  return Rectangle{.x = (float)i * 32.f,
                   .y = (float)j * 32.f,
                   .width = 32.f,
                   .height = 32.f};
}

std::pair<int, int> idx_to_next_sprite_location(int i, int j) {
  i++;
  if (i == 32) {
    i = 0;
    j++;
  }
  return {i, j};
}

struct Transform : BaseComponent {
  vec2 position{0.f, 0.f};
  vec2 velocity{0.f, 0.f};
  vec2 size{0.f, 0.f};
  float accel = 0.f;

  float angle{0.f};
  float angle_prev{0.f};

  vec2 pos() const { return position; }
  void update(vec2 &v) { position = v; }
  Transform(vec2 pos, vec2 sz) : position(pos), size(sz) {}
  raylib::Rectangle rect() const {
    return raylib::Rectangle{position.x, position.y, size.x, size.y};
  }

  float speed() const { return vec_mag(velocity); }

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
  float cooldown;
  float cooldownReset;
  using FireFn = std::function<void(Entity &, Weapon &)>;
  FireFn on_shoot;
  float knockback_amt = 1.f;

  Weapon(const FireFn &cb)
      : cooldown(0.f), cooldownReset(0.75f), on_shoot(cb) {}

  virtual ~Weapon() {}

  virtual bool fire(float) {
    if (cooldown <= 0) {
      cooldown = cooldownReset;
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
};

struct CanShoot : BaseComponent {
  std::map<InputAction, std::unique_ptr<Weapon>> weapons;

  CanShoot() = default;

  auto &register_weapon(InputAction action, const Weapon::FireFn &cb) {
    weapons[action] = std::make_unique<Weapon>(cb);
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
      weapons[action]->on_shoot(parent, *weapons[action]);
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

struct HasHealth : BaseComponent {
  int max_amount{0};
  int amount{0};

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
struct CanWrapAround : BaseComponent {
  CanWrapAround() = default;
};

struct EQ : public EntityQuery<EQ> {
  struct WhereInRange : EntityQuery::Modification {
    vec2 position;
    float range;

    // TODO mess around with the right epsilon here
    explicit WhereInRange(vec2 pos, float r = 0.01f)
        : position(pos), range(r) {}
    bool operator()(const Entity &entity) const override {
      vec2 pos = entity.get<Transform>().pos();
      return distance_sq(position, pos) < (range * range);
    }
  };

  EQ &whereInRange(const vec2 &position, float range) {
    return add_mod(new WhereInRange(position, range));
  }

  EQ &orderByDist(const vec2 &position) {
    return orderByLambda([position](const Entity &a, const Entity &b) {
      float a_dist = distance_sq(a.get<Transform>().pos(), position);
      float b_dist = distance_sq(b.get<Transform>().pos(), position);
      return a_dist < b_dist;
    });
  }

  struct WhereOverlaps : EntityQuery::Modification {
    Rectangle rect;

    explicit WhereOverlaps(Rectangle rect_) : rect(rect_) {}

    bool operator()(const Entity &entity) const override {
      const auto r1 = rect;
      const auto r2 = entity.get<Transform>().rect();
      // Check if the rectangles overlap on the x-axis
      const bool xOverlap = r1.x < r2.x + r2.width && r2.x < r1.x + r1.width;

      // Check if the rectangles overlap on the y-axis
      const bool yOverlap = r1.y < r2.y + r2.height && r2.y < r1.y + r1.height;

      // Return true if both x and y overlap
      return xOverlap && yOverlap;
    }
  };

  EQ &whereOverlaps(const Rectangle r) { return add_mod(new WhereOverlaps(r)); }
};

void make_poof_anim(Entity &parent, float direction) {
  Transform &transform = parent.get<Transform>();

  auto &poof = EntityHelper::createEntity();
  poof.addComponent<Transform>(transform.pos() + vec2{direction * 20.f, 10.f},
                               vec2{10.f, 10.f})
      .angle = transform.angle;
  HasAnimation anim(vec2{0, 0}, // start_position
                    14,         // total_frames
                    1.f / 20.f, // frame_dur
                    true,       // once
                    1.f         // scale
  );
  anim.cur_frame = 0;
  anim.rotation = (direction > 0 ? 90 : -90);
  poof.addComponent<HasAnimation>(anim);
}

void make_bullet(Entity &parent, float direction) {
  Transform &transform = parent.get<Transform>();

  auto &bullet = EntityHelper::createEntity();
  bullet.addComponent<Transform>(transform.pos() + vec2{0, 10.f},
                                 vec2{10.f, 10.f});

  bullet.addComponent<CanDamage>(parent.id, 5);

  bullet.addComponent<CanWrapAround>();

  auto bullet_color =
      parent.get<PlayerID>().id == 0 ? raylib::BLUE : raylib::GREEN;
  bullet.addComponent<HasColor>(bullet_color);
  bullet.addComponent<HasEntityIDBasedColor>(parent.id, bullet_color,
                                             raylib::RED);

  float rad = transform.as_rad() + ((float)(M_PI / 2.f) * direction);
  bullet.get<Transform>().velocity =
      vec2{std::sin(rad) * 5.f, -std::cos(rad) * 5.f};
}

void make_player(input::GamepadID id) {
  auto &entity = EntityHelper::createEntity();

  vec2 position = {.x = id == 0 ? 150.f : 1100.f, .y = 720.f / 2.f};

  entity.addComponent<PlayerID>(id);
  entity.addComponent<HasMultipleLives>(1);
  entity.addComponent<Transform>(position, vec2{15.f, 25.f});
  entity.addComponent<CanWrapAround>();
  entity.addComponent<HasHealth>(15);
  entity.addComponent<TireMarkComponent>();

  auto tint = raylib::RAYWHITE;
  if (id != 0) {
    tint = raylib::ORANGE;
  }

  entity.addComponent<HasSprite>(idx_to_sprite_frame(0, 1), 1.f, tint);

  entity.addComponent<CanShoot>()
      .register_weapon(InputAction::ShootLeft,
                       [](Entity &parent, Weapon &wp) {
                         make_poof_anim(parent, -1);
                         make_bullet(parent, -1);

                         vec2 dir = parent.get<Transform>().velocity;
                         // opposite of shot direction
                         dir = vec_norm(vec2{-dir.y, dir.x});
                         parent.get<Transform>().velocity +=
                             dir * wp.knockback_amt;
                       })
      .register_weapon(InputAction::ShootRight, [](Entity &parent, Weapon &wp) {
        make_poof_anim(parent, 1);
        make_bullet(parent, 1);

        vec2 dir = parent.get<Transform>().velocity;
        // opposite of shot direction
        dir = vec_norm(vec2{dir.y, -dir.x});
        parent.get<Transform>().velocity += dir * wp.knockback_amt;
      });
}

static void load_gamepad_mappings() {
  std::ifstream ifs("gamecontrollerdb.txt");
  if (!ifs.is_open()) {
    std::cout << "Failed to load game controller db" << std::endl;
    return;
  }
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  input::set_gamepad_mappings(buffer.str().c_str());
}

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
          nh * (weapon->cooldown / weapon->cooldownReset),
      };

      raylib::DrawRectanglePro(arm,
                               {nw / 2.f, nh / 2.f}, // rotate around center
                               transform.angle, raylib::RED);
    }
  }
};

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

struct RenderHealth : System<Transform, HasHealth> {

  virtual void for_each_with(const Entity &, const Transform &transform,
                             const HasHealth &hasHealth, float) const override {
    // Calculate the percentage of the height by the percentage of the health
    // max against its current value Scaling factors for bar length and height
    const float scale_x = 2.f;
    const float scale_y = 1.25f;

    float health_as_percent = static_cast<float>(hasHealth.amount) /
                              static_cast<float>(hasHealth.max_amount);

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
        vec2{0.0f, 0.0f}, // No rotation
        0.0f, raylib::RED);

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
        vec2{0.0f, 0.0f}, // No rotation
        0.0f, raylib::GREEN);
  }
};

struct VelFromInput : System<PlayerID, Transform> {

  float max_speed = 5.f;

  virtual void for_each_with(Entity &, PlayerID &playerID, Transform &transform,
                             float dt) override {

    input::PossibleInputCollector<InputAction> inpc =
        input::get_input_collector<InputAction>();
    if (!inpc.has_value()) {
      return;
    }

    transform.accel = 0.f;
    float steer = 0.f;

    for (auto &actions_done : inpc.inputs()) {
      if (actions_done.id != playerID.id)
        continue;
      switch (actions_done.action) {
      case InputAction::Accel:
        transform.accel = 2.0f;
        break;
      case InputAction::Brake:
        transform.accel = -0.5f;
        break;
      case InputAction::Left:
        steer = -1.f * actions_done.amount_pressed;
        break;
      case InputAction::Right:
        steer = actions_done.amount_pressed;
        break;
      default:
        break;
      }
    }

    float minRadius = 10.0f;
    float maxRadius = 300.0f;
    float rad = std::lerp(minRadius, maxRadius, transform.speed() / max_speed);

    float mvt = std::max(
        -max_speed, std::min(max_speed, transform.speed() + transform.accel));

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

struct MatchKartsToPlayers : System<input::ProvidesMaxGamepadID> {

  virtual void for_each_with(Entity &,
                             input::ProvidesMaxGamepadID &maxGamepadID,
                             float) override {

    auto existing_players = EQ().whereHasComponent<PlayerID>().gen();

    // we are good
    if (existing_players.size() == maxGamepadID.count())
      return;

    if (existing_players.size() > maxGamepadID.count()) {
      // remove the player that left
      for (Entity &player : existing_players) {
        if (input::is_gamepad_available(player.get<PlayerID>().id))
          continue;
        player.cleanup = true;
      }
      return;
    }

    // we need to add a new player

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

struct ProcessDamage : System<Transform, HasHealth> {

  virtual void for_each_with(Entity &entity, Transform &transform,
                             HasHealth &hasHealth, float) override {

    auto can_damage = EQ().whereHasComponent<CanDamage>()
                          .whereNotID(entity.id)
                          .whereOverlaps(transform.rect())
                          .gen();

    for (Entity &damager : can_damage) {
      CanDamage &cd = damager.get<CanDamage>();
      if (cd.id == entity.id)
        continue;
      hasHealth.amount -= cd.amount;

      damager.cleanup = true;
    }
  }
};

struct ProcessDeath : System<Transform, HasHealth> {

  virtual void for_each_with(Entity &entity, Transform &, HasHealth &hasHealth,
                             float) override {
    if (hasHealth.amount <= 0) {
      if (entity.has<HasMultipleLives>()) {
        entity.get<HasMultipleLives>().num_lives_remaining -= 1;
      }
      entity.cleanup = true;
    }
  }
};

struct WrapAroundTransform : System<Transform, CanWrapAround> {

  window_manager::Resolution resolution;

  virtual void once(float) {
    resolution =
        EQ().whereHasComponent<
                afterhours::window_manager::ProvidesCurrentResolution>()
            .gen_first_enforce()
            .get<afterhours::window_manager::ProvidesCurrentResolution>()
            .current_resolution;
  }

  virtual void for_each_with(Entity &, Transform &transform, CanWrapAround &,
                             float) override {
    float width = (float)resolution.width;
    float height = (float)resolution.height;
    if (transform.rect().x > width) {
      transform.position.x = 0;
    }

    if (transform.rect().x < 0) {
      transform.position.x = width;
    }

    if (transform.rect().y < 0) {
      transform.position.y = height;
    }

    if (transform.rect().y > height) {
      transform.position.y = 0;
    }
  }
};

struct SkidMarks : System<Transform, TireMarkComponent> {
  virtual void for_each_with(Entity &, Transform &transform,
                             TireMarkComponent &tire, float dt) override {

    tire.pass_time(dt);

    const auto should_skid = [&]() -> bool {
      // TODO - dont add skid marks when you are going in reverse
      float steering_angle = transform.angle - transform.angle_prev;

      if (transform.speed() > 4.f && steering_angle > 30.f) {
        return true;
      }

      float slip_angle =
          std::atan2(transform.velocity.y, transform.velocity.x) -
          std::atan2(transform.velocity.y,
                     transform.velocity.x -
                         (transform.velocity.y * std::tan(steering_angle)));

      if (slip_angle > (10.f * (M_PI / 180.f))) {
        return true;
      }

      return false;
    };

    if (should_skid()) {

      vec2 pos = transform.center();

      tire.add_mark(pos, !tire.added_last_frame);
      tire.added_last_frame = true;
    } else {
      tire.added_last_frame = false;
    }

    transform.angle_prev =
        std::atan2(transform.velocity.y, transform.velocity.x);
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

  virtual void once(float) {
    sheet = EQ().whereHasComponent<HasTexture>()
                .gen_first_enforce()
                .get<HasTexture>()
                .texture;
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

int main(void) {
  const int screenWidth = 1920;
  const int screenHeight = 1080;

  raylib::InitWindow(screenWidth, screenHeight, "kart-afterhours");
  raylib::SetTargetFPS(200);

  load_gamepad_mappings();

  // sophie
  {
    auto &entity = EntityHelper::createEntity();
    input::add_singleton_components<InputAction>(entity, get_mapping());
    window_manager::add_singleton_components(entity, 200);
    entity.addComponent<HasTexture>(
        raylib::LoadTexture("./resources/spritesheet.png"));
  }

  SystemManager systems;

  // debug systems
  {
    window_manager::enforce_singletons(systems);
    input::enforce_singletons<InputAction>(systems);
  }

  // external plugins
  {
    input::register_update_systems<InputAction>(systems);
    window_manager::register_update_systems(systems);
  }

  // Fixed update
  {
    systems.register_fixed_update_system(std::make_unique<VelFromInput>());
    systems.register_fixed_update_system(std::make_unique<Move>());
  }

  // normal update
  {
    systems.register_update_system(std::make_unique<Shoot>());
    systems.register_update_system(std::make_unique<MatchKartsToPlayers>());
    systems.register_update_system(std::make_unique<ProcessDamage>());
    systems.register_update_system(std::make_unique<ProcessDeath>());
    systems.register_update_system(std::make_unique<SkidMarks>());
    systems.register_update_system(std::make_unique<WrapAroundTransform>());
    systems.register_update_system(
        std::make_unique<AnimationUpdateCurrentFrame>());
    systems.register_update_system(
        std::make_unique<UpdateColorBasedOnEntityID>());
  }

  // renders
  {
    systems.register_render_system(
        [&](float) { raylib::ClearBackground(raylib::DARKGRAY); });
    systems.register_render_system(std::make_unique<RenderSkid>());
    systems.register_render_system(std::make_unique<RenderEntities>());
    systems.register_render_system(std::make_unique<RenderHealth>());
    systems.register_render_system(std::make_unique<RenderSprites>());
    systems.register_render_system(std::make_unique<RenderAnimation>());
    systems.register_render_system(std::make_unique<RenderWeaponCooldown>());
    //
    systems.register_render_system(std::make_unique<RenderFPS>());
  }

  while (!raylib::WindowShouldClose()) {
    raylib::BeginDrawing();
    { systems.run(raylib::GetFrameTime()); }
    raylib::EndDrawing();
  }

  raylib::CloseWindow();

  std::cout << "Num entities: " << EntityHelper::get_entities().size()
            << std::endl;

  return 0;
}
