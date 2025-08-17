#pragma once

#include "components.h"
#include "shader_library.h"
#include "shader_pass_registry.h"
#include "shader_types.h"
#include <afterhours/src/singleton.h>
#include <afterhours/src/system.h>
#include <memory>
#include <vector>

SINGLETON_FWD(MultipassRenderer)
struct MultipassRenderer {
  SINGLETON(MultipassRenderer)

  // Render pass configuration
  struct PassConfig {
    RenderPriority priority;
    bool clear_before = false;
    bool clear_after = false;
    raylib::Color clear_color = raylib::BLACK;

    PassConfig(RenderPriority p, bool clear_before = false,
               bool clear_after = false,
               raylib::Color clear_color = raylib::BLACK)
        : priority(p), clear_before(clear_before), clear_after(clear_after),
          clear_color(clear_color) {}
  };

  // Pre-configured render passes
  std::vector<PassConfig> pass_configs = {
      {RenderPriority::Background, true, false,
       raylib::SKYBLUE},                        // Clear to sky color
      {RenderPriority::Entities, false, false}, // Don't clear, render entities
      {RenderPriority::Particles, false,
       false},                            // Don't clear, render particles
      {RenderPriority::UI, false, false}, // Don't clear, render UI
      {RenderPriority::PostProcess, false,
       false}, // Don't clear, post-processing
      {RenderPriority::Debug, false, true, raylib::BLACK} // Clear after debug
  };

  // Render all passes in priority order
  template <typename EntityContainer>
  void render_all_passes(EntityContainer &entities) {
    auto &registry = ShaderPassRegistry::get();
    auto &shader_lib = ShaderLibrary::get();

    for (const auto &pass_config : pass_configs) {
      if (!registry.is_pass_enabled(pass_config.priority)) {
        continue;
      }

      // Clear before pass if configured
      if (pass_config.clear_before) {
        raylib::ClearBackground(pass_config.clear_color);
      }

      // Render this pass
      render_pass(entities, pass_config.priority);

      // Clear after pass if configured
      if (pass_config.clear_after) {
        raylib::ClearBackground(pass_config.clear_color);
      }
    }
  }

  // Render a specific pass
  template <typename EntityContainer>
  void render_pass(EntityContainer &entities, RenderPriority priority) {
    auto &registry = ShaderPassRegistry::get();
    auto &shader_lib = ShaderLibrary::get();

    // Get entities for this pass
    auto pass_entities = registry.get_entities_for_pass(entities, priority);

    // Render each entity in this pass
    for (Entity *entity : pass_entities) {
      if (!entity->has<HasShader>())
        continue;

      const auto &shader_comp = entity->get<HasShader>();
      if (!shader_comp.enabled)
        continue;

      // Render entity with its shaders
      render_entity(*entity, shader_comp);
    }
  }

  // Render a single entity with its shaders
  void render_entity(const Entity &entity, const HasShader &shader_comp) {
    auto &shader_lib = ShaderLibrary::get();

    // Apply each shader in order
    for (const auto &shader_type : shader_comp.shaders) {
      if (!shader_lib.contains(shader_type)) {
        log_warn("Shader not found for type: {}",
                 static_cast<int>(shader_type));
        continue;
      }

      const auto &shader = shader_lib.get(shader_type);

      // Begin shader mode
      raylib::BeginShaderMode(shader);

      // Set common uniforms
      set_common_uniforms(shader_type, shader);

      // Render the entity (this will be handled by existing rendering systems)
      // The shader is now active and will be used for subsequent rendering
      // calls

      // End shader mode
      raylib::EndShaderMode();
    }
  }

  // Set common uniforms for a shader
  void set_common_uniforms(ShaderType shader_type,
                           const raylib::Shader &shader) {
    auto &shader_lib = ShaderLibrary::get();

    // Set time uniform if available
    int time_loc =
        shader_lib.get_uniform_location(shader_type, UniformLocation::Time);
    if (time_loc != -1) {
      float time = static_cast<float>(raylib::GetTime());
      raylib::SetShaderValue(shader, time_loc, &time,
                             raylib::SHADER_UNIFORM_FLOAT);
    }

    // Set resolution uniform if available
    int resolution_loc = shader_lib.get_uniform_location(
        shader_type, UniformLocation::Resolution);
    if (resolution_loc != -1) {
      raylib::Vector2 resolution = {
          static_cast<float>(raylib::GetScreenWidth()),
          static_cast<float>(raylib::GetScreenHeight())};
      raylib::SetShaderValue(shader, resolution_loc, &resolution,
                             raylib::SHADER_UNIFORM_VEC2);
    }
  }

  // Set entity-specific uniforms
  void set_entity_uniforms(ShaderType shader_type, const raylib::Shader &shader,
                           const raylib::Color &color, float speed = 0.0f,
                           bool is_winner = false) {
    auto &shader_lib = ShaderLibrary::get();

    // Set entity color uniform if available
    int color_loc = shader_lib.get_uniform_location(
        shader_type, UniformLocation::EntityColor);
    if (color_loc != -1) {
      raylib::Vector4 color_vec = {color.r / 255.0f, color.g / 255.0f,
                                   color.b / 255.0f, color.a / 255.0f};
      raylib::SetShaderValue(shader, color_loc, &color_vec,
                             raylib::SHADER_UNIFORM_VEC4);
    }

    // Set speed uniform if available
    int speed_loc =
        shader_lib.get_uniform_location(shader_type, UniformLocation::Speed);
    if (speed_loc != -1) {
      raylib::SetShaderValue(shader, speed_loc, &speed,
                             raylib::SHADER_UNIFORM_FLOAT);
    }

    // Set winner rainbow uniform if available
    int rainbow_loc = shader_lib.get_uniform_location(
        shader_type, UniformLocation::WinnerRainbow);
    if (rainbow_loc != -1) {
      float rainbow_value = is_winner ? 1.0f : 0.0f;
      raylib::SetShaderValue(shader, rainbow_loc, &rainbow_value,
                             raylib::SHADER_UNIFORM_FLOAT);
    }
  }

  // Configure a render pass
  void configure_pass(RenderPriority priority, bool clear_before,
                      bool clear_after,
                      raylib::Color clear_color = raylib::BLACK) {
    for (auto &config : pass_configs) {
      if (config.priority == priority) {
        config.clear_before = clear_before;
        config.clear_after = clear_after;
        config.clear_color = clear_color;
        break;
      }
    }
  }

  // Get pass configuration
  const PassConfig *get_pass_config(RenderPriority priority) const {
    for (const auto &config : pass_configs) {
      if (config.priority == priority) {
        return &config;
      }
    }
    return nullptr;
  }

  // Enable/disable specific render passes
  void enable_pass(RenderPriority priority) {
    ShaderPassRegistry::get().enable_pass(priority);
  }

  void disable_pass(RenderPriority priority) {
    ShaderPassRegistry::get().disable_pass(priority);
  }

  // Get debug info
  std::string get_debug_info() const {
    std::string result = "Multipass Renderer:\n";
    result += ShaderPassRegistry::get().get_debug_info();
    result += "\nPass Configurations:\n";

    for (const auto &config : pass_configs) {
      result += "  " + std::to_string(static_cast<int>(config.priority)) +
                " - Clear Before: " + (config.clear_before ? "true" : "false") +
                ", Clear After: " + (config.clear_after ? "true" : "false") +
                "\n";
    }

    return result;
  }
};
