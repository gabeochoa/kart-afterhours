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

#### Improved Component Design

```cpp
// Replace simple HasShader with better capabilities
struct HasShader : BaseComponent {
    std::vector<std::string> shader_names;  // Multiple shaders per entity
    RenderPriority render_priority = RenderPriority::Entities;  // When to render
    bool enabled = true;
    
    // Easy debugging
    std::string get_debug_info() const {
        std::string info = "Shaders: ";
        for (const auto& name : shader_names) {
            info += name + " ";
        }
        info += "Priority: " + std::to_string(static_cast<int>(render_priority));
        return info;
    }
    
    // Validation
    bool has_shader(const std::string& name) const {
        return std::find(shader_names.begin(), shader_names.end(), name) != shader_names.end();
    }
    
    // Constructor helpers
    HasShader(const std::string& name) : shader_names{name} {}
    HasShader(const std::vector<std::string>& names) : shader_names(names) {}
    HasShader(const char* name) : shader_names{std::string(name)} {}
};
```

### 3. Simple Shader Pass Management

#### Basic Pass Registry

```cpp
struct ShaderPass {
    std::string name;
    std::string shader_name;
    RenderPriority priority;
    bool enabled = true;
    std::vector<std::string> tags;  // For filtering and debugging
};

struct ShaderPassRegistry {
    SINGLETON(ShaderPassRegistry)
    
    std::vector<ShaderPass> passes;
    
    // Add a new shader pass
    void add_pass(const ShaderPass& pass) {
        // Insert in sorted position to maintain priority order
        auto insert_pos = std::lower_bound(passes.begin(), passes.end(), pass,
                                         [](const ShaderPass& a, const ShaderPass& b) {
                                             return static_cast<int>(a.priority) < static_cast<int>(b.priority);
                                         });
        passes.insert(insert_pos, pass);
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
                info += "  " + pass.name + " (" + pass.shader_name + ") - Priority " + 
                       std::to_string(static_cast<int>(pass.priority)) + "\n";
            }
        }
        return info;
    }
};
```

### 4. Improved Rendering System

#### Enhanced Entity Shader Application

```cpp
// System for applying entity shaders efficiently
struct ApplyEntityShaders : System<HasShader, Transform> {
    virtual void for_each_with(Entity& entity, HasShader& hasShader, const Transform& transform, float dt) override {
        if (!hasShader.enabled) return;
        
        // Apply each shader in order
        for (const auto& shader_name : hasShader.shader_names) {
            if (!ShaderLibrary::get().contains(shader_name)) {
                log_warn("Shader not found: {}", shader_name);
                continue;
            }
            
            const auto& shader = ShaderLibrary::get().get(shader_name);
            raylib::BeginShaderMode(shader);
            
            // Update common uniforms
            update_common_uniforms(shader);
            
            // Update entity-specific uniforms
            update_entity_uniforms(shader, entity, hasShader, transform);
            
            // Render entity (delegated to existing rendering systems)
            render_entity_with_shader(entity, shader);
            
            raylib::EndShaderMode();
        }
    }
    
private:
    void update_common_uniforms(raylib::Shader& shader) {
        // Time uniform
        float time = static_cast<float>(raylib::GetTime());
        int time_loc = raylib::GetShaderLocation(shader, "time");
        if (time_loc != -1) {
            raylib::SetShaderValue(shader, time_loc, &time, raylib::SHADER_UNIFORM_FLOAT);
        }
        
        // Resolution uniform
        auto* rez_cmp = EntityHelper::get_singleton_cmp<window_manager::ProvidesCurrentResolution>();
        if (rez_cmp) {
            vec2 rez = {static_cast<float>(rez_cmp->current_resolution.width),
                        static_cast<float>(rez_cmp->current_resolution.height)};
            int rez_loc = raylib::GetShaderLocation(shader, "resolution");
            if (rez_loc != -1) {
                raylib::SetShaderValue(shader, rez_loc, &rez, raylib::SHADER_UNIFORM_VEC2);
            }
        }
    }
    
    void update_entity_uniforms(raylib::Shader& shader, const Entity& entity, 
                               const HasShader& hasShader, const Transform& transform) {
        // Entity color
        if (entity.has<HasColor>()) {
            auto& hasColor = entity.get<HasColor>();
            raylib::Color entityColor = hasColor.color();
            float color_array[4] = {
                entityColor.r / 255.0f, entityColor.g / 255.0f,
                entityColor.b / 255.0f, entityColor.a / 255.0f
            };
            int color_loc = raylib::GetShaderLocation(shader, "entityColor");
            if (color_loc != -1) {
                raylib::SetShaderValue(shader, color_loc, color_array, raylib::SHADER_UNIFORM_VEC4);
            }
        }
        
        // Car-specific uniforms
        if (hasShader.shader_name == "car") {
            float speed_percent = transform.speed() / Config::get().max_speed.data;
            int speed_loc = raylib::GetShaderLocation(shader, "speed");
            if (speed_loc != -1) {
                raylib::SetShaderValue(shader, speed_loc, &speed_percent, raylib::SHADER_UNIFORM_FLOAT);
            }
        }
        
        // Winner effects
        if (hasShader.shader_name == "car_winner") {
            float rainbow_on = 1.0f;
            int rainbow_loc = raylib::GetShaderLocation(shader, "winnerRainbow");
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
        .shader_name = "background",
        .priority = RenderPriority::Background,
        .tags = {"background", "map"}
    });
    
    // Entity passes
    registry.add_pass({
        .name = "entity_shaders",
        .shader_name = "entity_default",
        .priority = RenderPriority::Entities,
        .tags = {"entity", "per_object"}
    });
    
    // Post-processing passes
    registry.add_pass({
        .name = "global_post_processing",
        .shader_name = "post_processing",
        .priority = RenderPriority::PostProcess,
        .tags = {"post_processing", "global"}
    });
    
    registry.add_pass({
        .name = "bloom_effect",
        .shader_name = "bloom",
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