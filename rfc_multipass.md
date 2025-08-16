# RFC: Improved Multipass Shader System

## Overview

The current shader system in Kart Afterhours has several limitations that make it difficult to manage complex rendering pipelines and debug shader effects. This RFC proposes practical improvements that provide better organization and debugging without overcomplicating the existing system.

## Current State Analysis

### Existing Implementation
- **Per-Entity Shaders**: `HasShader` component with string-based shader names
- **Post-Processing**: Global post-processing shaders applied to final output
- **Manual Management**: Shader begin/end calls scattered throughout rendering systems
- **Limited Layering**: Basic render texture pipeline with mainRT → screenRT → screen
- **Hardcoded Passes**: Fixed rendering order in main.cpp

### Current Problems
1. **Debugging Difficulty**: Hard to determine which shaders are active on entities
2. **Render Order Complexity**: Manual management of shader begin/end pairs
3. **Limited Flexibility**: Can't easily add new rendering passes or effects
4. **Magic Numbers**: Unclear render priorities (what does priority 150 vs 200 mean?)
5. **Single Shader Limit**: Entities can only have one shader at a time

## Proposed Solution

### 1. Simple Priority System

#### Replace Magic Numbers with Clear Enums

```cpp
enum class RenderPriority {
    Background = 0,      // Sky, terrain, map background
    Entities = 100,      // Cars, items, game objects
    Particles = 200,     // Particle effects
    UI = 300,           // HUD, menus, interface
    PostProcess = 400,   // Final effects, bloom, etc.
    Debug = 500          // Debug overlays, profiling
};

// Helper functions
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
```

### 2. Enhanced Entity Shader System

#### Shader and Uniform Enum Definitions

```cpp
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

// Helper functions for shader enums
namespace ShaderUtils {
    // Convert string to enum (for backward compatibility) using magic_enum
    constexpr ShaderType from_string(const std::string& name) {
        auto result = magic_enum::enum_cast<ShaderType>(name);
        return result.value_or(ShaderType::Car); // Default fallback
    }
    
    // Get shader filename from enum using magic_enum
    constexpr const char* get_filename(ShaderType shader) {
        return magic_enum::enum_name(shader).data();
    }
}

// Note: Shader file names must match the enum names exactly
// Current files: car.fs, car_winner.fs, entity_enhanced.fs, etc.
// Should be renamed to: Car.fs, CarWinner.fs, EntityEnhanced.fs, etc.
// Or use lowercase conversion in get_filename() if keeping current names

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

// Helper functions for uniform enums
namespace UniformUtils {
    // Convert enum to string for shader lookup using magic_enum
    constexpr const char* to_string(UniformLocation uniform) {
        return magic_enum::enum_name(uniform).data();
    }
}
```
```

#### Improved Component Design

```cpp
// Replace simple HasShader with better capabilities
struct HasShader : BaseComponent {
    std::vector<ShaderType> shaders;  // Multiple shaders per entity using enums
    RenderPriority render_priority = RenderPriority::Entities;  // When to render
    bool enabled = true;
    
    // Cache for fast shader lookups (mutable for const methods)
    mutable std::optional<std::unordered_set<ShaderType>> shader_set_cache;
    
    // Easy debugging
    std::string get_debug_info() const {
        if (shaders.empty()) return "No shaders";
        
        // Use thread_local buffer to avoid allocations every frame
        static thread_local std::string buffer;
        buffer.clear();
        buffer.reserve(shaders.size() * 15);
        
        buffer += "Shaders: ";
        for (const auto& shader : shaders) {
            buffer += magic_enum::enum_name(shader);  // No string allocation
            buffer += " ";
        }
        buffer += "Priority: " + std::to_string(static_cast<int>(render_priority));
        return buffer;
    }
    
    // Fast shader validation using cached set
    bool has_shader(ShaderType shader) const {
        // Rebuild cache if shaders changed
        if (!shader_set_cache.has_value()) {
            shader_set_cache = std::unordered_set<ShaderType>(shaders.begin(), shaders.end());
        }
        return shader_set_cache->contains(shader);
    }
    
    // Constructor helpers
    HasShader(ShaderType shader) : shaders{shader} {}
    HasShader(const std::vector<ShaderType>& shader_list) : shaders(shader_list) {}
    
    // Backward compatibility constructors
    HasShader(const std::string& name) : shaders{ShaderUtils::from_string(name)} {}
    HasShader(const char* name) : shaders{ShaderUtils::from_string(std::string(name))} {}
    
    // Methods to modify shaders (invalidate cache)
    void add_shader(ShaderType shader) {
        shaders.push_back(shader);
        shader_set_cache.reset();  // Invalidate cache
    }
    
    void remove_shader(ShaderType shader) {
        auto it = std::find(shaders.begin(), shaders.end(), shader);
        if (it != shaders.end()) {
            shaders.erase(it);
            shader_set_cache.reset();  // Invalidate cache
        }
    }
    
    void clear_shaders() {
        shaders.clear();
        shader_set_cache.reset();  // Invalidate cache
    }
};
```

### 3. Simple Shader Pass Management

#### Basic Pass Registry

```cpp
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
```

### 4. Updated Shader Library

#### Enum-Based Shader Management

```cpp
// Updated ShaderLibrary that works with enums
struct ShaderLibrary {
    SINGLETON(ShaderLibrary)
    
    // Store shaders by enum type for fast lookup
    std::unordered_map<ShaderType, raylib::Shader> shaders_by_type;
    
    // Cache uniform locations for each shader
    std::unordered_map<ShaderType, std::unordered_map<UniformLocation, int>> uniform_locations;
    
    // Load all shaders at startup
    void load_all_shaders() {
        // Load each shader and cache its uniform locations
        load_shader(ShaderType::Car);
        load_shader(ShaderType::CarWinner);
        load_shader(ShaderType::EntityEnhanced);
        load_shader(ShaderType::EntityTest);
        load_shader(ShaderType::PostProcessing);
        load_shader(ShaderType::PostProcessingTag);
        load_shader(ShaderType::TextMask);
    }
    
    // Get shader by enum type (fast lookup)
    const raylib::Shader& get(ShaderType type) const {
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
        const char* filename = ShaderUtils::get_filename(type);
        std::string vert_path = "resources/shaders/base.vs";
        std::string frag_path = std::string("resources/shaders/") + filename + ".fs";
        
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
```

### 5. Improved Rendering System

#### Enhanced Entity Shader Application

```cpp
// System for applying entity shaders efficiently
struct ApplyEntityShaders : System<HasShader, Transform> {
    virtual void for_each_with(Entity& entity, HasShader& hasShader, const Transform& transform, float dt) override {
        if (!hasShader.enabled) return;
        
        // Apply each shader in order
        for (const auto& shader_type : hasShader.shaders) {
            if (!ShaderLibrary::get().contains(shader_type)) {
                log_warn("Shader not found for type: {}", static_cast<int>(shader_type));
                continue;
            }
            
            const auto& shader = ShaderLibrary::get().get(shader_type);
            raylib::BeginShaderMode(shader);
            
            // Update common uniforms
            update_common_uniforms(shader, shader_type);
            
            // Update entity-specific uniforms
            update_entity_uniforms(shader, shader_type, entity, hasShader, transform);
            
            // Render entity (delegated to existing rendering systems)
            render_entity_with_shader(entity, shader);
            
            raylib::EndShaderMode();
        }
    }
    
private:
    void update_common_uniforms(raylib::Shader& shader, ShaderType shader_type) {
        // Time uniform
        float time = static_cast<float>(raylib::GetTime());
        int time_loc = ShaderLibrary::get().get_uniform_location(shader_type, UniformLocation::Time);
        if (time_loc != -1) {
            raylib::SetShaderValue(shader, time_loc, &time, raylib::SHADER_UNIFORM_FLOAT);
        }
        
        // Resolution uniform
        auto* rez_cmp = EntityHelper::get_singleton_cmp<window_manager::ProvidesCurrentResolution>();
        if (rez_cmp) {
            vec2 rez = {static_cast<float>(rez_cmp->current_resolution.width),
                        static_cast<float>(rez_cmp->current_resolution.height)};
            int rez_loc = ShaderLibrary::get().get_uniform_location(shader_type, UniformLocation::Resolution);
            if (rez_loc != -1) {
                raylib::SetShaderValue(shader, rez_loc, &rez, raylib::SHADER_UNIFORM_VEC2);
            }
        }
    }
    
    void update_entity_uniforms(raylib::Shader& shader, ShaderType shader_type, const Entity& entity, 
                               const HasShader& hasShader, const Transform& transform) {
        // Entity color
        if (entity.has<HasColor>()) {
            auto& hasColor = entity.get<HasColor>();
            raylib::Color entityColor = hasColor.color();
            float color_array[4] = {
                entityColor.r / 255.0f, entityColor.g / 255.0f,
                entityColor.b / 255.0f, entityColor.a / 255.0f
            };
            int color_loc = ShaderLibrary::get().get_uniform_location(shader_type, UniformLocation::EntityColor);
            if (color_loc != -1) {
                raylib::SetShaderValue(shader, color_loc, color_array, raylib::SHADER_UNIFORM_VEC4);
            }
        }
        
        // Car-specific uniforms
        if (shader_type == ShaderType::Car) {
            float speed_percent = transform.speed() / Config::get().max_speed.data;
            int speed_loc = ShaderLibrary::get().get_uniform_location(shader_type, UniformLocation::Speed);
            if (speed_loc != -1) {
                raylib::SetShaderValue(shader, speed_loc, &speed_percent, raylib::SHADER_UNIFORM_FLOAT);
            }
        }
        
        // Winner effects
        if (shader_type == ShaderType::CarWinner) {
            float rainbow_on = 1.0f;
            int rainbow_loc = ShaderLibrary::get().get_uniform_location(shader_type, UniformLocation::WinnerRainbow);
            if (rainbow_loc != -1) {
                raylib::SetShaderValue(shader, rainbow_loc, &rainbow_on, raylib::SHADER_UNIFORM_FLOAT);
            }
        }
    }
    
    void render_entity_with_shader(const Entity& entity, const raylib::Shader& shader) {
        // This integrates with existing rendering systems
        // The shader is already active when this is called
        if (entity.has<afterhours::texture_manager::HasSprite>()) {
            render_sprite_with_shader(entity, shader);
        } else if (entity.has<afterhours::texture_manager::HasAnimation>()) {
            render_animation_with_shader(entity, shader);
        }
    }
    
    void render_sprite_with_shader(const Entity& entity, const raylib::Shader& shader) {
        // Simplified sprite rendering with shader already active
        auto& transform = entity.get<Transform>();
        auto& hasSprite = entity.get<afterhours::texture_manager::HasSprite>();
        auto& hasColor = entity.get<HasColor>();
        
        auto* spritesheet_component = EntityHelper::get_singleton_cmp<
            afterhours::texture_manager::HasSpritesheet>();
        if (!spritesheet_component) {
            log_warn("No spritesheet component found!");
            return;
        }
        
        raylib::Texture2D sheet = spritesheet_component->texture;
        Rectangle source_frame = afterhours::texture_manager::idx_to_sprite_frame(0, 1);
        
        float dest_width = source_frame.width * hasSprite.scale;
        float dest_height = source_frame.height * hasSprite.scale;
        
        // Apply sprite offset calculations
        float offset_x = SPRITE_OFFSET_X;
        float offset_y = SPRITE_OFFSET_Y;
        float rotated_x = offset_x * cosf(transform.angle * M_PI / 180.f) -
                          offset_y * sinf(transform.angle * M_PI / 180.f);
        float rotated_y = offset_x * sinf(transform.angle * M_PI / 180.f) +
                          offset_y * cosf(transform.angle * M_PI / 180.f);
        
        raylib::DrawTexturePro(
            sheet, source_frame,
            Rectangle{
                transform.position.x + transform.size.x / 2.f + rotated_x,
                transform.position.y + transform.size.y / 2.f + rotated_y,
                dest_width, dest_height
            },
            vec2{dest_width / 2.f, dest_height / 2.f}, 
            transform.angle, 
            hasColor.color()
        );
    }
};
```

### 5. Simple Pass Configuration

#### Example Usage

```cpp
// Set up shader passes once at startup
void configure_default_passes() {
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

// Execute passes in priority order
void execute_render_pipeline() {
    auto& registry = ShaderPassRegistry::get();
    
    for (auto* pass : registry.get_enabled_passes()) {
        if (pass->enabled) {
            execute_shader_pass(*pass);
        }
    }
}
```

### 6. Debug and Development Tools

#### Simple Debug System

```cpp
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
        auto* entity_ptr = EntityHelper::get_entity(entity);
        if (!entity_ptr || !entity_ptr->has<HasShader>()) return;
        
        auto& hasShader = entity_ptr->get<HasShader>();
        std::string entity_info = "Entity " + std::to_string(entity) + ": " + 
                                 hasShader.get_debug_info();
        
        raylib::DrawText(entity_info.c_str(), 10, 50, 16, raylib::YELLOW);
    }
};
```

## Performance Optimizations

### 1. Cached Uniform Locations
**Biggest performance win for minimal complexity**

- **Before**: `GetShaderLocation(shader, "time")` called every frame for every uniform
- **After**: Uniform locations cached at shader load time, O(1) lookup
- **Result**: Eliminates repeated GPU driver calls, significant CPU savings

```cpp
// Uniform locations cached once at load time
int time_loc = ShaderLibrary::get().get_uniform_location(shader_type, UniformLocation::Time);
if (time_loc != -1) {
    raylib::SetShaderValue(shader, time_loc, &time, raylib::SHADER_UNIFORM_FLOAT);
}
```

### 2. Enum-Based Shader Lookup
**Eliminates string conversions in hot path**

- **Before**: `ShaderLibrary::get("car")` with string operations every frame
- **After**: `ShaderLibrary::get(ShaderType::Car)` direct enum lookup
- **Result**: O(1) hash map access instead of string operations

```cpp
// Direct enum lookup - no string operations
const auto& shader = ShaderLibrary::get().get(ShaderType::Car);
```

### 3. Cached Shader Set for has_shader()
**Better for entities with multiple shaders**

- **Before**: O(n) linear search through shader vector every time
- **After**: O(1) hash set lookup with automatic cache invalidation
- **Result**: Fast lookups for entities with multiple shaders

```cpp
// Fast lookup using cached set
bool has_shader(ShaderType shader) const {
    if (!shader_set_cache.has_value()) {
        shader_set_cache = std::unordered_set<ShaderType>(shaders.begin(), shaders.end());
    }
    return shader_set_cache->contains(shader);
}
```

### 4. Vector Space Reservation
**Simple memory optimization**

- **Before**: Vector reallocates and copies elements when growing
- **After**: Pre-allocated space prevents reallocation during insertion
- **Result**: Eliminates memory allocations and element copying

```cpp
// Reserve space to avoid reallocation
ShaderPassRegistry() {
    passes.reserve(20);  // Reserve space for typical number of passes
}

void add_pass(const ShaderPass& pass) {
    if (passes.size() == passes.capacity()) {
        passes.reserve(passes.size() + 10);  // Reserve more space as needed
    }
    // ... insert pass
}
```

## Implementation Details

### 1. Migration Strategy

#### Phase 1: Core Improvements
1. Add `RenderPriority` enum
2. Enhance `HasShader` component to support multiple shaders
3. Create simple `ShaderPassRegistry`

#### Phase 2: Integration
1. Update existing rendering systems to use new priority system
2. Add debug tools for shader inspection
3. Test with existing shaders

#### Phase 3: Optimization
1. Add shader batching if needed
2. Optimize uniform updates
3. Add performance monitoring

### 2. Backward Compatibility

- Existing `HasShader` components continue to work
- Single shader names are automatically converted to vector format
- Default priority is `RenderPriority::Entities` for existing entities

## Benefits

### 1. Improved Developer Experience
- **Clear Priorities**: `RenderPriority::Entities` instead of magic number 100
- **Multiple Shaders**: Entities can have multiple effects
- **Better Debugging**: Easy to see what shaders are active

### 2. Better Organization
- **Structured Passes**: Clear organization of rendering order
- **Easy Configuration**: Simple to add new shader passes
- **Consistent Flow**: Predictable render pipeline

### 3. Maintainability
- **No Magic Numbers**: Clear, self-documenting priorities
- **Centralized Logic**: Shader management in one place
- **Easy Extension**: Simple to add new features

## Risks and Mitigation

### 1. Complexity Increase
- **Risk**: New system adds some complexity
- **Mitigation**: Keep changes minimal and focused

### 2. Performance Impact
- **Risk**: New abstraction could add overhead
- **Mitigation**: Profile and optimize as needed

### 3. Breaking Changes
- **Risk**: Changes to existing system
- **Mitigation**: Maintain backward compatibility

## Conclusion

This simplified approach provides the core benefits of better shader management without overcomplicating the system. The focus is on practical improvements that make the existing codebase easier to understand, debug, and extend.

The key improvements are:
1. **Clear priorities** instead of magic numbers
2. **Multiple shaders per entity** for more complex effects
3. **Better debugging tools** to see what's happening
4. **Organized pass management** without complex abstractions

This maintains the simplicity of the current system while addressing the main pain points of debugging and organization.