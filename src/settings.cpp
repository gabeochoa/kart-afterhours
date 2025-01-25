
#include "settings.h"

#include <algorithm>
#include <memory>
#include <nlohmann/json.hpp>

#include "rl.h"

#include "music_library.h"
#include "sound_library.h"

template <typename T> struct ValueHolder {
  using value_type = T;
  T data;

  ValueHolder(const T &initial) : data(initial) {}
  T &get() { return data; }
  const T &get() const { return data; }
  void set(const T &data_) { data = data_; }

  operator const T &() { return get(); }
};

struct Pct : ValueHolder<float> {
  Pct(const float &initial) : ValueHolder(0.f) { set(initial); }
  void set(const float &data_) { data = std::min(1.f, std::max(0.f, data_)); }
};

struct S_Data {
  afterhours::window_manager::Resolution resolution = {
      .width = 1280,
      .height = 1280,
  };

  Pct master_volume = 0.1f;
  Pct music_volume = 0.1f;
  Pct sfx_volume = 0.1f;

  bool fullscreen_enabled = false;

  fs::path loaded_from;
};

// TODO load last used settings
Settings::Settings() { data = new S_Data(); }
Settings::~Settings() { delete data; }

void Settings::reset() {
  delete data;
  data = new S_Data();
  refresh_settings();
}

int Settings::get_screen_width() const { return data->resolution.width; }
int Settings::get_screen_height() const { return data->resolution.height; }
float Settings::get_music_volume() const { return data->music_volume; }
float Settings::get_sfx_volume() const { return data->sfx_volume; }
float Settings::get_master_volume() const { return data->master_volume; }

void Settings::update_music_volume(float vol) {
  MusicLibrary::get().update_volume(vol);
  data->music_volume = vol;
}
void Settings::update_sfx_volume(float vol) {
  SoundLibrary::get().update_volume(vol);
  data->sfx_volume = vol;
}

void Settings::update_master_volume(float vol) {
  // TODO should these be updated after?
  // because if not, then why are we doing these at all
  update_music_volume(data->music_volume);
  update_sfx_volume(data->sfx_volume);

  raylib::SetMasterVolume(vol);
  data->master_volume = vol;
}

void match_fullscreen_to_setting(bool fs_enabled) {
  if (raylib::IsWindowFullscreen() && fs_enabled)
    return;
  if (!raylib::IsWindowFullscreen() && !fs_enabled)
    return;
  raylib::ToggleFullscreen();
}

void Settings::refresh_settings() {

  // audio settings
  {
    update_music_volume(data->music_volume);
    update_sfx_volume(data->sfx_volume);
    update_master_volume(data->master_volume);
  }

  match_fullscreen_to_setting(data->fullscreen_enabled);
}

void Settings::toggle_fullscreen() {
  data->fullscreen_enabled = !data->fullscreen_enabled;
  raylib::ToggleFullscreen();
}

bool &Settings::get_fullscreen_enabled() { return data->fullscreen_enabled; }

bool Settings::load_save_file(float width, float height) {

  auto settings_places = Files::get().relative_settings();

  size_t file_loc = 0;
  std::ifstream ifs;
  while (true) {
    if (file_loc >= settings_places.size()) {
      std::stringstream buffer;
      buffer << "Failed to find settings file (Read): \n";
      for (auto place : settings_places)
        buffer << place << ", \n";
      log_warn("{}", buffer.str());
      return false;
    }

    ifs = std::ifstream(settings_places[file_loc]);
    if (ifs.is_open()) {
      log_info("opened file {}", settings_places[file_loc]);
      break;
    }
    file_loc++;
  }

  data->loaded_from = settings_places[file_loc];

  try {
    const auto settingsJSON = nlohmann::json::parse(
        ifs, nullptr /*parser_callback_t*/, true /*allow_exceptions=*/,
        true /* ignore comments */);

    this->data->resolution.width = settingsJSON["resolution"]["width"];
    this->data->resolution.height = settingsJSON["resolution"]["height"];

    this->data->master_volume.set(settingsJSON["master_volume"]);
    this->data->music_volume.set(settingsJSON["music_volume"]);
    this->data->sfx_volume.set(settingsJSON["sfx_volume"]);

    this->data->fullscreen_enabled = settingsJSON["fullscreen_enabled"];

    refresh_settings();
    return true;

  } catch (const std::exception &e) {
    log_error("Settings::load_save_file: {} formatted improperly. {}",
              data->loaded_from, e.what());
    return false;
  }
  return false;
}
