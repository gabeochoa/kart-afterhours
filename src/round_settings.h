
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

struct RoundSettings {};

SINGLETON_FWD(RoundManager)
struct RoundManager {
  SINGLETON(RoundManager)

  std::array<std::unique_ptr<RoundSettings>, num_round_types> settings;

  WeaponSet enabled_weapons = WeaponSet().set(0);

  RoundManager() {
    // TODO Load previous settings from file

    settings[enum_to_index(RoundType::Lives)] =
        std::make_unique<RoundSettings>();
    settings[enum_to_index(RoundType::Kills)] =
        std::make_unique<RoundSettings>();
    settings[enum_to_index(RoundType::Score)] =
        std::make_unique<RoundSettings>();
  }

  RoundType active_round_type = RoundType::Lives;

  void set_active_round_type(int index) {
    active_round_type =
        magic_enum::enum_cast<RoundType>(index).value_or(RoundType::Lives);
  }
};
