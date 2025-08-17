#pragma once

#include "components.h"
#include "multipass_renderer.h"
#include "shader_pass_registry.h"
#include <afterhours/src/entity_helper.h>
#include <afterhours/src/system.h>

// Example system that demonstrates multipass rendering integration
struct MultipassRenderingSystem : System<> {
  virtual void once(float dt) override {
    auto &renderer = MultipassRenderer::get();
    auto &registry = ShaderPassRegistry::get();

    // Example: Configure render passes based on game state
    configure_render_passes();

    // Example: Enable/disable passes based on game conditions
    if (should_show_debug_info()) {
      registry.enable_pass(RenderPriority::Debug);
    } else {
      registry.disable_pass(RenderPriority::Debug);
    }

    // Example: Get debug info
    if (should_show_debug_info()) {
      std::string debug_info = renderer.get_debug_info();
      // You could display this in a debug UI
      log_info("Multipass Debug Info:\n{}", debug_info);
    }
  }

private:
  void configure_render_passes() {
    auto &renderer = MultipassRenderer::get();

    // Configure background pass to clear with sky color
    renderer.configure_pass(RenderPriority::Background, true, false,
                            raylib::SKYBLUE);

    // Configure entities pass to not clear (render on top of background)
    renderer.configure_pass(RenderPriority::Entities, false, false);

    // Configure post-processing pass to not clear (apply effects to existing
    // render)
    renderer.configure_pass(RenderPriority::PostProcess, false, false);

    // Configure debug pass to clear after rendering
    renderer.configure_pass(RenderPriority::Debug, false, true, raylib::BLACK);
  }

  bool should_show_debug_info() const {
    // Example: Show debug info when F3 is pressed or in debug mode
    // This is just a placeholder - implement based on your input system
    return false; // Change this based on your debug key or game state
  }
};

// Example system that demonstrates how to update existing entities to use
// multipass
struct MultipassEntitySetupSystem : System<> {
  virtual void once(float dt) override {
    // Example: Set up entities with proper render priorities
    setup_entity_render_priorities();
  }

private:
  void setup_entity_render_priorities() {
    // Example: Find all entities with HasShader and set appropriate priorities
    auto entities = EntityHelper::get_all_entities();

    for (auto &entity : entities) {
      if (entity.has<HasShader>()) {
        auto &shader_comp = entity.get<HasShader>();

        // Example: Set render priority based on entity type
        if (entity.has<Transform>()) {
          // This is a game entity (car, item, etc.)
          shader_comp.render_priority = RenderPriority::Entities;
        } else if (entity.has<ui::UIComponent>()) {
          // This is a UI element
          shader_comp.render_priority = RenderPriority::UI;
        }

        // Example: Enable/disable shaders based on entity state
        if (entity.has<Transform>()) {
          // Enable entity shaders
          shader_comp.enabled = true;
        }
      }
    }
  }
};

// Example system that demonstrates how to use the new uniform system
struct MultipassUniformSystem : System<> {
  virtual void for_each_with(Entity &entity, HasShader &shader_comp,
                             float dt) override {
    auto &renderer = MultipassRenderer::get();

    // Example: Update entity uniforms based on game state
    for (const auto &shader_type : shader_comp.shaders) {
      if (shader_type == ShaderType::Car ||
          shader_type == ShaderType::CarWinner) {
        // This is a car shader, set car-specific uniforms
        update_car_uniforms(entity, shader_type);
      }
    }
  }

private:
  void update_car_uniforms(Entity &entity, ShaderType shader_type) {
    // Example: Set car-specific uniforms
    // In a real implementation, you'd get these values from car components

    raylib::Color car_color = raylib::RED; // Get from car component
    float car_speed = 0.0f;                // Get from car component
    bool is_winner = false;                // Get from game state

    // Note: This is just an example - the actual uniform setting happens in the
    // renderer This system would prepare the data that the renderer uses
  }
};

// Example of how to integrate with existing rendering systems
struct MultipassRenderingIntegration : System<> {
  virtual void once(float dt) override {
    // Example: This system could be called from your main rendering loop
    // to integrate multipass rendering with existing systems

    // Get all entities that need to be rendered
    auto entities = EntityHelper::get_all_entities();

    // Use the multipass renderer to render everything
    auto &renderer = MultipassRenderer::get();
    renderer.render_all_passes(entities);
  }
};
