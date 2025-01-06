#pragma once

#include "afterhours/src/singleton.h"
#include "library.h"
//
#include "resources.h"

#include "rl.h"

enum struct SoundFile {
  Rumble,
  Skrt_Start,
  Skrt_Mid,
  Skrt_End,
};

static const char *sound_file_to_str(SoundFile sf) {
  switch (sf) {
  case SoundFile::Rumble:
    return "Rumble";
  case SoundFile::Skrt_Start:
    return "SkrtStart";
  case SoundFile::Skrt_Mid:
    return "SkrtMid";
  case SoundFile::Skrt_End:
    return "SkrtEnd";
    break;
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
  void load(const char *filename, const char *name) {
    impl.load(filename, name);
  }

  void play(SoundFile file) { play(sound_file_to_str(file)); }
  void play(const char *name) { PlaySound(get(name)); }

  void play_random_match(const std::string &prefix) {
    impl.get_random_match(prefix).transform(raylib::PlaySound);
  }

  void update_volume(float new_v) {
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

    void update_volume(float new_v) {
      for (const auto &kv : storage) {
        log_trace("updating sound volume for {} to {}", kv.first, new_v);
        raylib::SetSoundVolume(kv.second, new_v);
      }
    }
  } impl;
};

static void load_sounds() {
  magic_enum::enum_for_each<SoundFile>([](auto val) {
    constexpr SoundFile file = val;
    std::string filename;
    switch (file) {
    case SoundFile::Rumble:
      filename = "rumble.wav";
      break;
    case SoundFile::Skrt_Start:
      filename = "skrt_start.wav";
      break;
    case SoundFile::Skrt_Mid:
      filename = "skrt_mid.wav";
      break;
    case SoundFile::Skrt_End:
      filename = "skrt_end.wav";
      break;
    }
    SoundLibrary::get().load(GetAssetPath(filename.c_str()),
                             sound_file_to_str(file));
  });
}
