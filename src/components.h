
#pragma once

#include "shader_types.h"
#include <optional>
#include <string>

#include "input_mapping.h"
#include "math_util.h"
#include "max_health.h"
#include "rl.h"
#include "shader_types.h"

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

static vec2 get_spawn_position(size_t id) {
  auto *resolution_provider = afterhours::EntityHelper::get_singleton_cmp<
      afterhours::window_manager::ProvidesCurrentResolution>();
  return get_spawn_position(id, resolution_provider->width(),
                            resolution_provider->height());
}

#include "../vendor/afterhours/src/bitset_utils.h"
#include <afterhours/src/plugins/collision.h>
// Forward declare RoundType to avoid heavy header include
enum struct RoundType : size_t;
struct ManagesAvailableColors : afterhours::BaseComponent {
  constexpr static std::array<::raylib::Color, input::MAX_GAMEPAD_ID> colors = {{
      ::raylib::BLUE,
      ::raylib::ORANGE,
      ::raylib::PURPLE,
      ::raylib::SKYBLUE,
      ::raylib::DARKGREEN,
      ::raylib::BEIGE,
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

  ::raylib::Color release_and_get_next(size_t id) {
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

  ::raylib::Color get_next_NO_STORE(size_t start = 0) {
    int index = bitset_utils::get_next_disabled_bit(used, start);
    if (index == -1) {
      index = 0;
    }
    return colors[index];
  }

  ::raylib::Color get_next_available(size_t id, size_t start = 0) {
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

struct AIControlled : afterhours::BaseComponent {
  vec2 target{0.f, 0.f};
};

struct AIDifficulty : afterhours::BaseComponent {
  enum class Difficulty { Easy, Medium, Hard, Expert };

  Difficulty difficulty{Difficulty::Medium};

  AIDifficulty(Difficulty diff = Difficulty::Medium) : difficulty(diff) {}
};

struct AIMode : afterhours::BaseComponent {
  // If true, the mode will be kept in sync with RoundManager::active_round_type
  bool follow_round_type{true};
  RoundType mode{static_cast<RoundType>(0)};

  AIMode(RoundType m = static_cast<RoundType>(0), bool follow = true)
      : follow_round_type(follow), mode(m) {}
};

struct AIParams : afterhours::BaseComponent {
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

struct AIBoostCooldown : afterhours::BaseComponent {
  float next_allowed_time = 0.0f;
  float cooldown_seconds = 3.0f;
};

struct HasEntityIDBasedColor : afterhours::HasColor {
  afterhours::EntityID id{-1};
  ::raylib::Color default_{::raylib::RAYWHITE};
  HasEntityIDBasedColor(afterhours::EntityID id_in, ::raylib::Color col, ::raylib::Color backup)
      : afterhours::HasColor(col), id(id_in), default_(backup) {}
};

struct TracksEntity : afterhours::BaseComponent {
  afterhours::EntityID id{-1};
  vec2 offset = vec2{0, 0};
  TracksEntity(afterhours::EntityID id_, vec2 off) : id(id_), offset(off) {}
};

using CollisionConfig = ::afterhours::collision::CollisionConfig;

struct Transform : afterhours::BaseComponent {
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

struct TireMarkComponent : afterhours::BaseComponent {
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

struct CanDamage : afterhours::BaseComponent {
  afterhours::EntityID id;
  int amount;

  CanDamage(afterhours::EntityID id_in, int amount_in) : id{id_in}, amount{amount_in} {}
};

struct HasLifetime : afterhours::BaseComponent {
  float lifetime;
  HasLifetime(float life) : lifetime(life) {}
};

struct HasHealth : afterhours::BaseComponent {
  int max_amount{0};
  int amount{0};

  float iframes = 0.5f;
  float iframesReset = 0.5f;

  std::optional<afterhours::EntityID> last_damaged_by{};

  void pass_time(float dt) {
    if (iframes > 0)
      iframes -= dt;
  }

  HasHealth(int max_amount_in)
      : max_amount{max_amount_in}, amount{max_amount_in} {}

  HasHealth(int max_amount_in, int amount_in)
      : max_amount{max_amount_in}, amount{amount_in} {}
};

struct PlayerID : afterhours::BaseComponent {
  input::GamepadID id;
  PlayerID(input::GamepadID i) : id(i) {}
};

struct HasMultipleLives : afterhours::BaseComponent {
  int num_lives_remaining;
  HasMultipleLives(int num_lives) : num_lives_remaining(num_lives) {}
};

struct HasKillCountTracker : afterhours::BaseComponent {
  int kills = 0;
  HasKillCountTracker() = default;
  HasKillCountTracker(int initial_kills) : kills(initial_kills) {}
};

struct HasTagAndGoTracking : afterhours::BaseComponent {
  float time_as_not_it = 0.0f;
  bool is_tagger = false;
  float last_tag_time = -1.f;
  HasTagAndGoTracking() = default;
};

struct HasHippoCollection : afterhours::BaseComponent {
  int hippos_collected = 0;

  void collect_hippo() { hippos_collected++; }

  int get_hippo_count() const { return hippos_collected; }
};

struct HippoItem : afterhours::BaseComponent {
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
struct CanWrapAround : afterhours::BaseComponent {
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

struct HasLabels : public afterhours::BaseComponent {
  std::vector<LabelInfo> label_info{};

  HasLabels() = default;

  HasLabels(std::vector<LabelInfo> labels) : label_info{std::move(labels)} {}
};

struct CollisionAbsorber : public afterhours::BaseComponent {
  CollisionAbsorber() = default;

  enum class AbsorberType {
    Absorber,
    Absorbed, // Cleans up upon collision with a differently parented Absorber
  };

  CollisionAbsorber(AbsorberType absorber_type_in,
                    std::optional<size_t> parent_id_in = std::nullopt)
      : afterhours::BaseComponent{}, absorber_type{absorber_type_in},
        parent_id{parent_id_in} {}

  // Affects cleanup if objects touching are of opposite types
  AbsorberType absorber_type{AbsorberType::Absorber};

  // Optionally ignore collision if containing the same parent
  std::optional<afterhours::EntityID> parent_id{std::nullopt};
};

struct CarAffector : afterhours::BaseComponent {};
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

struct HasShader : afterhours::BaseComponent {
  std::vector<ShaderType> shaders; // Multiple shaders per entity using enums
  RenderPriority render_priority = RenderPriority::Entities; // When to render
  bool enabled = true;

  // Cache for fast shader lookups (mutable for const methods)
  mutable std::optional<std::unordered_set<ShaderType>> shader_set_cache;

  // Easy debugging
  std::string get_debug_info() const {
    if (shaders.empty()) {
      return "No shaders";
    }

    // Build debug info with minimal allocations
    std::string result = "Shaders: ";
    result.reserve(shaders.size() * 15 + 20); // Estimate size needed

    for (const auto &shader : shaders) {
      result += ShaderUtils::to_string(shader);
      result += " ";
    }

    result += "Priority: " + std::to_string(static_cast<int>(render_priority));
    return result;
  }

  // Fast shader validation using cached set
  bool has_shader(ShaderType shader) const {
    // Rebuild cache if shaders changed
    if (!shader_set_cache.has_value()) {
      shader_set_cache =
          std::unordered_set<ShaderType>(shaders.begin(), shaders.end());
    }
    return shader_set_cache->contains(shader);
  }

  // Constructor helpers
  HasShader(ShaderType shader) : shaders{shader} {}
  HasShader(const std::vector<ShaderType> &shader_list)
      : shaders(shader_list) {}

  // Methods to modify shaders (invalidate cache)
  void add_shader(ShaderType shader) {
    shaders.push_back(shader);
    shader_set_cache.reset(); // Invalidate cache
  }

  void remove_shader(ShaderType shader) {
    auto it = std::find(shaders.begin(), shaders.end(), shader);
    if (it != shaders.end()) {
      shaders.erase(it);
      shader_set_cache.reset(); // Invalidate cache
    }
  }

  void clear_shaders() {
    shaders.clear();
    shader_set_cache.reset(); // Invalidate cache
  }
};

struct WantsBoost : afterhours::BaseComponent {};

struct HonkState : afterhours::BaseComponent {
  bool was_down = false;
};

#include "components_weapons.h"

struct TeamID : afterhours::BaseComponent {
  int team_id; // 0 = Team A, 1 = Team B, -1 = No Team (individual mode)
  TeamID(int id = -1) : team_id(id) {}
};
