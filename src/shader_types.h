#pragma once

#include <string>
#include <string_view>
#include "rl.h"

// Render priority system to replace magic numbers
enum class RenderPriority {
    Background = 0,      // Sky, terrain, map background
    Entities = 100,      // Cars, items, game objects
    Particles = 200,     // Particle effects
    UI = 300,           // HUD, menus, interface
    PostProcess = 400,   // Final effects, bloom, etc.
    Debug = 500          // Debug overlays, profiling
};

// Define all available shaders as an enum
enum class ShaderType {
    // Entity shaders
    Car,
    CarWinner,
    EntityEnhanced,
    EntityTest,
    
    // Post-processing shaders
    PostProcessing,
    PostProcessingTag,
    
    // Special effects
    TextMask,
    
    // Add new shaders here as needed
};

// Define all uniform locations as an enum
enum class UniformLocation {
    // Common uniforms (used by most shaders)
    Time,           // Used by: Car, CarWinner, PostProcessing, PostProcessingTag
    Resolution,     // Used by: Car, CarWinner, PostProcessing, PostProcessingTag
    EntityColor,    // Used by: Car, CarWinner, EntityEnhanced, EntityTest
    
    // Car-specific uniforms
    Speed,          // Used by: Car
    WinnerRainbow,  // Used by: CarWinner
    
    // Post-processing uniforms
    SpotlightEnabled,    // Used by: PostProcessingTag
    SpotlightPos,        // Used by: PostProcessingTag
    SpotlightRadius,     // Used by: PostProcessingTag
    SpotlightSoftness,   // Used by: PostProcessingTag
    DimAmount,           // Used by: PostProcessingTag
    DesaturateAmount,    // Used by: PostProcessingTag
    
    // UV bounds (used by sprite-based shaders)
    UvMin,          // Used by: Car, CarWinner, EntityEnhanced, EntityTest
    UvMax,          // Used by: Car, CarWinner, EntityEnhanced, EntityTest
    
    // Add new uniforms here as needed
};

// Pre-defined uniform names to avoid magic_enum overhead at load time
namespace UniformNames {
    constexpr const char* TIME = "time";
    constexpr const char* RESOLUTION = "resolution";
    constexpr const char* ENTITY_COLOR = "entityColor";
    constexpr const char* SPEED = "speed";
    constexpr const char* WINNER_RAINBOW = "winnerRainbow";
    constexpr const char* SPOTLIGHT_ENABLED = "spotlightEnabled";
    constexpr const char* SPOTLIGHT_POS = "spotlightPos";
    constexpr const char* SPOTLIGHT_RADIUS = "spotlightRadius";
    constexpr const char* SPOTLIGHT_SOFTNESS = "spotlightSoftness";
    constexpr const char* DIM_AMOUNT = "dimAmount";
    constexpr const char* DESATURATE_AMOUNT = "desaturateAmount";
    constexpr const char* UV_MIN = "uvMin";
    constexpr const char* UV_MAX = "uvMax";
}

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
}

// Helper functions for shader enums
namespace ShaderUtils {
    // Convert string to enum (for backward compatibility) using magic_enum
    ShaderType from_string(const std::string& name) {
        auto result = magic_enum::enum_cast<ShaderType>(name);
        return result.value_or(ShaderType::Car); // Default fallback
    }
    
    // Convert enum to string for debugging
    std::string_view to_string(ShaderType shader) {
        return magic_enum::enum_name(shader);
    }
}

// Helper functions for uniform enums
namespace UniformUtils {
    // Convert enum to string for shader lookup using magic_enum
    std::string_view to_string(UniformLocation uniform) {
        return magic_enum::enum_name(uniform);
    }
}
