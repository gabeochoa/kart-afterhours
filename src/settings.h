#pragma once

#define AFTERHOURS_SETTINGS_OUTPUT_JSON
#include "translation_manager.h"
#include <afterhours/src/plugins/settings.h>
#include <afterhours/src/plugins/window_manager.h>
#include <nlohmann/json.hpp>

struct Pct {
  float data = 0.f;
  Pct() = default;
  Pct(float initial) { set(initial); }
  void set(float data_) { data = std::min(1.f, std::max(0.f, data_)); }
  float str() const { return data; }
};

struct SettingsData {
  ::afterhours::window_manager::Resolution resolution = {
      .width = 1280,
      .height = 720,
  };

  Pct master_volume = 0.1f;
  Pct music_volume = 0.1f;
  Pct sfx_volume = 0.1f;

  bool fullscreen_enabled = false;
  bool post_processing_enabled = true;

  translation_manager::Language language =
      translation_manager::Language::English;

  nlohmann::json round_settings;

  nlohmann::json to_json() const;
  static SettingsData from_json(const nlohmann::json &j);
};

using SettingsProvider = afterhours::settings::ProvidesSettings<SettingsData>;

struct Settings {
  static SettingsData &get() {
    return afterhours::settings::get_data<SettingsData>();
  }

  static const SettingsData &get_const() {
    return afterhours::settings::get_data_const<SettingsData>();
  }

  static bool load_save_file(int width, int height);
  static void write_save_file();
  static void reset();
  static void refresh_settings();
  static int get_screen_width();
  static int get_screen_height();
  static void update_resolution(::afterhours::window_manager::Resolution rez);
  static float get_music_volume();
  static void update_music_volume(float vol);
  static float get_sfx_volume();
  static void update_sfx_volume(float vol);
  static float get_master_volume();
  static void update_master_volume(float vol);
  static bool &get_fullscreen_enabled();
  static void toggle_fullscreen();
  static bool &get_post_processing_enabled();
  static void toggle_post_processing();
  static translation_manager::Language get_language();
  static void set_language(translation_manager::Language language);
  static void save_round_settings();
  static void load_round_settings();
};
