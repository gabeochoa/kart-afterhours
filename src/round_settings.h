
#pragma once

#include "library.h"
#include "rl.h"
#include <bitset>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include "weapons.h"

// Car size constants
namespace CarSizes {
constexpr vec2 NORMAL_CAR_SIZE = vec2{15.f, 25.f};
constexpr float NORMAL_SPRITE_SCALE = 1.0f;
constexpr float CAT_SPRITE_SCALE = 2.0f;
constexpr float CAT_SIZE_MULTIPLIER = 2.0f;
} // namespace CarSizes

enum struct RoundType : size_t {
  Lives,
  Kills,
  Hippo,
  CatAndMouse,
};
constexpr static size_t num_round_types = magic_enum::enum_count<RoundType>();
constexpr size_t enum_to_index(RoundType type) {
  return static_cast<size_t>(type);
}
constexpr static auto RoundType_NAMES = magic_enum::enum_names<RoundType>();

struct RoundSettings {
  WeaponSet enabled_weapons = WeaponSet().set(0);

  enum struct TimeOptions : size_t {
    Unlimited,
    Seconds_10,
    Seconds_30,
    Minutes_1,
  } time_option = TimeOptions::Unlimited;

  enum struct GameState : size_t {
    Countdown,
    InGame,
    GameOver,
  } state = GameState::Countdown;

  // Countdown before game starts (players can drive around)
  float countdown_before_start = 3.0f; // 3 seconds to get ready

  // Whether to show countdown timer in UI
  bool show_countdown_timer = true;

private:
  static float get_time_from_option(TimeOptions option) {
    switch (option) {
    case TimeOptions::Unlimited:
      return -1.0f;
    case TimeOptions::Seconds_10:
      return 10.0f;
    case TimeOptions::Seconds_30:
      return 30.0f;
    case TimeOptions::Minutes_1:
      return 60.0f;
    }
    return -1.0f; // Default fallback
  }

public:
  float current_round_time = -1;
  void set_time_option(const size_t index) {
    time_option = magic_enum::enum_value<TimeOptions>(index);
    reset_round_time();
  }

  void reset_round_time() {
    current_round_time = get_time_from_option(time_option);
  }

  void reset_countdown() {
    countdown_before_start = 3.0f;
    state = GameState::Countdown;
  }

  // Override in derived classes to clear per-round runtime values
  virtual void reset_temp_data() {}

  virtual ~RoundSettings() = default;
};

// Default time option for timer-using game modes
constexpr auto DEFAULT_TIMER_TIME_OPTION =
    RoundSettings::TimeOptions::Minutes_1;

struct RoundLivesSettings : RoundSettings {
  int num_starting_lives = 1;
};

struct RoundKillsSettings : RoundSettings {
  RoundKillsSettings() {
    time_option = DEFAULT_TIMER_TIME_OPTION;
    reset_round_time();
  }
};

struct RoundHippoSettings : RoundSettings {
  int total_hippos = 50;

  struct TempData {
    int hippos_spawned_total = 0;
  } data;

  void reset_temp_data() override { data = TempData{}; }

  void reset_spawn_counter() { data.hippos_spawned_total = 0; }

  // TODO why do we need this?
  // Override default time option for hippo game
  RoundHippoSettings() {
    time_option = DEFAULT_TIMER_TIME_OPTION;
    reset_round_time();
  }

  void set_total_hippos(int count) {
    if (count < 1) {
      log_error("Invalid total_hippos: {} (must be >= 1)", count);
      total_hippos = 1;
    } else if (count > 1000) {
      log_error("Invalid total_hippos: {} (must be <= 1000)", count);
      total_hippos = 1000;
    } else {
      total_hippos = count;
    }
  }
};

struct RoundCatAndMouseSettings : RoundSettings {
  // TODO: audit cooldown time setting
  // TODO: Add "tag back" rule option (allow/disallow tag backs)
  // TODO: Add UI announcement of who is the current cat on game start

  // Whether to announce the cat in UI
  bool announce_cat_in_ui = true;

  // how long a player is safe after being tagged
  // TODO: Add tag cooldown setting to settings UI
  float tag_cooldown_time = 2.0f;

  float speed_multiplier = 0.7f;

  // Override default time option for cat and mouse
  RoundCatAndMouseSettings() {
    time_option = DEFAULT_TIMER_TIME_OPTION;
    reset_round_time();
  }
};

SINGLETON_FWD(RoundManager)
struct RoundManager {
  SINGLETON(RoundManager)

  std::array<std::unique_ptr<RoundSettings>, num_round_types> settings;

  RoundManager() {
    // TODO Load previous settings from file

    settings[enum_to_index(RoundType::Lives)] =
        std::make_unique<RoundLivesSettings>();
    settings[enum_to_index(RoundType::Kills)] =
        std::make_unique<RoundKillsSettings>();
    settings[enum_to_index(RoundType::Hippo)] =
        std::make_unique<RoundHippoSettings>();
    settings[enum_to_index(RoundType::CatAndMouse)] =
        std::make_unique<RoundCatAndMouseSettings>();

    settings[0]->enabled_weapons.reset().set(0);
    settings[1]->enabled_weapons.reset().set(1);
    settings[2]->enabled_weapons.reset().set(2);
  }

  RoundType active_round_type = RoundType::CatAndMouse;

  RoundSettings &get_active_settings() {
    return *(settings[enum_to_index(active_round_type)]);
  };

  const RoundSettings &get_active_settings() const {
    return *(settings[enum_to_index(active_round_type)]);
  };

  template <typename T> T &get_active_rt() {
    auto &rt_settings = get_active_settings();
    switch (active_round_type) {
    case RoundType::Lives:
      return static_cast<T &>(rt_settings);
    case RoundType::Kills:
      return static_cast<T &>(rt_settings);
    case RoundType::Hippo:
      return static_cast<T &>(rt_settings);
    case RoundType::CatAndMouse:
      return static_cast<T &>(rt_settings);
    default:
      // This should never happen, but provides a clear error
      log_error("Invalid round type in get_active_rt: {}",
                static_cast<size_t>(active_round_type));
      return static_cast<T &>(rt_settings);
    }
  }

  template <typename T> const T &get_active_rt() const {
    const auto &rt_settings = get_active_settings();
    switch (active_round_type) {
    case RoundType::Lives:
      return static_cast<const T &>(rt_settings);
    case RoundType::Kills:
      return static_cast<const T &>(rt_settings);
    case RoundType::Hippo:
      return static_cast<const T &>(rt_settings);
    case RoundType::CatAndMouse:
      return static_cast<const T &>(rt_settings);
    default:
      // This should never happen, but provides a clear error
      log_error("Invalid round type in get_active_rt const: {}",
                static_cast<size_t>(active_round_type));
      return static_cast<const T &>(rt_settings);
    }
  }

  void set_active_round_type(const int index) {
    active_round_type =
        magic_enum::enum_cast<RoundType>(index).value_or(RoundType::Lives);

    switch (active_round_type) {
    case RoundType::Hippo: {
      auto &hippo_settings = get_active_rt<RoundHippoSettings>();
      hippo_settings.time_option = DEFAULT_TIMER_TIME_OPTION;
      hippo_settings.reset_round_time();
      break;
    }
    case RoundType::CatAndMouse: {
      auto &cat_mouse_settings = get_active_rt<RoundCatAndMouseSettings>();
      cat_mouse_settings.time_option = DEFAULT_TIMER_TIME_OPTION;
      cat_mouse_settings.reset_round_time();
      break;
    }
    case RoundType::Kills: {
      auto &kills_settings = get_active_rt<RoundKillsSettings>();
      kills_settings.time_option = DEFAULT_TIMER_TIME_OPTION;
      kills_settings.reset_round_time();
      break;
    }
    case RoundType::Lives:
      // Lives mode doesn't use timers, so no initialization needed
      break;
    default:
      log_error("Unknown round type in set_active_round_type: {}",
                static_cast<size_t>(active_round_type));
      break;
    }
  }

  WeaponSet &get_enabled_weapons() {
    return get_active_settings().enabled_weapons;
  }

  void set_enabled_weapons(const unsigned long enabled_bits) {
    get_active_settings().enabled_weapons = WeaponSet(enabled_bits);
  }

  int fetch_num_starting_lives() {
    if (active_round_type == RoundType::Lives) {
      auto &lives_settings = get_active_rt<RoundLivesSettings>();
      return lives_settings.num_starting_lives;
    }
    return 3; // default fallback for non-Lives round types
  }

  bool uses_countdown() const {
    switch (active_round_type) {
    case RoundType::Lives:
      return false;
    case RoundType::Kills:
      return true;
    case RoundType::Hippo:
      return true;
    case RoundType::CatAndMouse:
      return true;
    default:
      log_error("Invalid round type in uses_countdown: {}",
                static_cast<size_t>(active_round_type));
      return false;
    }
  }

  bool uses_timer() const {
    switch (active_round_type) {
    case RoundType::Lives:
      return false;
    case RoundType::Kills:
      return true;
    case RoundType::Hippo:
      return true;
    case RoundType::CatAndMouse:
      return true;
    default:
      log_error("Invalid round type in uses_timer: {}",
                static_cast<size_t>(active_round_type));
      return false;
    }
  }

  void reset_for_new_round() {
    auto &active_settings = get_active_settings();
    active_settings.reset_countdown();
    active_settings.reset_round_time();
    active_settings.reset_temp_data();
  }

  float get_current_round_time() const {

    switch (active_round_type) {
    case RoundType::Lives:
      return -1.0f; // Lives mode doesn't use timer
    case RoundType::Kills:
      return get_active_rt<RoundKillsSettings>().current_round_time;
    case RoundType::Hippo:
      return get_active_rt<RoundHippoSettings>().current_round_time;
    case RoundType::CatAndMouse:
      return get_active_rt<RoundCatAndMouseSettings>().current_round_time;
    default:
      log_error("Invalid round type in get_current_round_time: {}",
                static_cast<size_t>(active_round_type));
      return -1.0f;
    }
  }

  nlohmann::json to_json() const {
    nlohmann::json j;

    j["active_round_type"] = static_cast<size_t>(active_round_type);

    nlohmann::json settings_j;
    for (size_t i = 0; i < num_round_types; ++i) {
      auto round_type = static_cast<RoundType>(i);
      auto round_name = std::string(RoundType_NAMES[i]);

      nlohmann::json round_j;
      round_j["enabled_weapons"] = settings[i]->enabled_weapons.to_ulong();

      // Time settings are now in base class, so add to all round types
      round_j["time_option"] = static_cast<size_t>(settings[i]->time_option);

      // Countdown settings are now in base class, so add to all round types
      round_j["show_countdown_timer"] = settings[i]->show_countdown_timer;

      switch (round_type) {
      case RoundType::Lives: {
        auto &lives_settings =
            static_cast<const RoundLivesSettings &>(*settings[i]);
        round_j["num_starting_lives"] = lives_settings.num_starting_lives;
        break;
      }
      case RoundType::Kills: {
        // Time and countdown settings handled above
        break;
      }
      case RoundType::Hippo: {
        auto &hippo_settings =
            static_cast<const RoundHippoSettings &>(*settings[i]);
        round_j["total_hippos"] = hippo_settings.total_hippos;
        break;
      }
      case RoundType::CatAndMouse: {
        auto &cat_settings =
            static_cast<const RoundCatAndMouseSettings &>(*settings[i]);
        round_j["speed_multiplier"] = cat_settings.speed_multiplier;
        break;
      }
      }

      settings_j[round_name] = round_j;
    }

    j["settings"] = settings_j;
    return j;
  }

  void from_json(const nlohmann::json &j) {
    if (j.contains("active_round_type")) {
      active_round_type =
          static_cast<RoundType>(j["active_round_type"].get<size_t>());
    }

    if (j.contains("settings")) {
      auto settings_j = j["settings"];

      for (size_t i = 0; i < num_round_types; ++i) {
        auto round_type = static_cast<RoundType>(i);
        auto round_name = std::string(RoundType_NAMES[i]);

        if (settings_j.contains(round_name)) {
          auto round_j = settings_j[round_name];

          if (round_j.contains("enabled_weapons")) {
            settings[i]->enabled_weapons =
                WeaponSet(round_j["enabled_weapons"].get<unsigned long>());
          }

          // Time settings are now in base class, so handle for all round types
          if (round_j.contains("time_option")) {
            settings[i]->set_time_option(round_j["time_option"].get<size_t>());
          }

          // Countdown settings are now in base class, so handle for all round
          // types
          if (round_j.contains("show_countdown_timer")) {
            settings[i]->show_countdown_timer =
                round_j["show_countdown_timer"].get<bool>();
          }

          switch (round_type) {
          case RoundType::Lives: {
            auto &lives_settings =
                static_cast<RoundLivesSettings &>(*settings[i]);
            if (round_j.contains("num_starting_lives")) {
              lives_settings.num_starting_lives =
                  round_j["num_starting_lives"].get<int>();
            }
            break;
          }
          case RoundType::Kills: {
            // Time and countdown settings handled above
            break;
          }
          case RoundType::Hippo: {
            auto &hippo_settings =
                static_cast<RoundHippoSettings &>(*settings[i]);
            if (round_j.contains("total_hippos")) {
              hippo_settings.total_hippos = round_j["total_hippos"].get<int>();
            }
            break;
          }
          case RoundType::CatAndMouse: {
            auto &cat_settings =
                static_cast<RoundCatAndMouseSettings &>(*settings[i]);
            if (round_j.contains("speed_multiplier")) {
              cat_settings.speed_multiplier =
                  round_j["speed_multiplier"].get<float>();
            }
            break;
          }
          }
        }
      }
    }
  }
};
