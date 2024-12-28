
#include "std_include.h"

#include "rl.h"

#define ENABLE_SOUNDS

static int next_id = 0;
bool running = true;
const vec2 button_size = vec2{100, 50};

// 12 gives us these options:
// 1,2,3,4,6,12
int MAX_HEALTH = 120;

int kill_shots_to_base_dmg(int num_shots) {
  if (!(num_shots == 1 || num_shots == 2 || num_shots == 3 || num_shots == 4 ||
        num_shots == 6 || num_shots == 12)) {
    log_error("You are setting a non divisible number of shots: {}", num_shots);
  }
  return MAX_HEALTH / num_shots;
}

struct ConfigurableValues {

  template <typename T> struct ValueInRange {
    T data;
    T min;
    T max;

    ValueInRange(T default_, T min_, T max_)
        : data(default_), min(min_), max(max_) {}

    void operator=(ValueInRange<T> &new_value) { set(new_value.data); }

    void set(T &nv) { data = std::min(max, std::max(min, nv)); }

    void set_pct(const float &pct) {
      // lerp
      T nv = min + pct * (max - min);
      set(nv);
    }

    float get_pct() const { return (data - min) / (max - min); }

    operator T() { return data; }

    operator T() const { return data; }
    operator T &() { return data; }
  };

  ValueInRange<float> max_speed{10.f, 1.f, 20.f};
  ValueInRange<float> skid_threshold{98.5f, 0.f, 100.f};
  ValueInRange<float> steering_sensitivity{2.f, 1.f, 10.f};
};

static ConfigurableValues config;

//
using namespace afterhours;

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

constexpr float distance_sq(const vec2 a, const vec2 b) {
  return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}

float vec_dot(const vec2 &a, const vec2 &b) { return a.x * b.x + a.y * b.y; }
float vec_cross(const vec2 &a, const vec2 &b) { return a.x * b.y - a.y * b.x; }

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

constexpr static float to_radians(float angle) {
  return angle * (float)(M_PI / 180.f);
}

constexpr static float to_degrees(float radians) {
  return radians * 180.f / (float)M_PI;
}

static bool is_point_inside(const vec2 point, const RectangleType &rect) {
  return point.x >= rect.x && point.x <= rect.x + rect.width &&
         point.y >= rect.y && point.y <= rect.y + rect.height;
}
vec2 rect_center(const Rectangle &rect) {
  return vec2{
      rect.x + (rect.width / 2.f),
      rect.y + (rect.height / 2.f),
  };
}

vec2 calc(const Rectangle &rect, const vec2 &point) {
  vec2 center = rect_center(rect);
  float s = (point.y - center.y) / (point.x - center.x);
  float h = rect.height;
  float w = rect.width;
  float h2 = h / 2.f;
  float w2 = w / 2.f;
  float h2s = h2 / s;

  if ((-h2 <= s * w2) && (s * w2 <= h2)) {
    float dir = (point.x > center.x) ? 1.f : -1.f;
    return {center.x + (dir * w2), center.y + dir * (s * w2)};
  }
  if ((-w2 <= h2s) && (h2s <= w2)) {
    float dir = (point.y > center.y) ? 1.f : -1.f;
    return {center.x + dir * h2s, center.y + (dir * h2)};
  }
  return {0, 0};
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

const char *GetAssetsDirectory() {
  static char assetDir[1024] = {0};

  // 1. Check environment variable
  const char *assetsEnv = getenv("RESOURCES");
  if (assetsEnv != NULL) {
    strcpy(assetDir, assetsEnv);

    // Ensure trailing slash.
    if (assetDir[strlen(assetDir) - 1] != '/') {
      strcat(assetDir, "/");
    }
    return assetDir;
  }

  // 2. Check working directory
  char wd[1024] = {0};
  strcpy(wd, raylib::GetWorkingDirectory());
  char *d = strcat(wd, "/resources/");
  if (raylib::DirectoryExists(d)) {
    strcpy(assetDir, d);
    return assetDir;
  }

  // 3. Seek out git root
  char searchDir[1024] = {0};
  strcpy(searchDir, raylib::GetWorkingDirectory());
  for (int i = 0; i < 10; i++) {
    char gitDir[1024] = {0};
    strcpy(gitDir, searchDir);
    strcat(gitDir, "/.git");
    if (raylib::DirectoryExists(gitDir)) {
      strcpy(assetDir, searchDir);
      return strcat(assetDir, "/resources/");
    }
    strcpy(searchDir, raylib::GetPrevDirectoryPath(searchDir));
  }

  return "/Users/gabeochoa/p/kart-afterhours/resources/";
}

// Use this path to load the asset immediately, once called again original value
// is replaced!
const char *GetAssetPath(const char *filename) {
  static char path[1024] = {0};
  strcpy(path, GetAssetsDirectory());
  strcat(path, filename);
  std::cout << "Loading asset: " << path << std::endl;
  return path;
}

enum class InputAction {
  None,
  Accel,
  Left,
  Right,
  Brake,
  ShootLeft,
  ShootRight,
  //
  WidgetNext,
  WidgetPress,
  WidgetMod,
  ValueUp,
  ValueDown,
  ToggleUIDebug,
  ToggleUILayoutDebug,
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

  mapping[InputAction::WidgetNext] = {
      raylib::KEY_TAB,
  };

  mapping[InputAction::WidgetPress] = {
      raylib::KEY_ENTER,
  };

  mapping[InputAction::WidgetMod] = {
      raylib::KEY_LEFT_SHIFT,
  };

  mapping[InputAction::ToggleUIDebug] = {
      raylib::KEY_GRAVE,
  };

  mapping[InputAction::ToggleUILayoutDebug] = {
      raylib::KEY_EQUAL,
  };

  return mapping;
}

#ifdef ENABLE_SOUNDS
enum struct SoundFile {
  Rumble,
  Skrt_Start,
  Skrt_Mid,
  Skrt_End,
};

std::map<SoundFile, raylib::Sound> sounds;

static void load_sounds() {
  magic_enum::enum_for_each<SoundFile>([](auto val) {
    constexpr SoundFile file = val;
    std::string filename;
    switch (file) {
    case SoundFile::Rumble:
      filename = "rumble.wav";
      break;
    case SoundFile::Skrt_Start:
      filename = "skrt_start.wav";
      break;
    case SoundFile::Skrt_Mid:
      filename = "skrt_mid.wav";
      break;
    case SoundFile::Skrt_End:
      filename = "skrt_end.wav";
      break;
    }
    raylib::Sound sound = raylib::LoadSound(GetAssetPath(filename.c_str()));
    sounds[file] = sound;
  });
}

static void play_sound(SoundFile sf) {
  raylib::SetSoundVolume(sounds[sf], 0.25f);
  raylib::PlaySound(sounds[sf]);
}
#endif

struct AIControlled : BaseComponent {
  vec2 target{0.f, 0.f};
};

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

struct TracksEntity : BaseComponent {
  EntityID id{-1};
  vec2 offset = vec2{0, 0};
  TracksEntity(EntityID id_, vec2 off) : id(id_), offset(off) {}
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
  float frame_dur = 1.f / 20.f;
  float frame_time = 1.f / 20.f;
  bool once = false;
  float scale = 1.f;

  int cur_frame = 0;
  float rotation = 0;

  HasAnimation(vec2 start_position_, int total_frames_, float frame_dur_,
               bool once_, float scale_, int cur_frame_, float rot)
      : start_position(start_position_), cur_frame_position(start_position_),
        total_frames(total_frames_), frame_dur(frame_dur_),
        frame_time(frame_dur_), once(once_), scale(scale_),
        cur_frame(cur_frame_), rotation(rot) {}
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
  float mass = 1.f;
  float accel = 0.f;

  float angle{0.f};
  float angle_prev{0.f};
  float speed_dot_angle{0.f};

  vec2 pos() const { return position; }
  void update(vec2 &v) { position = v; }
  Transform(vec2 pos, vec2 sz) : position(pos), size(sz) {}
  raylib::Rectangle rect() const {
    return raylib::Rectangle{position.x, position.y, size.x, size.y};
  }

  auto &set_angle(float ang) {
    angle = ang;
    return *this;
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
  using FireFn = std::function<void(Entity &, Weapon &)>;

  enum struct Type {
    Cannon,
    Sniper,
    Shotgun,
  } type;

  struct Config {
    float cooldownReset;
    FireFn on_shoot;

    float knockback_amt = 0.25f;
    int base_damage = 1;
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
};

void make_poof_anim(Entity &, Weapon &, float angle_offset = 0);
void make_bullet(Entity &, Weapon &, float angle_offset = 0);

struct Cannon : Weapon {

  Cannon(const Weapon::FiringDirection &fd)
      : Weapon(Weapon::Type::Cannon, //
               Weapon::Config{
                   .cooldownReset = 1.f,
                   .on_shoot =
                       [](Entity &parent, Weapon &wp) {
                         make_poof_anim(parent, wp);
                         make_bullet(parent, wp);

                         vec2 dir = parent.get<Transform>().velocity;
                         // opposite of shot direction
                         dir = vec_norm(vec2{-dir.y, dir.x});
                         parent.get<Transform>().velocity +=
                             dir * wp.config.knockback_amt;
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

                         vec2 dir = parent.get<Transform>().velocity;
                         // opposite of shot direction
                         dir = vec_norm(vec2{-dir.y, dir.x});
                         parent.get<Transform>().velocity +=
                             dir * wp.config.knockback_amt;
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

                         vec2 dir = parent.get<Transform>().velocity;
                         // opposite of shot direction
                         dir = vec_norm(vec2{-dir.y, dir.x});
                         parent.get<Transform>().velocity +=
                             dir * wp.config.knockback_amt;
                       },
                   .knockback_amt = 0.50f,
                   // This is per bullet
                   .base_damage = kill_shots_to_base_dmg(4),
               },
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

raylib::Color get_color_for_player(size_t id) {
  constexpr std::array<raylib::Color, input::MAX_GAMEPAD_ID> colors = {{
      raylib::BLUE,
      raylib::ORANGE,
      raylib::PURPLE,
      raylib::SKYBLUE,
      raylib::DARKGREEN,
      raylib::BEIGE,
      raylib::MAROON,
      raylib::GOLD,
  }};
  size_t index = id % colors.size();
  return colors[index];
}

vec2 get_spawn_position(size_t id, int width, int height) {
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

void make_explosion_anim(Entity &parent) {
  Transform &transform = parent.get<Transform>();

  auto &poof = EntityHelper::createEntity();
  poof.addComponent<Transform>(transform.pos(), vec2{10.f, 10.f});

  poof.addComponent<HasAnimation>(vec2{0, 3},
                                  9,          // total_frames
                                  1.f / 20.f, // frame_dur
                                  true,       // once
                                  2.f,        // scale
                                  0, 0);
}

void make_poof_anim(Entity &parent, Weapon &wp, float angle_offset) {
  Transform &transform = parent.get<Transform>();

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

void make_bullet(Entity &parent, Weapon &wp, float angle_offset) {
  Transform &transform = parent.get<Transform>();

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
      .register_weapon(InputAction::ShootLeft, Weapon::FiringDirection::Left,
                       Weapon::Type::Cannon)
      .register_weapon(InputAction::ShootRight,
                       Weapon::FiringDirection::Forward, Weapon::Type::Cannon);

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

static void load_gamepad_mappings() {
  std::ifstream ifs(GetAssetPath("gamecontrollerdb.txt"));
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
          nh * (weapon->cooldown / weapon->config.cooldownReset),
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
    resolution =
        EQ().whereHasComponent<
                afterhours::window_manager::ProvidesCurrentResolution>()
            .gen_first_enforce()
            .get<afterhours::window_manager::ProvidesCurrentResolution>()
            .current_resolution;

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

#ifdef ENABLE_SOUNDS
// For lack of a better filter
struct CarRumble : System<Transform, CanShoot> {
  virtual void for_each_with(const Entity &, const Transform &,
                             const CanShoot &, float) const override {
    play_sound(SoundFile::Rumble);
  }
};
#endif

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

// TODO at the moment we are half imm mode and have retained
// at some point we should transition to fully imm
struct UISystem : System<ui::AutoLayoutRoot> {
  bool init = false;
  Entity *screen_ptr;

  virtual void once(float) override {
    if (!init || screen_ptr == nullptr)
      init_screen();
  }

  void init_screen() {
    auto &entity = EntityQuery()
                       .whereHasComponent<ui::AutoLayoutRoot>()
                       .gen_first_enforce();
    setup_ui(entity);
    init = true;
  }

  virtual ~UISystem() {}
  virtual void setup_ui(Entity &) = 0;
};

struct RenderMainMenuUI : UISystem {
  RenderMainMenuUI() : UISystem() {}
  virtual ~RenderMainMenuUI() {}

  virtual void setup_ui(Entity &root) override {
    using afterhours::ui::children;
    using afterhours::ui::ComponentSize;
    using afterhours::ui::make_button;
    using afterhours::ui::make_div;
    using afterhours::ui::padding_;
    using afterhours::ui::pixels;

    auto &screen = EntityHelper::createEntity();
    screen_ptr = &screen;
    {
      // making a root component to attach the UI to
      screen.addComponent<ui::AutoLayoutRoot>();
      screen.addComponent<ui::UIComponentDebug>("main_screen");
      screen.addComponent<ui::UIComponent>(screen.id)
          .set_desired_width(ui::Size{
              // TODO figure out how to update this
              // when resolution changes
              .dim = ui::Dim::Pixels,
              .value = raylib::GetRenderWidth(),
          })
          .set_desired_height(ui::Size{
              .dim = ui::Dim::Pixels,
              .value = raylib::GetRenderHeight(),
          })
          .set_parent(root)
          .make_absolute();
    }

    if (0) {
      auto &dropdown =
          ui::make_dropdown<window_manager::ProvidesAvailableWindowResolutions>(
              screen);
      dropdown.get<ui::UIComponent>()
          .set_desired_width(ui::Size{
              .dim = ui::Dim::Pixels,
              .value = button_size.x,
          })
          .set_desired_height(ui::Size{
              .dim = ui::Dim::Children,
              .value = button_size.y,
          });
      dropdown.get<ui::HasChildrenComponent>().register_on_child_add(
          [](Entity &child) {
            if (child.is_missing<ui::HasColor>()) {
              child.addComponent<ui::HasColor>(raylib::PURPLE);
            }
          });
    }

    // TODO figure out how to update this
    // when resolution changes
    auto &div = make_div(screen, {padding_(1.f, 1.f), padding_(1.f, 1.f)});

    auto &buttons = make_div(div, afterhours::ui::children_xy());
    buttons.addComponent<ui::UIComponentDebug>("button_group");

    {
      const auto close_menu = [&div](Entity &) {
        div.get<ui::UIComponent>().should_hide = true;
      };
      make_button(buttons, "play", button_size, close_menu);
      make_button(buttons, "about", button_size, close_menu);
      make_button(buttons, "settings", button_size, close_menu);
      make_button(buttons, "exit", button_size,
                  [&](Entity &) { running = false; });
    }
  }
};

struct RenderDebugUI : UISystem {
  bool enabled = true;
  float enableCooldown = 0.f;
  float enableCooldownReset = 0.2f;

  RenderDebugUI() : UISystem() {}
  virtual ~RenderDebugUI() {}

  virtual bool should_run(float dt) override {
    enableCooldown -= dt;

    if (enableCooldown < 0) {
      enableCooldown = enableCooldownReset;
      input::PossibleInputCollector<InputAction> inpc =
          input::get_input_collector<InputAction>();
      for (auto &actions_done : inpc.inputs()) {
        if (actions_done.action == InputAction::ToggleUIDebug) {
          enabled = !enabled;
          screen_ptr->get<ui::UIComponent>().should_hide = enabled;
          break;
        }
      }
    }
    return enabled;
  }

  virtual void setup_ui(Entity &root) override {
    using afterhours::ui::children_xy;
    using afterhours::ui::FlexDirection;
    using afterhours::ui::make_div;
    using afterhours::ui::make_slider;
    using afterhours::ui::pixels;

    // TODO just replace all of these with autolayout roots....
    auto &screen = EntityHelper::createEntity();
    screen_ptr = &screen;
    {
      // making a root component to attach the UI to
      screen.addComponent<ui::AutoLayoutRoot>();
      screen.addComponent<ui::UIComponentDebug>("debug_screen");
      screen.addComponent<ui::UIComponent>(screen.id)
          .set_desired_width(ui::Size{
              // TODO figure out how to update this
              // when resolution changes
              .dim = ui::Dim::Pixels,
              .value = raylib::GetRenderWidth(),
          })
          .set_desired_height(ui::Size{
              .dim = ui::Dim::Pixels,
              .value = raylib::GetRenderHeight(),
          })
          .set_parent(root)
          .make_absolute();
    }

    auto &div = make_div(screen, children_xy());
    div.get<ui::UIComponent>().flex_direction = FlexDirection::Row;
    {
      auto &max_speed = ui::make_div(div, {
                                              pixels(button_size.x),
                                              pixels(button_size.y / 2.f),
                                          });
      max_speed.addComponent<ui::HasColor>(raylib::GRAY);
      max_speed.addComponent<ui::HasLabel>(
          "Max Speed\n" + std::to_string(config.max_speed.data) + " m/s");

      make_slider(div,
                  vec2{
                      button_size.x,
                      button_size.y / 2.f,
                  },
                  config.max_speed.get_pct(), [&](const float pct) {
                    config.max_speed.set_pct(pct);
                    max_speed.get<ui::HasLabel>().label =
                        "Max Speed\n" + std::to_string(config.max_speed.data) +
                        " m/s";
                  });

      auto &skid_threshold = ui::make_div(div, {
                                                   pixels(button_size.x),
                                                   pixels(button_size.y / 2.f),
                                               });
      skid_threshold.addComponent<ui::HasColor>(raylib::GRAY);
      skid_threshold.addComponent<ui::HasLabel>(
          "Skid Threshold\n" + std::to_string(config.skid_threshold.data) +
          "%");

      make_slider(div,
                  vec2{
                      button_size.x,
                      button_size.y / 2.f,
                  },
                  config.skid_threshold.get_pct(), [&](const float pct) {
                    config.skid_threshold.set_pct(pct);
                    skid_threshold.get<ui::HasLabel>().label =
                        "Skid Threshold\n" +
                        std::to_string(config.skid_threshold.data) + "%";
                  });

      auto &steering_sensitivity =
          ui::make_div(div, {
                                pixels(button_size.x),
                                pixels(button_size.y / 2.f),
                            });
      steering_sensitivity.addComponent<ui::HasColor>(raylib::GRAY);
      steering_sensitivity.addComponent<ui::HasLabel>(
          "Steering Sensitivity\n" +
          std::to_string(config.steering_sensitivity.data));

      make_slider(div,
                  vec2{
                      button_size.x,
                      button_size.y / 2.f,
                  },
                  config.steering_sensitivity.get_pct(), [&](const float pct) {
                    config.steering_sensitivity.set_pct(pct);
                    steering_sensitivity.get<ui::HasLabel>().label =
                        "Steering Sensitivity\n" +
                        std::to_string(config.steering_sensitivity.data);
                  });
    }
  }
};

int main(void) {
  const int screenWidth = 1280;
  const int screenHeight = 720;

  raylib::InitWindow(screenWidth, screenHeight, "kart-afterhours");
  raylib::SetTargetFPS(200);

  raylib::SetWindowState(raylib::FLAG_WINDOW_RESIZABLE);

  raylib::InitAudioDevice();
  raylib::SetMasterVolume(1.f);

  load_gamepad_mappings();

  load_sounds();

  // sophie
  auto &sophie = EntityHelper::createEntity();
  {
    input::add_singleton_components<InputAction>(sophie, get_mapping());
    window_manager::add_singleton_components(sophie, 200);
    ui::add_singleton_components<InputAction>(sophie);
    sophie.addComponent<HasTexture>(
        raylib::LoadTexture(GetAssetPath("spritesheet.png")));

    // making a root component to attach the UI to
    sophie.addComponent<ui::AutoLayoutRoot>();
    sophie.addComponent<ui::UIComponentDebug>("sophie");
    sophie.addComponent<ui::UIComponent>(sophie.id)
        .set_desired_width(ui::Size{
            // TODO figure out how to update this
            // when resolution changes
            .dim = ui::Dim::Pixels,
            .value = screenWidth,
        })
        .set_desired_height(ui::Size{
            .dim = ui::Dim::Pixels,
            .value = screenHeight,
        });
  }

  make_player(0);

  // make_ai();

  SystemManager systems;

  // debug systems
  {
    window_manager::enforce_singletons(systems);
    ui::enforce_singletons<InputAction>(systems);
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
    systems.register_update_system(std::make_unique<UpdateCollidingEntities>());
    systems.register_update_system(std::make_unique<WrapAroundTransform>());
    systems.register_update_system(
        std::make_unique<AnimationUpdateCurrentFrame>());
    systems.register_update_system(
        std::make_unique<UpdateColorBasedOnEntityID>());
    systems.register_update_system(std::make_unique<AIVelocity>());
    systems.register_update_system(std::make_unique<DrainLife>());
    systems.register_update_system(std::make_unique<UpdateTrackingEntities>());

    ui::register_update_systems<InputAction>(systems);
  }

  // renders
  {
    systems.register_render_system(
        [&](float) { raylib::ClearBackground(raylib::DARKGRAY); });
    systems.register_render_system(std::make_unique<RenderDebugUI>());
    // TODO disabling main menu for right now
    // systems.register_render_system(std::make_unique<RenderMainMenuUI>());
    ui::register_render_systems<InputAction>(systems,
                                             InputAction::ToggleUILayoutDebug);
    systems.register_render_system(std::make_unique<RenderSkid>());
    systems.register_render_system(std::make_unique<RenderEntities>());
    systems.register_render_system(std::make_unique<RenderHealthAndLives>());
    systems.register_render_system(std::make_unique<RenderSprites>());
    systems.register_render_system(std::make_unique<RenderAnimation>());
    systems.register_render_system(std::make_unique<RenderWeaponCooldown>());
    systems.register_render_system(std::make_unique<RenderOOB>());
    systems.register_render_system(std::make_unique<CarRumble>());
    //
    systems.register_render_system(std::make_unique<RenderFPS>());
  }

  while (running && !raylib::WindowShouldClose()) {
    raylib::BeginDrawing();
    { systems.run(raylib::GetFrameTime()); }
    raylib::EndDrawing();
  }

  raylib::CloseAudioDevice();
  raylib::CloseWindow();

  std::cout << "Num entities: " << EntityHelper::get_entities().size()
            << std::endl;

  return 0;
}
