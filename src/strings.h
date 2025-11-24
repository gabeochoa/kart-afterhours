#pragma once

#include <functional>
#include "magic_enum/magic_enum.hpp"
#include <map>
#include <string>

namespace strings {

// Enum for all translatable strings in the game
enum struct i18n {
  // Main menu
  play,
  about,
  exit,

  // Game states
  loading,
  gameover,
  victory,

  // UI elements
  start,
  back,
  continue_game,
  quit,
  exit_game,
  back_to_setup,
  select_map,

  // Settings
  settings,
  volume,
  fullscreen,
  resolution,
  language,

  round_settings,
  allow_tag_backs,

  // Settings
  master_volume,
  music_volume,
  sfx_volume,
  post_processing,

  round_end,
  unknown,

  // "Paused" Screen
  paused,
  resume,

  // Round time
  round_length,
  unlimited,

  // AI difficulty settings
  easy,
  medium,
  hard,
  expert,

  // Player Statistics
  lives_label,
  kills_label,
  hippos_label,
  hippos_zero,
  not_it_timer,

  // Round Settings Labels
  win_condition_label,
  num_lives_label,
  round_length_with_time,
  total_hippos_label,

  // Count for array sizing
  Count
};

// Global translation map - will be populated by language files
extern std::map<i18n, std::string> pre_translation;

// Helper function to get translated string
[[nodiscard]] inline std::string get_string(const i18n &key) {
  if (!pre_translation.contains(key)) {
    // Fallback to enum name if translation missing
    return std::string(magic_enum::enum_name<i18n>(key));
  }
  return pre_translation.at(key);
}

// Helper function to get translated string with fallback
[[nodiscard]] inline std::string get_string(const i18n &key,
                                            const std::string &fallback) {
  if (!pre_translation.contains(key)) {
    return fallback;
  }
  return pre_translation.at(key);
}

} // namespace strings
