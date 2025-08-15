# RFC: Improved Multipass Shader System

## Overview

The current shader system in Kart Afterhours has several limitations that make it difficult to manage complex rendering pipelines and debug shader effects. This RFC proposes a comprehensive redesign that provides better flows for multipass rendering, improved layering control, and more efficient resource management.

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
3. **Resource Inefficiency**: No shader batching or pass optimization
4. **Limited Flexibility**: Can't easily add new rendering passes or effects
5. **CPU Overhead**: Per-entity shader state changes without batching

## Proposed Solution

### 1. Shader Pass Management System

#### Core Components

```cpp
// Shader pass definition with metadata
struct ShaderPass {
    std::string name;
    std::string shader_name;
    int priority;                    // Render order (lower = earlier)
    ShaderScope scope;              // Entity, System, Global, etc.
    std::vector<std::string> tags;  // For filtering and debugging
    bool enabled = true;
    
    // Resource management
    std::optional<raylib::RenderTexture2D> target;
    std::optional<raylib::RenderTexture2D> source;
    
    // Uniform management
    std::unordered_map<std::string, UniformValue> default_uniforms;
};

// Shader scope enumeration
enum class ShaderScope {
    Global,         // Applies to entire game output
    System,         // Applies to specific rendering systems
    Entity,         // Applies to individual entities
    Particle,       // Applies to particle systems
    UI,            // Applies to UI elements
    Custom         // Custom scope for special cases
};

// Uniform value container
struct UniformValue {
    enum class Type { Float, Vec2, Vec3, Vec4, Int, Bool, Texture };
    Type type;
    std::variant<float, vec2, vec3, vec4, int, bool, raylib::Texture2D> value;
};
```

#### Shader Pass Registry

```cpp
struct ShaderPassRegistry {
    SINGLETON(ShaderPassRegistry)
    
    // Register a new shader pass
    void register_pass(const ShaderPass& pass);
    
    // Get passes by scope, priority, or tags
    std::vector<ShaderPass*> get_passes(ShaderScope scope);
    std::vector<ShaderPass*> get_passes_by_priority(int min_priority, int max_priority);
    std::vector<ShaderPass*> get_passes_by_tags(const std::vector<std::string>& tags);
    
    // Enable/disable passes dynamically
    void set_pass_enabled(const std::string& name, bool enabled);
    
    // Debug information
    std::string get_active_passes_debug_info() const;
    std::string get_entity_shaders_debug_info(EntityID entity) const;
};
```

### 2. Enhanced Entity Shader System

#### Improved Component Design

```cpp
// Replace simple HasShader with comprehensive shader management
struct HasShader : BaseComponent {
    std::vector<std::string> shader_passes;  // Multiple shader passes per entity
    std::unordered_map<std::string, UniformValue> custom_uniforms;
    bool inherit_system_shaders = true;      // Apply system-level shaders
    int render_priority = 0;                 // Entity-specific render order
    
    // Debug and validation
    bool validate_shaders() const;
    std::string get_shader_debug_info() const;
};

// System-level shader application
struct SystemShader : BaseComponent {
    std::string system_name;
    std::vector<std::string> shader_passes;
    std::unordered_map<std::string, UniformValue> system_uniforms;
    bool apply_to_children = true;           // Apply to child entities
};
```

### 3. Render Pipeline Management

#### Pass Execution System

```cpp
struct ShaderPassExecutor {
    // Execute all passes in correct order
    void execute_passes(float delta_time);
    
    // Execute specific pass
    void execute_pass(const ShaderPass& pass, float delta_time);
    
    // Batch entity shaders for efficiency
    void batch_entity_shaders(const std::vector<Entity*>& entities);
    
    // Manage render targets and texture switching
    void setup_render_target(const ShaderPass& pass);
    void cleanup_render_target(const ShaderPass& pass);
};

// Render pipeline configuration
struct RenderPipeline {
    std::vector<ShaderPass> passes;
    std::unordered_map<std::string, raylib::RenderTexture2D> render_targets;
    
    // Pipeline validation
    bool validate_pipeline() const;
    std::string get_pipeline_debug_info() const;
};
```

### 4. Uniform Management System

#### Automatic Uniform Updates

```cpp
struct UniformManager {
    // Common uniforms (time, resolution, etc.)
    void update_common_uniforms(raylib::Shader& shader);
    
    // Entity-specific uniforms
    void update_entity_uniforms(raylib::Shader& shader, const Entity& entity);
    
    // System uniforms
    void update_system_uniforms(raylib::Shader& shader, const std::string& system_name);
    
    // Custom uniform injection
    void inject_custom_uniforms(raylib::Shader& shader, const std::unordered_map<std::string, UniformValue>& uniforms);
};
```

### 5. Debug and Development Tools

#### Shader Debug System

```cpp
struct ShaderDebugger {
    // Visual shader debugging
    void render_shader_debug_overlay();
    
    // Entity shader inspection
    void inspect_entity_shaders(EntityID entity);
    
    // Pass execution logging
    void log_pass_execution(const ShaderPass& pass, bool success);
    
    // Performance metrics
    struct ShaderMetrics {
        float execution_time;
        int draw_calls;
        int uniform_updates;
        bool cached;
    };
    std::unordered_map<std::string, ShaderMetrics> get_shader_metrics();
};
```

## Implementation Details

### 1. Shader Pass Configuration

```cpp
// Example shader pass configuration
void configure_default_passes() {
    auto& registry = ShaderPassRegistry::get();
    
    // Global post-processing pass
    registry.register_pass({
        .name = "global_post_processing",
        .shader_name = "post_processing",
        .priority = 1000,
        .scope = ShaderScope::Global,
        .tags = {"post_processing", "global"},
        .target = std::nullopt,  // Render to screen
        .source = "main_output"
    });
    
    // Entity rendering pass
    registry.register_pass({
        .name = "entity_shaders",
        .shader_name = "entity_default",
        .priority = 100,
        .scope = ShaderScope::Entity,
        .tags = {"entity", "per_object"},
        .target = "entity_output",
        .source = "main_scene"
    });
    
    // Particle system pass
    registry.register_pass({
        .name = "particle_effects",
        .shader_name = "particle_system",
        .priority = 200,
        .scope = ShaderScope::Particle,
        .tags = {"particle", "effects"},
        .target = "particle_output",
        .source = "entity_output"
    });
}
```

### 2. Entity Shader Application

```cpp
// System for applying entity shaders efficiently
struct ApplyEntityShaders : System<HasShader, Transform> {
    virtual void for_each_with(Entity& entity, HasShader& hasShader, const Transform& transform, float dt) override {
        // Validate shaders before application
        if (!hasShader.validate_shaders()) {
            log_warn("Entity {} has invalid shaders", entity.id());
            return;
        }
        
        // Apply each shader pass
        for (const auto& pass_name : hasShader.shader_passes) {
            auto* pass = ShaderPassRegistry::get().get_pass(pass_name);
            if (!pass || !pass->enabled) continue;
            
            // Apply shader with entity-specific uniforms
            apply_entity_shader_pass(*pass, entity, hasShader, transform);
        }
    }
    
private:
    void apply_entity_shader_pass(const ShaderPass& pass, const Entity& entity, 
                                 const HasShader& hasShader, const Transform& transform) {
        const auto& shader = ShaderLibrary::get().get(pass.shader_name);
        
        // Begin shader mode
        raylib::BeginShaderMode(shader);
        
        // Update uniforms
        UniformManager::get().update_common_uniforms(shader);
        UniformManager::get().update_entity_uniforms(shader, entity);
        UniformManager::get().inject_custom_uniforms(shader, hasShader.custom_uniforms);
        
        // Render entity (delegated to existing rendering systems)
        render_entity_with_shader(entity, shader);
        
        raylib::EndShaderMode();
    }
};
```

### 3. Render Pipeline Execution

```cpp
// Main render pipeline execution
struct ExecuteRenderPipeline : System<> {
    virtual void once(float dt) override {
        auto& executor = ShaderPassExecutor::get();
        auto& registry = ShaderPassRegistry::get();
        
        // Get passes in priority order
        auto passes = registry.get_passes_by_priority(0, 9999);
        
        // Execute each pass
        for (auto* pass : passes) {
            if (!pass->enabled) continue;
            
            executor.execute_pass(*pass, dt);
        }
    }
};
```

### 4. Performance Optimizations

#### Shader Batching

```cpp
struct ShaderBatchingSystem {
    // Group entities by shader to minimize state changes
    void batch_entities_by_shader() {
        std::unordered_map<std::string, std::vector<Entity*>> shader_groups;
        
        // Group entities by active shader combinations
        for (auto& entity : EntityHelper::get_entities()) {
            if (auto* hasShader = entity.get<HasShader>()) {
                std::string shader_key = create_shader_key(*hasShader);
                shader_groups[shader_key].push_back(&entity);
            }
        }
        
        // Render each group with minimal shader switches
        for (const auto& [shader_key, entities] : shader_groups) {
            render_entity_batch(entities, shader_key);
        }
    }
    
private:
    std::string create_shader_key(const HasShader& hasShader) {
        // Create unique key for shader combination
        std::string key;
        for (const auto& pass : hasShader.shader_passes) {
            key += pass + "|";
        }
        return key;
    }
};
```

#### Render Target Management

```cpp
struct RenderTargetManager {
    // Pool of render textures to minimize allocation
    std::vector<raylib::RenderTexture2D> texture_pool;
    std::unordered_set<raylib::RenderTexture2D*> active_textures;
    
    // Get or create render target
    raylib::RenderTexture2D& get_render_target(const std::string& name, int width, int height) {
        // Find existing target or create new one
        for (auto& target : texture_pool) {
            if (target.texture.width == width && target.texture.height == height) {
                return target;
            }
        }
        
        // Create new target
        auto new_target = raylib::LoadRenderTexture(width, height);
        texture_pool.push_back(new_target);
        return texture_pool.back();
    }
    
    // Cleanup unused targets
    void cleanup_unused_targets() {
        // Remove targets not used in current frame
        // Implementation details...
    }
};
```

## Migration Strategy

### Phase 1: Core Infrastructure
1. Implement `ShaderPassRegistry` and `ShaderPass` structures
2. Create `UniformManager` for centralized uniform handling
3. Add basic pass execution system

### Phase 2: Entity System Integration
1. Enhance `HasShader` component with new capabilities
2. Implement shader validation and debugging
3. Add system-level shader support

### Phase 3: Pipeline Management
1. Implement `ShaderPassExecutor` and `RenderPipeline`
2. Add render target management
3. Create shader batching system

### Phase 4: Optimization and Tools
1. Implement performance optimizations
2. Add comprehensive debugging tools
3. Create development utilities

## Benefits

### 1. Improved Developer Experience
- **Clear Shader Flow**: Easy to understand which shaders apply where
- **Better Debugging**: Visual tools for shader inspection
- **Flexible Configuration**: Easy to add new effects and passes

### 2. Better Performance
- **Reduced State Changes**: Shader batching minimizes GPU state switches
- **Efficient Resource Management**: Render target pooling and reuse
- **CPU Optimization**: Minimized uniform updates and shader validation

### 3. Enhanced Flexibility
- **Multiple Passes**: Entities can have multiple shader effects
- **Dynamic Layering**: Runtime control over render order
- **System Integration**: Easy to apply effects to entire systems

### 4. Maintainability
- **Centralized Management**: All shader logic in one place
- **Validation**: Automatic shader validation and error detection
- **Documentation**: Self-documenting shader configurations

## Risks and Mitigation

### 1. Complexity Increase
- **Risk**: New system adds complexity to existing codebase
- **Mitigation**: Gradual migration with clear documentation and examples

### 2. Performance Overhead
- **Risk**: New abstraction layers could add overhead
- **Mitigation**: Performance testing and optimization at each phase

### 3. Breaking Changes
- **Risk**: Changes to existing shader system could break current functionality
- **Mitigation**: Maintain backward compatibility during migration

## Conclusion

The proposed multipass shader system addresses the current limitations while providing a solid foundation for future rendering enhancements. The phased implementation approach ensures minimal disruption to existing functionality while delivering immediate benefits in terms of debugging and maintainability.

The system's focus on performance, flexibility, and developer experience makes it well-suited for a game that needs to run efficiently on various devices while supporting complex visual effects and easy debugging.