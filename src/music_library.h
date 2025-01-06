#pragma once

#include "afterhours/src/singleton.h"
#include "library.h"

#include "rl.h"

SINGLETON_FWD(MusicLibrary)
struct MusicLibrary {
  SINGLETON(MusicLibrary)

  [[nodiscard]] const raylib::Music &get(const std::string &name) const {
    return impl.get(name);
  }

  [[nodiscard]] raylib::Music &get(const std::string &name) {
    return impl.get(name);
  }
  void load(const char *filename, const char *name) {
    impl.load(filename, name);

    // TODO SPEED: right now we loop over every sound again
    // could just look up the newly added one
    update_volume(current_volume);
  }

  void update_volume(float new_v) {
    impl.update_volume(new_v);
    current_volume = new_v;
  }

  void unload_all() { impl.unload_all(); }

private:
  // This note is also referenced in sound library
  //
  // Note: this is set from settings
  // we store this because when a new music is added it uses the default
  // volume instead of the one in settings because we are reactive. this
  // allows us to locally cache the most recent volume and handle it here
  float current_volume = 1.f;

  struct MusicLibraryImpl : Library<raylib::Music> {
    virtual raylib::Music
    convert_filename_to_object(const char *, const char *filename) override {
      return raylib::LoadMusicStream(filename);
    }

    void update_volume(float new_v) {
      for (const auto &kv : storage) {
        log_trace("updating music volume for {} to {}", kv.first, new_v);
        raylib::SetMusicVolume(kv.second, new_v);
      }
    }

    virtual void unload(raylib::Music music) override {
      raylib::UnloadMusicStream(music);
    }
  } impl;
};
