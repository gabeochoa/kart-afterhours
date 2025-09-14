#pragma once

#include "shader_types.h"
#include <afterhours/src/library.h>
#include <afterhours/src/singleton.h>

#include "rl.h"
#include <string>
#include <unordered_map>

SINGLETON_FWD(ShaderLibrary)
struct ShaderLibrary {
  SINGLETON(ShaderLibrary)

  // Store shaders by enum type for fast lookup
  std::unordered_map<ShaderType, raylib::Shader> shaders_by_type;

  // Cache uniform locations for each shader
  std::unordered_map<ShaderType, std::unordered_map<UniformLocation, int>>
      uniform_locations;

  // Load all shaders at startup using magic_enum
  void load_all_shaders() {
    // Use magic_enum to automatically load all shader types
    constexpr auto shader_types = magic_enum::enum_values<ShaderType>();
    for (auto shader_type : shader_types) {
      load_shader(shader_type);
    }
  }

  // Get shader by enum type (fast lookup)
  [[nodiscard]] const raylib::Shader &get(ShaderType type) const {
    auto it = shaders_by_type.find(type);
    if (it == shaders_by_type.end()) {
      log_error("Shader not found for type: {}", static_cast<int>(type));
      // Return a default shader or throw
      static raylib::Shader default_shader = raylib::LoadShader("", "");
      return default_shader;
    }
    return it->second;
  }

  // Get uniform location (cached)
  [[nodiscard]] int get_uniform_location(ShaderType shader_type,
                                         UniformLocation uniform) const {
    auto shader_it = uniform_locations.find(shader_type);
    if (shader_it == uniform_locations.end())
      return -1;

    auto uniform_it = shader_it->second.find(uniform);
    if (uniform_it == shader_it->second.end())
      return -1;

    return uniform_it->second;
  }

  // Check if shader exists
  [[nodiscard]] bool contains(ShaderType type) const {
    return shaders_by_type.find(type) != shaders_by_type.end();
  }

  void load(const char *const /* filename */, const char *const name) {
    // Convert string name to enum type and load using new system
    ShaderType type = ShaderUtils::from_string(name);
    load_shader(type);
  }

  void unload_all() {
    shaders_by_type.clear();
    uniform_locations.clear();
  }

private:
  void load_shader(ShaderType type) {
    // Use magic_enum to automatically convert enum name to filename
    std::string enum_name = std::string(magic_enum::enum_name(type));
    std::string frag_filename = enum_name + ".fs";

    std::string vert_path = "resources/shaders/base.vs";
    std::string frag_path = "resources/shaders/" + frag_filename;

    raylib::Shader shader =
        raylib::LoadShader(vert_path.c_str(), frag_path.c_str());
    shaders_by_type[type] = shader;

    // Cache uniform locations for this shader
    cache_uniform_locations(type, shader);
  }

  void cache_uniform_locations(ShaderType type, const raylib::Shader &shader) {
    auto &locations = uniform_locations[type];

    // Cache all possible uniform locations using pre-defined names
    locations[UniformLocation::Time] =
        raylib::GetShaderLocation(shader, UniformNames::TIME);
    locations[UniformLocation::Resolution] =
        raylib::GetShaderLocation(shader, UniformNames::RESOLUTION);
    locations[UniformLocation::EntityColor] =
        raylib::GetShaderLocation(shader, UniformNames::ENTITY_COLOR);
    locations[UniformLocation::Speed] =
        raylib::GetShaderLocation(shader, UniformNames::SPEED);
    locations[UniformLocation::WinnerRainbow] =
        raylib::GetShaderLocation(shader, UniformNames::WINNER_RAINBOW);
    locations[UniformLocation::SpotlightEnabled] =
        raylib::GetShaderLocation(shader, UniformNames::SPOTLIGHT_ENABLED);
    locations[UniformLocation::SpotlightPos] =
        raylib::GetShaderLocation(shader, UniformNames::SPOTLIGHT_POS);
    locations[UniformLocation::SpotlightRadius] =
        raylib::GetShaderLocation(shader, UniformNames::SPOTLIGHT_RADIUS);
    locations[UniformLocation::SpotlightSoftness] =
        raylib::GetShaderLocation(shader, UniformNames::SPOTLIGHT_SOFTNESS);
    locations[UniformLocation::DimAmount] =
        raylib::GetShaderLocation(shader, UniformNames::DIM_AMOUNT);
    locations[UniformLocation::DesaturateAmount] =
        raylib::GetShaderLocation(shader, UniformNames::DESATURATE_AMOUNT);
    locations[UniformLocation::UvMin] =
        raylib::GetShaderLocation(shader, UniformNames::UV_MIN);
    locations[UniformLocation::UvMax] =
        raylib::GetShaderLocation(shader, UniformNames::UV_MAX);
  }
};

struct UpdateShaderValues : System<> {
  virtual void once(float) override {
    // Update shader values for all active shaders
    // This system runs once per frame to update global shader uniforms

    auto *resolution_provider = EntityHelper::get_singleton_cmp<
        afterhours::window_manager::ProvidesCurrentResolution>();
    if (!resolution_provider) {
      return;
    }

    const auto &resolution = resolution_provider->current_resolution;
    const float current_time = static_cast<float>(raylib::GetTime());

    // Update all shaders with current time and resolution
    constexpr auto shader_types = magic_enum::enum_values<ShaderType>();
    for (auto shader_type : shader_types) {
      if (!ShaderLibrary::get().contains(shader_type)) {
        continue;
      }

      const auto &shader = ShaderLibrary::get().get(shader_type);

      // Update time uniform if available
      int time_loc = raylib::GetShaderLocation(shader, "time");
      if (time_loc != -1) {
        raylib::SetShaderValue(shader, time_loc, &current_time,
                               raylib::SHADER_UNIFORM_FLOAT);
      }

      // Update resolution uniform if available
      int res_loc = raylib::GetShaderLocation(shader, "resolution");
      if (res_loc != -1) {
        vec2 res_vec = {static_cast<float>(resolution.width),
                        static_cast<float>(resolution.height)};
        raylib::SetShaderValue(shader, res_loc, &res_vec,
                               raylib::SHADER_UNIFORM_VEC2);
      }
    }
  }
};
