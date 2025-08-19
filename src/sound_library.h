#pragma once

#include "library.h"
#include <afterhours/src/singleton.h>
//
#include "resources.h"

#include "rl.h"

enum struct SoundFile {
  UI_Select,
  UI_Move,
  Engine_Idle_Short,
  Round_Start,
  Tiny_Gears_Sequence_045,
  Weapon_Sniper_Shot,
  Weapon_Canon_Shot,
  Weapon_Shotgun_Shot,
  // Car_Boost,
  // Weapon_Machinegun_Shot,
};

constexpr static const char *sound_file_to_str(SoundFile sf) {
  switch (sf) {
  case SoundFile::UI_Select:
    return "UISelect";
  case SoundFile::UI_Move:
    return "WaterDripSingle";
  case SoundFile::Engine_Idle_Short:
    return "EngineIdleShort";
  case SoundFile::Round_Start:
    return "RoundStart";
  case SoundFile::Tiny_Gears_Sequence_045:
    return "TinyGearsSequence045";
  case SoundFile::Weapon_Sniper_Shot:
    return "WeaponSniperShot";
  case SoundFile::Weapon_Canon_Shot:
    return "WeaponCanonShot";
  case SoundFile::Weapon_Shotgun_Shot:
    return "WeaponShotgunShot";
  }
  return "";
}

SINGLETON_FWD(SoundLibrary)
struct SoundLibrary {
  SINGLETON(SoundLibrary)

  [[nodiscard]] raylib::Sound &get(const std::string &name) {
    return impl.get(name);
  }
  [[nodiscard]] const raylib::Sound &get(const std::string &name) const {
    return impl.get(name);
  }
  [[nodiscard]] bool contains(const std::string &name) const {
    return impl.contains(name);
  }
  void load(const char *filename, const char *name) {
    impl.load(filename, name);
  }

  void play(SoundFile file) { play(sound_file_to_str(file)); }
  void play(const char *const name) { raylib::PlaySound(get(name)); }

  void play_random_match(const std::string &prefix) {
    impl.get_random_match(prefix).transform(raylib::PlaySound);
  }

  void play_if_none_playing(const std::string &prefix) {
    auto matches = impl.lookup(prefix);
    if (matches.first == matches.second) {
      log_warn("got no matches for your prefix search: {}", prefix);
      return;
    }
    for (auto it = matches.first; it != matches.second; ++it) {
      if (raylib::IsSoundPlaying(it->second)) {
        return;
      }
    }
    raylib::PlaySound(matches.first->second);
  }

  void play_first_available_match(const std::string &prefix) {
    auto matches = impl.lookup(prefix);
    if (matches.first == matches.second) {
      log_warn("got no matches for your prefix search: {}", prefix);
      return;
    }
    for (auto it = matches.first; it != matches.second; ++it) {
      if (!raylib::IsSoundPlaying(it->second)) {
        raylib::PlaySound(it->second);
        return;
      }
    }
    raylib::PlaySound(matches.first->second);
  }

  void update_volume(const float new_v) {
    impl.update_volume(new_v);
    current_volume = new_v;
  }

  void unload_all() { impl.unload_all(); }

private:
  // Note: Read note in MusicLibrary
  float current_volume = 1.f;

  struct SoundLibraryImpl : Library<raylib::Sound> {
    virtual raylib::Sound
    convert_filename_to_object(const char *, const char *filename) override {
      return raylib::LoadSound(filename);
    }
    virtual void unload(raylib::Sound sound) override {
      raylib::UnloadSound(sound);
    }

    void update_volume(const float new_v) {
      for (const auto &kv : storage) {
        log_trace("updating sound volume for {} to {}", kv.first, new_v);
        raylib::SetSoundVolume(kv.second, new_v);
      }
    }
  } impl;
};

constexpr static void load_sounds() {
  magic_enum::enum_for_each<SoundFile>([](auto val) {
    constexpr SoundFile file = val;
    std::string filename;
    switch (file) {
    case SoundFile::UI_Select:
      filename = "gdc/doex_qantum_ui_ui_select_plastic_05_03.wav";
      break;
    case SoundFile::UI_Move:
      filename = "gdc/"
                 "inmotionaudio_cave_design_WATRDrip_SingleDrip03_"
                 "InMotionAudio_CaveDesign.wav";
      break;
    case SoundFile::Engine_Idle_Short:
      filename = "gdc/"
                 "cactuzz_sound_1993_Suzuki_VS_800_GL_Intruder_Onboard,_idle_"
                 "Mix_Loop_Short.wav";
      break;
    case SoundFile::Round_Start:
      filename = "gdc/METLImpt_Metal_Impact-03_MWSFX_TM.wav";
      break;
    case SoundFile::Weapon_Canon_Shot:
      filename = "gdc/Bluezone_BC0302_industrial_lever_switch_039.wav";
      break;
    case SoundFile::Weapon_Shotgun_Shot:
      filename =
          "gdc/Bluezone_BC0296_steampunk_weapon_flare_shot_explosion_003.wav";
      break;
    case SoundFile::Weapon_Sniper_Shot:
      filename = "gdc/CREAMisc_Heavy_Mechanical_Footsteps_03_DDUMAIS_MCSFX.wav";
      break;
      //

    case SoundFile::Tiny_Gears_Sequence_045:
      filename =
          "gdc/Bluezone_BC0301_tiny_gears_small_mechanism_sequence_045.wav";
      break;
    }
    SoundLibrary::get().load(
        Files::get().fetch_resource_path("sounds", filename).c_str(),
        sound_file_to_str(file));
  });

  // Preload machinegun variations for random selection by prefix
  const char *mg_prefix =
      "SPAS-12_-_FIRING_-_Pump_Action_-_Take_1_-_20m_In_Front_-_AB_-_MKH8020_";
  for (int i = 1; i <= 5; ++i) {
    std::string stem = std::string(mg_prefix) + std::to_string(i);
    std::string path = std::string("gdc/") + stem + ".wav";
    SoundLibrary::get().load(
        Files::get().fetch_resource_path("sounds", path).c_str(), stem.c_str());
  }

  // Preload boost variations for random selection by prefix
  const char *boost_prefix = "AIRBrst_Steam_Release_Short_03_JSE_SG_Mono_";
  for (int i = 1; i <= 6; ++i) {
    std::string stem = std::string(boost_prefix) + std::to_string(i);
    std::string path = std::string("gdc/") + stem + ".wav";
    SoundLibrary::get().load(
        Files::get().fetch_resource_path("sounds", path).c_str(), stem.c_str());
  }

  SoundLibrary::get().load(
      Files::get()
          .fetch_resource_path("sounds", "gdc/"
                                         "1993_Suzuki_VS_800_GL_Intruder_pass-"
                                         "by_back_to_front_asphalt_M-S_LR2.wav")
          .c_str(),
      "IntroPassBy_0");
  SoundLibrary::get().load(
      Files::get()
          .fetch_resource_path(
              "sounds", "gdc/"
                        "VEHCar_1967_Corvette_EXT-Group_A_Approach_In_"
                        "Accelerate_MEDIUM_Lead_car_then_Vette_Left_to_Right_"
                        "02_M1_GoldSND_M1C_101419_aaOVPpPmTQSk_LR1.wav")
          .c_str(),
      "IntroPassBy_1");
  SoundLibrary::get().load(
      Files::get()
          .fetch_resource_path("sounds",
                               "gdc/"
                               "VEHCar_Audi_Q7_EXTERIOR_Approach_Fast_Stop_"
                               "Drive_Away_Fast_ORTF_DRCA_AUQ7_MK012_LR3.wav")
          .c_str(),
      "IntroPassBy_2");

  // Preload horn variations for random selection by prefix
  const char *horn_prefix =
      "VEHHorn_Renault_R4_GTL_Horn_Signal_01_Interior_JSE_RR4_Mono_";
  for (int i = 1; i <= 6; ++i) {
    std::string stem = std::string(horn_prefix) + std::to_string(i);
    std::string path = std::string("gdc/") + stem + ".wav";
    SoundLibrary::get().load(
        Files::get().fetch_resource_path("sounds", path).c_str(), stem.c_str());
    // Load additional aliases to allow overlapping playback
    for (int copy = 1; copy <= 3; ++copy) {
      std::string alias = stem + std::string("_a") + std::to_string(copy);
      SoundLibrary::get().load(
          Files::get().fetch_resource_path("sounds", path).c_str(),
          alias.c_str());
    }
  }
}
