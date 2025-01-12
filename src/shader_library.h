#pragma once

#include "afterhours/src/singleton.h"
#include "library.h"

#include "rl.h"

SINGLETON_FWD(ShaderLibrary)
struct ShaderLibrary {
  SINGLETON(ShaderLibrary)

  [[nodiscard]] const raylib::Shader &get(const std::string &name) const {
    return impl.get(name);
  }

  [[nodiscard]] raylib::Shader &get(const std::string &name) {
    return impl.get(name);
  }
  void load(const char *filename, const char *name) {
    impl.load(filename, name);
  }

  void unload_all() { impl.unload_all(); }

  void update_values(vec2 *rez, float *time) {
    for (auto &kv : impl.storage) {
      auto &shader = kv.second;
      int time_loc = raylib::GetShaderLocation(shader, "time");
      int resolution_loc = raylib::GetShaderLocation(shader, "resolution");
      raylib::SetShaderValue(shader, resolution_loc, rez,
                             raylib::SHADER_UNIFORM_VEC2);
      raylib::SetShaderValue(shader, time_loc, time,
                             raylib::SHADER_UNIFORM_FLOAT);
    }
  }

private:
  struct ShaderLibraryImpl : Library<raylib::Shader> {
    virtual raylib::Shader
    convert_filename_to_object(const char *, const char *filename) override {
      return raylib::LoadShader(0, filename);
    }

    virtual void unload(raylib::Shader shader) override {}
  } impl;
};

using namespace afterhours;

struct UpdateShaderValues : System<> {
  virtual void once(float) {
    auto resolution = EntityHelper::get_singleton_cmp<
                          window_manager::ProvidesCurrentResolution>()
                          ->current_resolution;
    vec2 rez = {
        static_cast<float>(resolution.width),
        static_cast<float>(resolution.height),
    };
    float time = raylib::GetTime();
    ShaderLibrary::get().update_values(&rez, &time);
  }
};

struct BeginShader : System<> {
  raylib::Shader shader;
  BeginShader(const std::string &shader_name)
      : shader(ShaderLibrary::get().get(shader_name)) {}

  virtual void once(float dt) { raylib::BeginShaderMode(shader); }
};
