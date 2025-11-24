#pragma once

#include "components.h"
#include "shader_types.h"
#include <afterhours/src/singleton.h>
#include <algorithm>
#include <unordered_map>
#include <vector>

using namespace afterhours;

SINGLETON_FWD(ShaderPassRegistry)
struct ShaderPassRegistry {
  SINGLETON(ShaderPassRegistry)

  // Pass configuration for different render priorities
  struct RenderPass {
    RenderPriority priority;
    std::vector<ShaderType> required_shaders;
    bool enabled = true;

    constexpr RenderPass(RenderPriority p, std::vector<ShaderType> shaders = {})
        : priority(p), required_shaders(std::move(shaders)) {}
  };

  // Pre-defined render passes
  constexpr static std::vector<RenderPass> render_passes = {
      {RenderPriority::Background},
      {RenderPriority::Entities,
       {
           ShaderType::car,
           ShaderType::car_winner,
           ShaderType::entity_enhanced,
           ShaderType::entity_test,
       }},
      {RenderPriority::Particles},
      {RenderPriority::UI},
      {RenderPriority::PostProcess,
       {
           ShaderType::post_processing,
           ShaderType::post_processing_tag,
       }},
      {RenderPriority::Debug},
  };

  // Get all entities for a specific render pass
  template <typename EntityContainer>
  RefEntities get_entities_for_pass(const EntityContainer &entities,
                                    RenderPriority priority) const {
    // Pre-allocate result vector to avoid multiple reallocations
    // Estimate size based on typical entity counts
    RefEntities result;
    result.reserve(entities.size() / 4); // Assume ~25% of entities have shaders

    // Filter entities by render priority and enabled state
    for (Entity &entity : entities) {
      if (entity.has<HasShader>()) {
        const HasShader &shader_comp = entity.get<HasShader>();
        if (shader_comp.render_priority == priority && shader_comp.enabled) {
          result.push_back(entity);
        }
      }
    }

    // Sort entities by their shader priority to ensure consistent rendering
    // order Lower priority numbers are rendered first (background before
    // entities, etc.)
    std::sort(result.begin(), result.end(),
              [](const RefEntity a, const RefEntity b) {
                const HasShader &shader_a = a.get<HasShader>();
                const HasShader &shader_b = b.get<HasShader>();
                return static_cast<int>(shader_a.render_priority) <
                       static_cast<int>(shader_b.render_priority);
              });

    return result;
  }

  // Get all render passes in priority order
  constexpr const std::vector<RenderPass> &get_render_passes() const {
    return render_passes;
  }

  // Enable/disable specific render passes
  void enable_pass(RenderPriority priority) {
    for (auto &pass : render_passes) {
      if (pass.priority == priority) {
        pass.enabled = true;
        break;
      }
    }
  }

  void disable_pass(RenderPriority priority) {
    for (auto &pass : render_passes) {
      if (pass.priority == priority) {
        pass.enabled = false;
        break;
      }
    }
  }

  // Check if a pass is enabled
  bool is_pass_enabled(RenderPriority priority) const {
    for (const auto &pass : render_passes) {
      if (pass.priority == priority) {
        return pass.enabled;
      }
    }
    return false;
  }

  // Get pass info by priority
  const RenderPass *get_pass(RenderPriority priority) const {
    for (const auto &pass : render_passes) {
      if (pass.priority == priority) {
        return &pass;
      }
    }
    return nullptr;
  }

  // Add custom render pass
  void add_custom_pass(RenderPriority priority,
                       std::vector<ShaderType> shaders = {}) {
    render_passes.emplace_back(priority, std::move(shaders));

    // Re-sort by priority
    std::sort(render_passes.begin(), render_passes.end(),
              [](const RenderPass &a, const RenderPass &b) {
                return static_cast<int>(a.priority) <
                       static_cast<int>(b.priority);
              });
  }

  // Remove custom render pass
  void remove_pass(RenderPriority priority) {
    render_passes.erase(std::remove_if(render_passes.begin(),
                                       render_passes.end(),
                                       [priority](const RenderPass &pass) {
                                         return pass.priority == priority;
                                       }),
                        render_passes.end());
  }

  // Get debug info
  std::string get_debug_info() const {
    std::string result = "Render Passes:\n";
    for (const auto &pass : render_passes) {
      result += "  Priority " +
                std::to_string(static_cast<int>(pass.priority)) +
                " (Enabled: " + (pass.enabled ? "true" : "false") + ")\n";
    }
    return result;
  }
};
