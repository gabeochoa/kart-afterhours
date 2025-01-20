
#pragma once

#include "library.h"
#include "rl.h"

#include "weapons.h"

enum struct RoundType : size_t {
  Lives,
  Kills,
  Score,
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
  int num_starting_lives = 3;
};

struct RoundKillsSettings : RoundSettings {
  float round_time = 30.f;
};

struct RoundScoreSettings : RoundSettings {
  int score_needed_to_win = 3;
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

    settings[0]->enabled_weapons.reset().set(0);
    settings[1]->enabled_weapons.reset().set(1);
    settings[2]->enabled_weapons.reset().set(2);
  }

  RoundType active_round_type = RoundType::Lives;

  RoundSettings &get_active_settings() const {
    return *(settings[enum_to_index(active_round_type)]);
  };

  template <typename T> T &get_active_rt() {
    auto &rt_settings = get_active_settings();
    switch (active_round_type) {
    case RoundType::Lives:
      return static_cast<RoundLivesSettings &>(rt_settings);
    case RoundType::Kills:
      return static_cast<RoundKillsSettings &>(rt_settings);
    case RoundType::Score:
      return static_cast<RoundScoreSettings &>(rt_settings);
    }
  }

  void set_active_round_type(int index) {
    active_round_type =
        magic_enum::enum_cast<RoundType>(index).value_or(RoundType::Lives);
  }

  WeaponSet get_enabled_weapons() {
    return get_active_settings().enabled_weapons;
  }

  void set_enabled_weapons(unsigned long enabled_bits) {
    get_active_settings().enabled_weapons = WeaponSet(enabled_bits);
  }
};
