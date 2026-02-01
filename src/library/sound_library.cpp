#include "sound_library.h"
#include <functional>

using namespace afterhours;

void load_sounds() {
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
    case SoundFile::Tiny_Gears_Sequence_045:
      filename =
          "gdc/Bluezone_BC0301_tiny_gears_small_mechanism_sequence_045.wav";
      break;
    }
    SoundLibrary::get().load(
        files::get_resource_path("sounds", filename).string().c_str(),
        sound_file_to_str(file));
  });

  const char *mg_prefix =
      "SPAS-12_-_FIRING_-_Pump_Action_-_Take_1_-_20m_In_Front_-_AB_-_MKH8020_";
  for (int i = 1; i <= 5; ++i) {
    std::string stem = std::string(mg_prefix) + std::to_string(i);
    std::string path = std::string("gdc/") + stem + ".wav";
    SoundLibrary::get().load(
        files::get_resource_path("sounds", path).string().c_str(), stem.c_str());
  }

  const char *boost_prefix = "AIRBrst_Steam_Release_Short_03_JSE_SG_Mono_";
  for (int i = 1; i <= 6; ++i) {
    std::string stem = std::string(boost_prefix) + std::to_string(i);
    std::string path = std::string("gdc/") + stem + ".wav";
    SoundLibrary::get().load(
        files::get_resource_path("sounds", path).string().c_str(), stem.c_str());
  }

  SoundLibrary::get().load(
      files::get_resource_path("sounds", "gdc/"
                                         "1993_Suzuki_VS_800_GL_Intruder_pass-"
                                         "by_back_to_front_asphalt_M-S_LR2.wav").string().c_str(),
      "IntroPassBy_0");
  SoundLibrary::get().load(
      files::get_resource_path(
              "sounds", "gdc/"
                        "VEHCar_1967_Corvette_EXT-Group_A_Approach_In_"
                        "Accelerate_MEDIUM_Lead_car_then_Vette_Left_to_Right_"
                        "02_M1_GoldSND_M1C_101419_aaOVPpPmTQSk_LR1.wav").string().c_str(),
      "IntroPassBy_1");
  SoundLibrary::get().load(
      files::get_resource_path("sounds",
                               "gdc/"
                               "VEHCar_Audi_Q7_EXTERIOR_Approach_Fast_Stop_"
                               "Drive_Away_Fast_ORTF_DRCA_AUQ7_MK012_LR3.wav").string().c_str(),
      "IntroPassBy_2");

  const char *horn_prefix =
      "VEHHorn_Renault_R4_GTL_Horn_Signal_01_Interior_JSE_RR4_Mono_";
  for (int i = 1; i <= 6; ++i) {
    std::string stem = std::string(horn_prefix) + std::to_string(i);
    std::string path = std::string("gdc/") + stem + ".wav";
    SoundLibrary::get().load(
        files::get_resource_path("sounds", path).string().c_str(), stem.c_str());
    for (int copy = 1; copy <= 3; ++copy) {
      std::string alias = stem + std::string("_a") + std::to_string(copy);
      SoundLibrary::get().load(
          files::get_resource_path("sounds", path).string().c_str(),
          alias.c_str());
    }
  }
}

