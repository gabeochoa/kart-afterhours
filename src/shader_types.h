#pragma once

#include "rl.h"
#include <string>
#include <string_view>

// Render priority system to replace magic numbers
enum class RenderPriority {
  Background = 0,    // Sky, terrain, map background
  Entities = 100,    // Cars, items, game objects
  Particles = 200,   // Particle effects
  UI = 300,          // HUD, menus, interface
  PostProcess = 400, // Final effects, bloom, etc.
  Debug = 500        // Debug overlays, profiling
};

// Define all available shaders as an enum
enum class ShaderType {
  // Entity shaders
  car,
  car_winner,
  entity_enhanced,
  entity_test,

  // Post-processing shaders
  post_processing,
  post_processing_tag,

  // Special effects
  text_mask,

  // Add new shaders here as needed
};

// Define all uniform locations as an enum
enum class UniformLocation {
  // Common uniforms (used by most shaders)
  Time,        // Used by: Car, CarWinner, PostProcessing, PostProcessingTag
  Resolution,  // Used by: Car, CarWinner, PostProcessing, PostProcessingTag
  EntityColor, // Used by: Car, CarWinner, EntityEnhanced, EntityTest

  // Car-specific uniforms
  Speed,         // Used by: Car
  WinnerRainbow, // Used by: CarWinner

  // Post-processing uniforms
  SpotlightEnabled,  // Used by: PostProcessingTag
  SpotlightPos,      // Used by: PostProcessingTag
  SpotlightRadius,   // Used by: PostProcessingTag
  SpotlightSoftness, // Used by: PostProcessingTag
  DimAmount,         // Used by: PostProcessingTag
  DesaturateAmount,  // Used by: PostProcessingTag

  // UV bounds (used by sprite-based shaders)
  UvMin, // Used by: Car, CarWinner, EntityEnhanced, EntityTest
  UvMax, // Used by: Car, CarWinner, EntityEnhanced, EntityTest

  // Add new uniforms here as needed
};

// Pre-defined uniform names to avoid magic_enum overhead at load time
namespace UniformNames {
constexpr const char *TIME = "time";
constexpr const char *RESOLUTION = "resolution";
constexpr const char *ENTITY_COLOR = "entityColor";
constexpr const char *SPEED = "speed";
constexpr const char *WINNER_RAINBOW = "winnerRainbow";
constexpr const char *SPOTLIGHT_ENABLED = "spotlightEnabled";
constexpr const char *SPOTLIGHT_POS = "spotlightPos";
constexpr const char *SPOTLIGHT_RADIUS = "spotlightRadius";
constexpr const char *SPOTLIGHT_SOFTNESS = "spotlightSoftness";
constexpr const char *DIM_AMOUNT = "dimAmount";
constexpr const char *DESATURATE_AMOUNT = "desaturateAmount";
constexpr const char *UV_MIN = "uvMin";
constexpr const char *UV_MAX = "uvMax";
} // namespace UniformNames

// Helper functions for priority enums
namespace PriorityUtils {
constexpr int to_int(RenderPriority priority) {
  return static_cast<int>(priority);
}

constexpr bool is_entity(RenderPriority priority) {
  return priority == RenderPriority::Entities;
}

constexpr bool is_post_process(RenderPriority priority) {
  return priority == RenderPriority::PostProcess;
}
} // namespace PriorityUtils

// Helper functions for shader enums
namespace ShaderUtils {
// Convert string to enum (for backward compatibility)
inline ShaderType from_string(const std::string &name) {
  // Use magic_enum to convert string to enum
  auto result = magic_enum::enum_cast<ShaderType>(name);
  if (result.has_value()) {
    return result.value();
  }

  // Fallback to car if unknown
  return ShaderType::car;
}

// Convert enum to string for debugging
inline constexpr std::string_view to_string(ShaderType shader) {
  return magic_enum::enum_name(shader);
}
} // namespace ShaderUtils

// Helper functions for uniform enums
namespace UniformUtils {
// Convert enum to string for shader lookup using magic_enum
inline constexpr std::string_view to_string(UniformLocation uniform) {
  return magic_enum::enum_name(uniform);
}
} // namespace UniformUtils
