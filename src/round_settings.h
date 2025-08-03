
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
  Score,
  CatAndMouse,
};
constexpr static size_t num_round_types = magic_enum::enum_count<RoundType>();
constexpr size_t enum_to_index(RoundType type) {
  return static_cast<size_t>(type);
}
constexpr static auto RoundType_NAMES = magic_enum::enum_names<RoundType>();

struct RoundSettings {
  WeaponSet enabled_weapons = WeaponSet().set(0);
};

struct RoundLivesSettings : RoundSettings {
  int num_starting_lives = 1;
};

struct RoundKillsSettings : RoundSettings {
  enum struct TimeOptions : size_t {
    Unlimited,
    Seconds_30,
    Minutes_1,
  } time_option = TimeOptions::Unlimited;

private:
  static float get_time_from_option(TimeOptions option) {
    switch (option) {
    case TimeOptions::Unlimited:
      return -1.0f;
    case TimeOptions::Seconds_30:
      return 30.0f;
    case TimeOptions::Minutes_1:
      return 60.0f;
    }
    return -1.0f; // Default fallback for kills
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
};

struct RoundScoreSettings : RoundSettings {
  int score_needed_to_win = 3;
};

struct RoundCatAndMouseSettings : RoundSettings {
  // TODO: audit cooldown time setting
  // TODO: Add "tag back" rule option (allow/disallow tag backs)
  // TODO: Add UI announcement of who is the current cat on game start

  enum struct GameState : size_t {
    Countdown,
    InGame,
    GameOver,
  } state = GameState::Countdown;

  enum struct TimeOptions : size_t {
    Unlimited,
    Seconds_10,
    Seconds_30,
    Minutes_1,
  } time_option = TimeOptions::Minutes_1;

  // Countdown before game starts (players can drive around)
  float countdown_before_start = 3.0f; // 3 seconds to get ready

  // Whether to announce the cat in UI
  bool announce_cat_in_ui = true;

  // Whether to show countdown timer in UI
  bool show_countdown_timer = true;

  // how long a player is safe after being tagged
  // TODO: Add tag cooldown setting to settings UI
  float tag_cooldown_time = 2.0f;

  float speed_multiplier = 0.7f;

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
    return 30.0f; // Default fallback
  }

public:
  float current_round_time = 30.f; // Default to 30 seconds
  void set_time_option(const size_t index) {
    time_option = magic_enum::enum_value<TimeOptions>(index);
    reset_round_time();
  }

  void reset_round_time() {
    current_round_time = get_time_from_option(time_option);
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
    settings[enum_to_index(RoundType::Score)] =
        std::make_unique<RoundScoreSettings>();
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

  template <typename T> T &get_active_rt() {
    auto &rt_settings = get_active_settings();
    switch (active_round_type) {
    case RoundType::Lives:
      return static_cast<T &>(rt_settings);
    case RoundType::Kills:
      return static_cast<T &>(rt_settings);
    case RoundType::Score:
      return static_cast<T &>(rt_settings);
    case RoundType::CatAndMouse:
      return static_cast<T &>(rt_settings);
    }
  }

  void set_active_round_type(const int index) {
    active_round_type =
        magic_enum::enum_cast<RoundType>(index).value_or(RoundType::Lives);
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

  nlohmann::json to_json() const {
    nlohmann::json j;

    j["active_round_type"] = static_cast<size_t>(active_round_type);

    nlohmann::json settings_j;
    for (size_t i = 0; i < num_round_types; ++i) {
      auto round_type = static_cast<RoundType>(i);
      auto round_name = std::string(RoundType_NAMES[i]);

      nlohmann::json round_j;
      round_j["enabled_weapons"] = settings[i]->enabled_weapons.to_ulong();

      switch (round_type) {
      case RoundType::Lives: {
        auto &lives_settings =
            static_cast<const RoundLivesSettings &>(*settings[i]);
        round_j["num_starting_lives"] = lives_settings.num_starting_lives;
        break;
      }
      case RoundType::Kills: {
        auto &kills_settings =
            static_cast<const RoundKillsSettings &>(*settings[i]);
        round_j["time_option"] =
            static_cast<size_t>(kills_settings.time_option);
        break;
      }
      case RoundType::Score: {
        auto &score_settings =
            static_cast<const RoundScoreSettings &>(*settings[i]);
        round_j["score_needed_to_win"] = score_settings.score_needed_to_win;
        break;
      }
      case RoundType::CatAndMouse: {
        auto &cat_settings =
            static_cast<const RoundCatAndMouseSettings &>(*settings[i]);
        round_j["time_option"] = static_cast<size_t>(cat_settings.time_option);
        round_j["show_countdown_timer"] = cat_settings.show_countdown_timer;
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
            auto &kills_settings =
                static_cast<RoundKillsSettings &>(*settings[i]);
            if (round_j.contains("time_option")) {
              kills_settings.set_time_option(
                  round_j["time_option"].get<size_t>());
            }
            break;
          }
          case RoundType::Score: {
            auto &score_settings =
                static_cast<RoundScoreSettings &>(*settings[i]);
            if (round_j.contains("score_needed_to_win")) {
              score_settings.score_needed_to_win =
                  round_j["score_needed_to_win"].get<int>();
            }
            break;
          }
          case RoundType::CatAndMouse: {
            auto &cat_settings =
                static_cast<RoundCatAndMouseSettings &>(*settings[i]);
            if (round_j.contains("time_option")) {
              cat_settings.set_time_option(
                  round_j["time_option"].get<size_t>());
            }
            if (round_j.contains("show_countdown_timer")) {
              cat_settings.show_countdown_timer =
                  round_j["show_countdown_timer"].get<bool>();
            }
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
