#pragma once

#include "magic_enum/magic_enum.hpp"
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
  time_10_seconds,
  time_30_seconds,
  time_1_minute,

  // Round type names, as the player reads them on the rules screen
  round_type_lives,
  round_type_kills,
  round_type_hippo,
  round_type_tag,

  // Weapon names
  weapon_cannon,
  weapon_shotgun,
  weapon_sniper,
  weapon_machine_gun,

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

} // namespace strings
