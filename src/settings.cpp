
#include "settings.h"

#include <algorithm>
#include <memory>

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

  Pct master_volume = 0.5f;
  Pct music_volume = 0.5f;
  Pct sfx_volume = 0.5f;

  bool fullscreen_enabled = false;
};

Settings::Settings() { data = new S_Data(); }
Settings::~Settings() { delete data; }

void Settings::reset() {
  delete data;
  data = new S_Data();
  refresh_settings();
}

void Settings::update_music_volume(float vol) {
  MusicLibrary::get().update_volume(vol);
}
void Settings::update_sfx_volume(float vol) {
  SoundLibrary::get().update_volume(vol);
}

void Settings::update_master_volume(float vol) {
  // TODO should these be updated after?
  // because if not, then why are we doing these at all
  update_music_volume(data->music_volume);
  update_sfx_volume(data->sfx_volume);

  raylib::SetMasterVolume(vol);
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
