
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

static vec2 get_spawn_position(size_t id) {
  auto *resolution_provider = EntityHelper::get_singleton_cmp<
      window_manager::ProvidesCurrentResolution>();
  return get_spawn_position(id, resolution_provider->width(),
                            resolution_provider->height());
}

#include "bitset_utils.h"
// Forward declare RoundType to avoid heavy header include
enum struct RoundType : size_t;
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
  std::map<size_t, size_t> users; // userid to color index
                                  //
  void release_only(size_t id) {
    if (users.contains(id)) {
      int bit_index = static_cast<int>(users[id]);
      users.erase(id);
      // turn off the one we were previously using
      used[bit_index] = false;
    }
  }

  raylib::Color release_and_get_next(size_t id) {
    int bit_index = -1;
    if (users.contains(id)) {
      bit_index = static_cast<int>(users[id]);
      users.erase(id);
      // we dont turn off until after because otherwise
      // we could just get the same color back
    }

    auto next_color = get_next_available(id, std::max(0, bit_index));

    // turn off the one we were previously using
    if (bit_index != -1) {
      used[bit_index] = false;
    }

    return next_color;
  }

  bool any_available_colors() const {
    return bitset_utils::get_next_disabled_bit(used, 0) != -1;
  }

  raylib::Color get_next_NO_STORE(size_t start = 0) {
    int index = bitset_utils::get_next_disabled_bit(used, start);
    if (index == -1) {
      index = 0;
    }
    return colors[index];
  }

  raylib::Color get_next_available(size_t id, size_t start = 0) {
    if (users.contains(id)) {
      return colors[users[id]];
    }

    int index = bitset_utils::get_next_disabled_bit(used, start);
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

struct AIDifficulty : BaseComponent {
  enum class Difficulty { Easy, Medium, Hard, Expert };

  Difficulty difficulty{Difficulty::Medium};

  AIDifficulty(Difficulty diff = Difficulty::Medium) : difficulty(diff) {}
};

struct AIMode : BaseComponent {
  // If true, the mode will be kept in sync with RoundManager::active_round_type
  bool follow_round_type{true};
  RoundType mode{static_cast<RoundType>(0)};

  AIMode(RoundType m = static_cast<RoundType>(0), bool follow = true)
      : follow_round_type(follow), mode(m) {}
};

struct AIParams : BaseComponent {
  // How close to the current target before choosing a new one (world units)
  float retarget_radius{10.0f};

  // Tag & Go: how far ahead runners try to move when evading
  float runner_evade_lookahead_distance{100.0f};

  // Hippo mode: base jitter radius by difficulty
  float hippo_jitter_easy{200.0f};
  float hippo_jitter_medium{100.0f};
  float hippo_jitter_hard{50.0f};
  float hippo_jitter_expert{0.0f};

  // Hippo mode: divisor for distance-based jitter attenuation
  float hippo_jitter_distance_scale{300.0f};

  // Hippo mode: evaluated jitter to use (set by systems, not code branching)
  float hippo_target_jitter{100.0f};

  // Kills mode shooting: maximum allowed misalignment to fire (degrees)
  float shooting_alignment_angle_deg{10.0f};

  // Boost behavior parameters
  // Only consider boosting when the target is at least this far away (squared
  // distance)
  float boost_min_distance_sq{400.0f};
  // Only consider boosting when the target is within this ahead cone (degrees)
  float boost_ahead_alignment_deg{1.0f};
  // Cooldown override for AI boost requests (seconds); <= 0 to keep current
  // component/default
  float boost_cooldown_seconds{3.0f};
};

struct AIBoostCooldown : BaseComponent {
  float next_allowed_time = 0.0f;
  float cooldown_seconds = 3.0f;
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

struct CollisionConfig {
  float mass{1.f}; // Affects impulse strength & knock back effect
  float friction{
      0.f}; // Affects post collision slide velocity and subsequently distance
  float restitution{.5f}; // Affects "bouncy-ness" [0, 1] where 1 is bouncy
};

struct Transform : BaseComponent {
  vec2 position{0.f, 0.f};
  vec2 velocity{0.f, 0.f};
  vec2 size{0.f, 0.f};
  CollisionConfig collision_config{};
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
  Transform(Rectangle rect)
      : position({rect.x, rect.y}), size({rect.width, rect.height}) {}
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
    float hue;
  };

  bool added_last_frame = false;
  std::vector<MarkPoint> points;
  float rolling_hue = 0.0f;

  void add_mark(vec2 pos, bool gap = false, float hue = 0.f) {
    points.push_back(MarkPoint{.position = pos,
                               .time = 10.f,
                               .lifetime = 10.f,
                               .gap = gap,
                               .hue = hue});
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

  std::optional<EntityID> last_damaged_by{};

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

struct HasKillCountTracker : BaseComponent {
  int kills = 0;
  HasKillCountTracker() = default;
  HasKillCountTracker(int initial_kills) : kills(initial_kills) {}
};

struct HasTagAndGoTracking : BaseComponent {
  float time_as_not_it = 0.0f;
  bool is_tagger = false;
  float last_tag_time = -1.f;
  HasTagAndGoTracking() = default;
};

struct HasHippoCollection : BaseComponent {
  int hippos_collected = 0;

  void collect_hippo() { hippos_collected++; }

  int get_hippo_count() const { return hippos_collected; }
};

struct HippoItem : BaseComponent {
  bool collected = false;
  float spawn_time = 0.0f;

  HippoItem() = default;
  HippoItem(float spawn_t) : spawn_time(spawn_t) {}
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

struct CollisionAbsorber : public BaseComponent {
  CollisionAbsorber() = default;

  enum class AbsorberType {
    Absorber,
    Absorbed, // Cleans up upon collision with a differently parented Absorber
  };

  CollisionAbsorber(AbsorberType absorber_type_in,
                    std::optional<size_t> parent_id_in = std::nullopt)
      : BaseComponent{}, absorber_type{absorber_type_in},
        parent_id{parent_id_in} {}

  // Affects cleanup if objects touching are of opposite types
  AbsorberType absorber_type{AbsorberType::Absorber};

  // Optionally ignore collision if containing the same parent
  std::optional<EntityID> parent_id{std::nullopt};
};

struct MapGenerated : BaseComponent {
  // This component marks entities that were generated by the map system
  // and should be cleaned up when a new map is created
};

struct IsFloorOverlay : BaseComponent {};

struct CarAffector : BaseComponent {};
struct SteeringAffector : CarAffector {
  float multiplier{1.f};
  SteeringAffector(float mult) : multiplier(mult) {}
};
struct AccelerationAffector : CarAffector {
  float multiplier{1.f};
  AccelerationAffector(float mult) : multiplier(mult) {}
};

struct SteeringIncrementor : CarAffector {
  float target_sensitivity{0.f};
  SteeringIncrementor(float sensitivity) : target_sensitivity(sensitivity) {}
};

struct SpeedAffector : CarAffector {
  float multiplier{1.f};
  SpeedAffector(float mult) : multiplier(mult) {}
};

struct HasShader : BaseComponent {
  std::string shader_name;

  HasShader(const std::string &name) : shader_name(name) {}
  HasShader(const char *name) : shader_name(name) {}
};

struct SkipTextureManagerRendering : BaseComponent {
  // Marker component to prevent texture_manager from rendering entities with
  // shaders
};

struct WasWinnerLastRound : BaseComponent {
  // Marker component to indicate this entity was the winner of the previous
  // round
};

struct WantsBoost : BaseComponent {};

struct HonkState : BaseComponent {
  bool was_down = false;
};

// New: Weapon firing config components
struct RecoilConfig : BaseComponent {
  float knockback_amt{0.f};
  RecoilConfig() = default;
  explicit RecoilConfig(float amt) : knockback_amt(amt) {}
};

struct ProjectileConfig : BaseComponent {
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
};

struct WantsWeaponFire : BaseComponent {
  InputAction action;
  WantsWeaponFire() = default;
  explicit WantsWeaponFire(InputAction a) : action(a) {}
};

struct WeaponFired : BaseComponent {
  // Store minimal info to avoid circular deps with weapons.h
  int weapon_type{0};
  int firing_direction{0};
  ProjectileConfig projectile;
  RecoilConfig recoil;
  InputAction action{};

  WeaponFired() = default;
  WeaponFired(InputAction act, int weapon_type_in, int firing_direction_in,
              const ProjectileConfig &proj, const RecoilConfig &rec)
      : weapon_type(weapon_type_in), firing_direction(firing_direction_in),
        projectile(proj), recoil(rec), action(act) {}
};
