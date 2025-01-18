#pragma once

#include "afterhours/src/singleton.h"
#include "library.h"

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
  void load(const char *filename, const char *name) {
    impl.load(filename, name);
  }

  void unload_all() { impl.unload_all(); }

private:
  struct TextureLibraryImpl : Library<raylib::Texture2D> {
    virtual raylib::Texture2D
    convert_filename_to_object(const char *, const char *filename) override {
      return raylib::LoadTexture(filename);
    }

    virtual void unload(raylib::Texture shader) override {}
  } impl;
};
