#pragma once

#include "../rl.h"
#include <afterhours/src/plugins/sound_system.h>
#include <afterhours/src/plugins/files.h>
#include <magic_enum/magic_enum.hpp>

enum struct SoundFile {
  UI_Select,
  UI_Move,
  Engine_Idle_Short,
  Round_Start,
  Weapon_Canon_Shot,
  Weapon_Shotgun_Shot,
  Weapon_Sniper_Shot,
  Tiny_Gears_Sequence_045,
};

using SoundLibrary = afterhours::sound_system::SoundLibrary;
using SoundEmitter = afterhours::sound_system::SoundEmitter;
using PlaySoundRequest = afterhours::sound_system::PlaySoundRequest;

inline const char *sound_file_to_str(SoundFile sf) {
  return magic_enum::enum_name(sf).data();
}

void load_sounds();
