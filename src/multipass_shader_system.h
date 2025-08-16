#pragma once

#include "std_include.h"
#include "rl.h"
#include "log.h"
#include <magic_enum/magic_enum.hpp>

// Forward declarations
struct Entity;
struct EntityID;
struct HasShader;
struct HasColor;
struct Transform;

// Include other necessary headers
#include "config.h"

// Include window manager for resolution access
#include <afterhours/src/plugins/window_manager.h>

// Include afterhours for EntityHelper
#include <afterhours/ah.h>

// Include texture manager for HasSpritesheet
#include <afterhours/src/plugins/texture_manager.h>

// Simple singleton implementation since afterhours singleton.h is not available
#define SINGLETON_FWD(Type) \
    struct Type; \
    Type& get(); \
    const Type& get_const();

#define SINGLETON(Type) \
    static Type& get() { \
        static Type instance; \
        return instance; \
    } \
    static const Type& get_const() { \
        return get(); \
    } \
    Type(const Type&) = delete; \
    Type& operator=(const Type&) = delete; \
    Type(Type&&) = delete; \
    Type& operator=(Type&&) = delete;

// Render priority system
enum class RenderPriority {
    Background = 0,      // Sky, terrain, map background
    Entities = 100,      // Cars, items, game objects
    Particles = 200,     // Particle effects
    UI = 300,           // HUD, menus, interface
    PostProcess = 400,   // Final effects, bloom, etc.
    Debug = 500          // Debug overlays, profiling
};

// Helper functions for render priority
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

// Helper functions for shader enums
namespace ShaderUtils {
    // Convert string to enum (for backward compatibility) using magic_enum
    constexpr ShaderType from_string(const std::string& name) {
        auto result = magic_enum::enum_cast<ShaderType>(name);
        return result.value_or(ShaderType::Car); // Default fallback
    }
    
    // Convert enum to string for shader lookup using magic_enum
    constexpr const char* to_string(ShaderType shader_type) {
        return magic_enum::enum_name(shader_type).data();
    }
}

// Helper functions for uniform enums
namespace UniformUtils {
    // Convert enum to string for shader lookup using magic_enum
    constexpr const char* to_string(UniformLocation uniform) {
        return magic_enum::enum_name(uniform).data();
    }
}

// Shader pass management
struct ShaderPass {
    std::string name;
    ShaderType shader_type;  // Use enum instead of string
    RenderPriority priority;
    bool enabled = true;
    std::vector<std::string> tags;  // For filtering and debugging
};

struct ShaderPassRegistry {
    SINGLETON(ShaderPassRegistry)
    
    std::vector<ShaderPass> passes;
    
    // Constructor with space reservation
    ShaderPassRegistry() {
        passes.reserve(20);  // Reserve space for typical number of passes
    }
    
    // Add a new shader pass
    void add_pass(const ShaderPass& pass) {
        // Reserve more space if needed to avoid reallocation
        if (passes.size() == passes.capacity()) {
            passes.reserve(passes.size() + 10);
        }
        
        // Insert in sorted position to maintain priority order
        auto insert_pos = std::lower_bound(passes.begin(), passes.end(), pass,
                                         [](const ShaderPass& a, const ShaderPass& b) {
                                             return static_cast<int>(a.priority) < static_cast<int>(b.priority);
                                         });
        passes.insert(insert_pos, pass);
    }
    
    // Batch add multiple passes (more efficient than individual adds)
    void add_passes(const std::vector<ShaderPass>& new_passes) {
        if (new_passes.empty()) return;
        
        // Reserve space for all new passes
        passes.reserve(passes.size() + new_passes.size());
        
        // Insert all passes
        for (const auto& pass : new_passes) {
            auto insert_pos = std::lower_bound(passes.begin(), passes.end(), pass,
                                             [](const ShaderPass& a, const ShaderPass& b) {
                                                 return static_cast<int>(a.priority) < static_cast<int>(b.priority);
                                             });
            passes.insert(insert_pos, pass);
        }
    }
    
    // Get all enabled passes in priority order using views (no copying)
    auto get_enabled_passes() {
        return passes | std::views::filter([](const ShaderPass& pass) {
            return pass.enabled;
        }) | std::views::transform([](const ShaderPass& pass) -> const ShaderPass* {
            return &pass;
        });
    }
    
    // Get passes by priority range using views (no copying)
    auto get_passes_by_priority(RenderPriority min_priority, RenderPriority max_priority) {
        return passes | std::views::filter([min_priority, max_priority](const ShaderPass& pass) {
            return pass.enabled && 
                   static_cast<int>(pass.priority) >= static_cast<int>(min_priority) &&
                   static_cast<int>(pass.priority) <= static_cast<int>(max_priority);
        }) | std::views::transform([](const ShaderPass& pass) -> const ShaderPass* {
            return &pass;
        });
    }
    
    // Enable/disable passes
    void set_pass_enabled(const std::string& name, bool enabled) {
        for (auto& pass : passes) {
            if (pass.name == name) {
                pass.enabled = enabled;
                break;
            }
        }
    }
    
    // Debug information
    std::string get_active_passes_debug_info() const {
        std::string info = "Active Shader Passes:\n";
        for (const auto& pass : passes) {
            if (pass.enabled) {
                info += "  " + pass.name + " (" + ShaderUtils::to_string(pass.shader_type) + ") - Priority " + 
                       std::to_string(static_cast<int>(pass.priority)) + "\n";
            }
        }
        return info;
    }
};

// Updated ShaderLibrary that works with enums
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
    const raylib::Shader& get(ShaderType type) const {
        auto it = shaders_by_type.find(type);
        if (it == shaders_by_type.end()) {
            log_error("Shader not found for type: {}", magic_enum::enum_name(type));
            // Return a default shader or throw
            static raylib::Shader default_shader = raylib::LoadShader("", "");
            return default_shader;
        }
        return it->second;
    }
    
    // Get uniform location (cached)
    int get_uniform_location(ShaderType shader_type, UniformLocation uniform) const {
        auto shader_it = uniform_locations.find(shader_type);
        if (shader_it == uniform_locations.end()) return -1;
        
        auto uniform_it = shader_it->second.find(uniform);
        if (uniform_it == shader_it->second.end()) return -1;
        
        return uniform_it->second;
    }
    
    // Check if shader exists
    bool contains(ShaderType type) const {
        return shaders_by_type.find(type) != shaders_by_type.end();
    }
    
    // Backward compatibility
    const raylib::Shader& get(const std::string& name) const {
        ShaderType type = ShaderUtils::from_string(name);
        return get(type);
    }
    
    bool contains(const std::string& name) const {
        ShaderType type = ShaderUtils::from_string(name);
        return contains(type);
    }
    
private:
    void load_shader(ShaderType type) {
        // Use magic_enum to get shader name and filename
        std::string_view shader_name = magic_enum::enum_name(type);
        std::string vert_path = "resources/shaders/base.vs";
        std::string frag_path = std::string("resources/shaders/") + std::string(shader_name) + ".fs";
        
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
};

// Debug system for shader inspection
struct ShaderDebugger {
  // Show active shaders in debug overlay
  void render_shader_debug_overlay() {
    auto& registry = ShaderPassRegistry::get();
    std::string debug_info = registry.get_active_passes_debug_info();
    
    // Render debug info on screen
    raylib::DrawText(debug_info.c_str(), 10, 10, 20, raylib::WHITE);
  }
  
  // Show entity shader info
  void show_entity_shaders(EntityID entity) {
    auto opt_entity = EntityHelper::getEntityForID(entity);
    if (!opt_entity.valid() || !opt_entity->has<HasShader>()) return;
    
    auto& hasShader = opt_entity->get<HasShader>();
    std::string entity_info = "Entity " + std::to_string(entity) + ": " + 
                             hasShader.get_debug_info();
    
    raylib::DrawText(entity_info.c_str(), 10, 50, 16, raylib::YELLOW);
  }
};

// Configuration function to set up default shader passes
inline void configure_default_passes() {
  auto& registry = ShaderPassRegistry::get();
  
  // Background passes
  registry.add_pass({
    .name = "background_rendering",
    .shader_type = ShaderType::EntityEnhanced,
    .priority = RenderPriority::Background,
    .tags = {"background", "map"}
  });
  
  // Entity passes
  registry.add_pass({
    .name = "entity_shaders",
    .shader_type = ShaderType::Car,
    .priority = RenderPriority::Entities,
    .tags = {"entity", "per_object"}
  });
  
  // Post-processing passes
  registry.add_pass({
    .name = "global_post_processing",
    .shader_type = ShaderType::PostProcessing,
    .priority = RenderPriority::PostProcess,
    .tags = {"post_processing", "global"}
  });
  
  registry.add_pass({
    .name = "bloom_effect",
    .shader_type = ShaderType::PostProcessing,
    .priority = RenderPriority::PostProcess,
    .tags = {"bloom", "post_processing"}
  });
}