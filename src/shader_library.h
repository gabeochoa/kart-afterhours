#pragma once

#include "library.h"
#include <afterhours/src/singleton.h>
#include "shader_types.h"

#include "rl.h"
#include <string>
#include <unordered_map>

SINGLETON_FWD(ShaderLibrary)
struct ShaderLibrary {
  SINGLETON(ShaderLibrary)

  // Store shaders by enum type for fast lookup
  std::unordered_map<ShaderType, raylib::Shader> shaders_by_type;
  
  // Cache uniform locations for each shader
  std::unordered_map<ShaderType, std::unordered_map<UniformLocation, int>> uniform_locations;

  // Load all shaders at startup using magic_enum
  void load_all_shaders() {
    // Use magic_enum to automatically load all shader types
    constexpr auto shader_types = magic_enum::enum_values<ShaderType>();
    for (auto shader_type : shader_types) {
      load_shader(shader_type);
    }
  }

  // Get shader by enum type (fast lookup)
  [[nodiscard]] const raylib::Shader& get(ShaderType type) const {
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
  [[nodiscard]] int get_uniform_location(ShaderType shader_type, UniformLocation uniform) const {
    auto shader_it = uniform_locations.find(shader_type);
    if (shader_it == uniform_locations.end()) return -1;
    
    auto uniform_it = shader_it->second.find(uniform);
    if (uniform_it == shader_it->second.end()) return -1;
    
    return uniform_it->second;
  }

  // Check if shader exists
  [[nodiscard]] bool contains(ShaderType type) const {
    return shaders_by_type.find(type) != shaders_by_type.end();
  }

  // Backward compatibility
  [[nodiscard]] const raylib::Shader &get(const std::string &name) const {
    ShaderType type = ShaderUtils::from_string(name);
    return get(type);
  }

  [[nodiscard]] raylib::Shader &get(const std::string &name) {
    ShaderType type = ShaderUtils::from_string(name);
    return const_cast<raylib::Shader&>(get(type));
  }

  [[nodiscard]] bool contains(const std::string &name) const {
    ShaderType type = ShaderUtils::from_string(name);
    return contains(type);
  }

  void load(const char *const filename, const char *const name) {
    // Convert string name to enum type and load using new system
    ShaderType type = ShaderUtils::from_string(name);
    load_shader(type);
  }

  void unload_all() { 
    shaders_by_type.clear();
    uniform_locations.clear();
    impl.unload_all(); 
  }

  void update_values(const vec2 *rez, const float *time) {
    (void)rez;
    (void)time;
    // for (auto &kv : impl.storage) {
    // auto &shader = kv.second;
    // TODO this doesnt seem to work on my personal laptop?
    // int time_loc = raylib::GetShaderLocation(shader, "time");
    // int resolution_loc = raylib::GetShaderLocation(shader, "resolution");
    // raylib::SetShaderValue(shader, resolution_loc, rez,
    // raylib::SHADER_UNIFORM_FLOAT);
    // raylib::SetShaderValue(shader, time_loc, time,
    // raylib::SHADER_UNIFORM_FLOAT);
    // }
  }

private:
  void load_shader(ShaderType type) {
    // Map enum values to actual shader filenames
    std::string frag_filename;
    switch (type) {
      case ShaderType::Car: frag_filename = "car.fs"; break;
      case ShaderType::CarWinner: frag_filename = "car_winner.fs"; break;
      case ShaderType::EntityEnhanced: frag_filename = "entity_enhanced.fs"; break;
      case ShaderType::EntityTest: frag_filename = "entity_test.fs"; break;
      case ShaderType::PostProcessing: frag_filename = "post_processing.fs"; break;
      case ShaderType::PostProcessingTag: frag_filename = "post_processing_tag.fs"; break;
      case ShaderType::TextMask: frag_filename = "text_mask.fs"; break;
      default: 
        log_warn("Unknown shader type: {}", magic_enum::enum_name(type));
        return;
    }
    
    std::string vert_path = "resources/shaders/base.vs";
    std::string frag_path = "resources/shaders/" + frag_filename;
    
    raylib::Shader shader = raylib::LoadShader(vert_path.c_str(), frag_path.c_str());
    shaders_by_type[type] = shader;
    
    // Cache uniform locations for this shader
    cache_uniform_locations(type, shader);
  }
  
  void cache_uniform_locations(ShaderType type, const raylib::Shader& shader) {
    auto& locations = uniform_locations[type];
    
    // Cache all possible uniform locations using pre-defined names
    locations[UniformLocation::Time] = raylib::GetShaderLocation(shader, UniformNames::TIME);
    locations[UniformLocation::Resolution] = raylib::GetShaderLocation(shader, UniformNames::RESOLUTION);
    locations[UniformLocation::EntityColor] = raylib::GetShaderLocation(shader, UniformNames::ENTITY_COLOR);
    locations[UniformLocation::Speed] = raylib::GetShaderLocation(shader, UniformNames::SPEED);
    locations[UniformLocation::WinnerRainbow] = raylib::GetShaderLocation(shader, UniformNames::WINNER_RAINBOW);
    locations[UniformLocation::SpotlightEnabled] = raylib::GetShaderLocation(shader, UniformNames::SPOTLIGHT_ENABLED);
    locations[UniformLocation::SpotlightPos] = raylib::GetShaderLocation(shader, UniformNames::SPOTLIGHT_POS);
    locations[UniformLocation::SpotlightRadius] = raylib::GetShaderLocation(shader, UniformNames::SPOTLIGHT_RADIUS);
    locations[UniformLocation::SpotlightSoftness] = raylib::GetShaderLocation(shader, UniformNames::SPOTLIGHT_SOFTNESS);
    locations[UniformLocation::DimAmount] = raylib::GetShaderLocation(shader, UniformNames::DIM_AMOUNT);
    locations[UniformLocation::DesaturateAmount] = raylib::GetShaderLocation(shader, UniformNames::DESATURATE_AMOUNT);
    locations[UniformLocation::UvMin] = raylib::GetShaderLocation(shader, UniformNames::UV_MIN);
    locations[UniformLocation::UvMax] = raylib::GetShaderLocation(shader, UniformNames::UV_MAX);
  }

  struct ShaderLibraryImpl : Library<raylib::Shader> {
    virtual raylib::Shader
    convert_filename_to_object(const char *, const char *filename) override {
      // Use a generic vertex shader that contains all attributes Raylib
      // expects.
      std::string fragPath(filename);
      // Determine directory of fragment shader file.
      auto pos = fragPath.find_last_of('/');
      std::string dir = (pos == std::string::npos) ? std::string(".")
                                                   : fragPath.substr(0, pos);
      std::string vertPath = dir + "/base.vs";
      return raylib::LoadShader(vertPath.c_str(), filename);
    }

    virtual void unload(raylib::Shader) override {}
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
    float time = static_cast<float>(raylib::GetTime());
    ShaderLibrary::get().update_values(&rez, &time);
  }
};

struct BeginShader : System<> {
  raylib::Shader shader;
  BeginShader(const std::string &shader_name)
      : shader(ShaderLibrary::get().get(shader_name)) {}

  virtual void once(float dt) {
    (void)dt; // Suppress unused parameter warning
    raylib::BeginShaderMode(shader);
  }
};
