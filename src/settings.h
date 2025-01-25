
#pragma once

#include <memory>

#include "afterhours/src/plugins/window_manager.h"

#include "afterhours/src/singleton.h"
#include "library.h"

struct S_Data;

SINGLETON_FWD(Settings)
struct Settings {
  SINGLETON(Settings)

  S_Data *data;

  Settings();
  ~Settings();

  Settings(const Settings &) = delete;
  void operator=(const Settings &) = delete;

  bool load_save_file(int, int);
  void write_save_file();

  void reset();
  void refresh_settings();

  int get_screen_width() const;
  int get_screen_height() const;

  float get_music_volume() const;
  void update_music_volume(float);

  float get_sfx_volume() const;
  void update_sfx_volume(float);

  float get_master_volume() const;
  void update_master_volume(float);

  bool &get_fullscreen_enabled();
  void toggle_fullscreen();
};
