#pragma once

#include <afterhours/src/library.h>
#include <afterhours/src/singleton.h>

#include "rl.h"

SINGLETON_FWD(TextureLibrary)
struct TextureLibrary {
  SINGLETON(TextureLibrary)

  [[nodiscard]] const raylib::Texture2D &get(const std::string &name) const {
    return impl.get(name);
  }

  [[nodiscard]] raylib::Texture &get(const std::string &name) {
    return impl.get(name);
  }
  void load(const char *const filename, const char *const name) {
    impl.load(filename, name);
  }

  void unload_all() { impl.unload_all(); }

private:
  struct TextureLibraryImpl : Library<raylib::Texture2D> {
    virtual raylib::Texture2D
    convert_filename_to_object(const char *, const char *filename) override {
      return raylib::LoadTexture(filename);
    }

    virtual void unload(raylib::Texture shader) override {
      (void)shader; // Suppress unused parameter warning
    }
  } impl;
};
