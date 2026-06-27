#include "settings.h"
#include "library/music_library.h"
#include "rl.h"
#include "library/sound_library.h"
#include <fstream>
#include <magic_enum/magic_enum.hpp>
#include <sstream>

#include "round_settings.h"

using namespace afterhours;

void to_json(nlohmann::json &j, const window_manager::Resolution &resolution) {
  j = nlohmann::json{
      {"width", resolution.width},
      {"height", resolution.height},
  };
}

void from_json(const nlohmann::json &j,
               window_manager::Resolution &resolution) {
  j.at("width").get_to(resolution.width);
  j.at("height").get_to(resolution.height);
}

void to_json(nlohmann::json &j, const translation_manager::Language &language) {
  j = magic_enum::enum_name(language);
}

void from_json(const nlohmann::json &j,
               translation_manager::Language &language) {
  if (j.is_string()) {
    auto lang_enum = magic_enum::enum_cast<translation_manager::Language>(
        j.get<std::string>());
    if (lang_enum.has_value()) {
      language = lang_enum.value();
    }
  } else {
    language = translation_manager::Language::English;
  }
}

bool Settings::load_save_file(int width, int height) {
  auto &data = Settings::get();
  data.resolution.width = width;
  data.resolution.height = height;

  if (!settings::load<SettingsData>()) {
    return false;
  }

  Settings::refresh_settings();
  Settings::load_round_settings();
  return true;
}

void Settings::write_save_file() {
  Settings::save_round_settings();
  settings::save<SettingsData>();
}

void Settings::reset() {
  auto &data = Settings::get();
  data = SettingsData();
  Settings::refresh_settings();
}

int Settings::get_screen_width() { return Settings::get().resolution.width; }

int Settings::get_screen_height() { return Settings::get().resolution.height; }

void Settings::update_resolution(window_manager::Resolution rez) {
  Settings::get().resolution = rez;
}

float Settings::get_music_volume() {
  return Settings::get().music_volume.str();
}

void Settings::update_music_volume(float vol) {
  MusicLibrary::get().update_volume(vol);
  Settings::get().music_volume.set(vol);
}

float Settings::get_sfx_volume() { return Settings::get().sfx_volume.str(); }

void Settings::update_sfx_volume(float vol) {
  SoundLibrary::get().update_volume(vol);
  Settings::get().sfx_volume.set(vol);
}

float Settings::get_master_volume() {
  return Settings::get().master_volume.str();
}

void Settings::update_master_volume(float vol) {
  Settings::update_music_volume(Settings::get().music_volume.str());
  Settings::update_sfx_volume(Settings::get().sfx_volume.str());
  raylib::SetMasterVolume(vol);
  Settings::get().master_volume.set(vol);
}

void match_fullscreen_to_setting(bool fs_enabled) {
  if (raylib::IsWindowFullscreen() && fs_enabled)
    return;
  if (!raylib::IsWindowFullscreen() && !fs_enabled)
    return;
  raylib::ToggleFullscreen();
}

void Settings::refresh_settings() {
  auto &data = Settings::get();
  Settings::update_music_volume(data.music_volume.str());
  Settings::update_sfx_volume(data.sfx_volume.str());
  Settings::update_master_volume(data.master_volume.str());
  match_fullscreen_to_setting(data.fullscreen_enabled);
}

void Settings::toggle_fullscreen() {
  auto &data = Settings::get();
  data.fullscreen_enabled = !data.fullscreen_enabled;
  raylib::ToggleFullscreen();
}

bool &Settings::get_fullscreen_enabled() {
  return Settings::get().fullscreen_enabled;
}

bool &Settings::get_post_processing_enabled() {
  return Settings::get().post_processing_enabled;
}

void Settings::toggle_post_processing() {
  auto &data = Settings::get();
  data.post_processing_enabled = !data.post_processing_enabled;
}

translation_manager::Language Settings::get_language() {
  return Settings::get().language;
}

void Settings::set_language(translation_manager::Language language) {
  Settings::get().language = language;
}

void Settings::save_round_settings() {
  Settings::get().round_settings = RoundManager::get().to_json();
}

void Settings::load_round_settings() {
  auto &data = Settings::get();
  if (!data.round_settings.is_null()) {
    RoundManager::get().from_json(data.round_settings);
  }
}
