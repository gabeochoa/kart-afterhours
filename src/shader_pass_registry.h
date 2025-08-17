#pragma once

#include "shader_types.h"
#include "components.h"
#include <afterhours/src/singleton.h>
#include <vector>
#include <unordered_map>
#include <algorithm>

SINGLETON_FWD(ShaderPassRegistry)
struct ShaderPassRegistry {
  SINGLETON(ShaderPassRegistry)

  // Pass configuration for different render priorities
  struct RenderPass {
    RenderPriority priority;
    std::string name;
    std::vector<ShaderType> required_shaders;
    bool enabled = true;
    
    RenderPass(RenderPriority p, const std::string& n, std::vector<ShaderType> shaders = {})
      : priority(p), name(n), required_shaders(std::move(shaders)) {}
  };

  // Pre-defined render passes
  std::vector<RenderPass> render_passes = {
    {RenderPriority::Background, "Background"},
    {RenderPriority::Entities, "Entities", {ShaderType::Car, ShaderType::CarWinner, ShaderType::EntityEnhanced, ShaderType::EntityTest}},
    {RenderPriority::Particles, "Particles"},
    {RenderPriority::UI, "UI"},
    {RenderPriority::PostProcess, "PostProcess", {ShaderType::PostProcessing, ShaderType::PostProcessingTag}},
    {RenderPriority::Debug, "Debug"}
  };

  // Get all entities for a specific render pass
  template<typename EntityContainer>
  std::vector<Entity*> get_entities_for_pass(EntityContainer& entities, RenderPriority priority) const {
    std::vector<Entity*> result;
    
    for (auto& entity : entities) {
      if (entity.has<HasShader>()) {
        const auto& shader_comp = entity.get<HasShader>();
        if (shader_comp.render_priority == priority && shader_comp.enabled) {
          result.push_back(&entity);
        }
      }
    }
    
    // Sort by priority (lower numbers first)
    std::sort(result.begin(), result.end(), [](const Entity* a, const Entity* b) {
      const auto& shader_a = a->get<HasShader>();
      const auto& shader_b = b->get<HasShader>();
      return static_cast<int>(shader_a.render_priority) < static_cast<int>(shader_b.render_priority);
    });
    
    return result;
  }

  // Get all render passes in priority order
  const std::vector<RenderPass>& get_render_passes() const {
    return render_passes;
  }

  // Enable/disable specific render passes
  void enable_pass(RenderPriority priority) {
    for (auto& pass : render_passes) {
      if (pass.priority == priority) {
        pass.enabled = true;
        break;
      }
    }
  }

  void disable_pass(RenderPriority priority) {
    for (auto& pass : render_passes) {
      if (pass.priority == priority) {
        pass.enabled = false;
        break;
      }
    }
  }

  // Check if a pass is enabled
  bool is_pass_enabled(RenderPriority priority) const {
    for (const auto& pass : render_passes) {
      if (pass.priority == priority) {
        return pass.enabled;
      }
    }
    return false;
  }

  // Get pass info by priority
  const RenderPass* get_pass(RenderPriority priority) const {
    for (const auto& pass : render_passes) {
      if (pass.priority == priority) {
        return &pass;
      }
    }
    return nullptr;
  }

  // Add custom render pass
  void add_custom_pass(RenderPriority priority, const std::string& name, std::vector<ShaderType> shaders = {}) {
    render_passes.emplace_back(priority, name, std::move(shaders));
    
    // Re-sort by priority
    std::sort(render_passes.begin(), render_passes.end(), [](const RenderPass& a, const RenderPass& b) {
      return static_cast<int>(a.priority) < static_cast<int>(b.priority);
    });
  }

  // Remove custom render pass
  void remove_pass(RenderPriority priority) {
    render_passes.erase(
      std::remove_if(render_passes.begin(), render_passes.end(), 
        [priority](const RenderPass& pass) { return pass.priority == priority; }),
      render_passes.end()
    );
  }

  // Get debug info
  std::string get_debug_info() const {
    std::string result = "Render Passes:\n";
    for (const auto& pass : render_passes) {
      result += "  " + pass.name + " (Priority: " + std::to_string(static_cast<int>(pass.priority)) + 
                ", Enabled: " + (pass.enabled ? "true" : "false") + ")\n";
    }
    return result;
  }
};
