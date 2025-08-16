# RFC: Initial Theming System for Afterhours UI

## Overview

This RFC proposes a comprehensive theming system for the afterhours UI library that will eliminate the need for manual component sizing and styling in game code. The goal is to provide a clean, declarative API where developers can set a default theme once and have all UI components automatically sized and styled appropriately.

## Current State

Currently, the kart project manually configures component defaults in `SetupGameStylingDefaults`:

```cpp
auto &styling_defaults = UIStylingDefaults::get();

styling_defaults.set_component_config(
    ComponentType::Button,
    ComponentConfig{}
        .with_padding(Padding{.top = screen_pct(5.f / 720.f),
                              .left = pixels(0.f),
                              .bottom = screen_pct(5.f / 720.f),
                              .right = pixels(0.f)})
        .with_size(ComponentSize{screen_pct(200.f / 1280.f),
                                 screen_pct(50.f / 720.f)})
        .with_color_usage(Theme::Usage::Primary));
```

This approach has several issues:
1. **Manual sizing**: Every component type requires explicit size configuration
2. **Hardcoded values**: Sizes are tied to specific screen resolutions
3. **Scattered configuration**: Defaults are set in game code rather than UI library
4. **No theme inheritance**: Components can't inherit from a base theme
5. **Limited flexibility**: No support for different themes or responsive design

## Proposed Solution

### 1. Unified Theme Structure

Merge `Theme`, `ThemeManager`, and `UIStylingDefaults` into a single, comprehensive `Theme` class:

```cpp
namespace afterhours::ui {

class Theme {
public:
    // Colors (existing from current Theme)
    Color font;
    Color darkfont;
    Color background;
    Color primary;
    Color secondary;
    Color accent;
    Color error;
    
    // Component sizing defaults (from UIStylingDefaults)
    std::map<ComponentType, ComponentConfig> component_defaults;
    
    // Global styling defaults
    Padding default_padding;
    Margin default_margin;
    float default_font_size;
    std::string default_font_name;
    float default_component_spacing;
    float default_section_spacing;
    
    // Theme metadata
    std::string name;
    std::string variant; // "light", "dark", "compact", "spacious"
    
    // Singleton pattern (from UIStylingDefaults)
    static Theme& get();
    
    // Constructor with sensible defaults
    Theme();
    
    // Factory methods for common themes
    static Theme light();
    static Theme dark();
    static Theme compact();
    static Theme spacious();
    
    // Component configuration methods (from UIStylingDefaults)
    Theme& set_component_default(ComponentType type, const ComponentConfig& config);
    std::optional<ComponentConfig> get_component_default(ComponentType type) const;
    ComponentConfig merge_with_defaults(ComponentType type, const ComponentConfig& overrides) const;
    
    // Convenience methods for common patterns
    Theme& with_component_size(ComponentType type, float width, float height);
    Theme& with_standard_sizes(float width, float height); // All components get same size
    Theme& with_default_padding(float padding);
    Theme& with_default_font_size(float size);
    
    // Theme switching and management
    void switch_to(const std::string& theme_name);
    void register_variant(const std::string& name, const Theme& variant);
    
    // Global theme setting (replaces UIStylingDefaults::get())
    static void set_default_theme(const Theme& theme);
    static const Theme& get_default_theme();
    
    // Pre-built themes
    static Theme kart_style(); // Pre-configured for kart games
    
private:
    static Theme* default_theme_instance;
    std::map<std::string, Theme> registered_variants;
};

} // namespace afterhours::ui
```

### 2. ComponentConfig Integration Analysis

**Question: Should ComponentConfig be merged with Theme?**

After analyzing the current `ComponentConfig` structure, I believe **partial integration** is better than full merger:

**Keep ComponentConfig separate because:**
1. **Different lifecycle**: ComponentConfig is per-component instance, Theme is global
2. **Different scope**: ComponentConfig has instance-specific data (label, disabled state, etc.)
3. **Builder pattern**: ComponentConfig's fluent interface is valuable for component creation
4. **Performance**: ComponentConfig is lightweight and copied frequently

**Integrate where it makes sense:**
1. **Theme inheritance**: ComponentConfig automatically inherits from Theme defaults
2. **Smart defaults**: ComponentConfig can be created with minimal configuration
3. **Auto-sizing**: ComponentConfig can defer sizing decisions to Theme

**Proposed ComponentConfig enhancements:**
```cpp
struct ComponentConfig {
    // Existing fields...
    
    // Theme inheritance: everything inherits by default unless explicitly set
    // Just set the values you want to override, everything else comes from theme
};
```

### 3. New API for Setting Default Theme

Replace the current manual configuration with a simple theme setting:

```cpp
// In kart code, replace SetupGameStylingDefaults with:
void setup_default_theme() {
    // Option 1: Use a predefined theme
    afterhours::ui::Theme::set_default_theme(afterhours::ui::Theme::compact());
    
    // Option 2: Create a custom theme with fluent API
    afterhours::ui::Theme kart_theme = afterhours::ui::Theme::compact()
        .with_component_size(ComponentType::Button, screen_pct(0.15f), screen_pct(0.07f))
        .with_component_size(ComponentType::Slider, screen_pct(0.15f), screen_pct(0.07f))
        .with_component_size(ComponentType::Checkbox, screen_pct(0.15f), screen_pct(0.07f))
        .with_default_padding(screen_pct(0.01f))
        .with_default_font_size(24.0f);
    
    afterhours::ui::Theme::set_default_theme(kart_theme);
}

// Option 3: Even more concise with size presets
void setup_default_theme_concise() {
    afterhours::ui::Theme::set_default_theme(
        afterhours::ui::Theme::compact()
            .with_standard_sizes(screen_pct(0.15f), screen_pct(0.07f)) // All components get same size
            .with_default_padding(screen_pct(0.01f))
            .with_default_font_size(24.0f)
    );
}

// Option 4: Ultra-concise with theme presets
void setup_default_theme_ultra_concise() {
    afterhours::ui::Theme::set_default_theme(afterhours::ui::Theme::kart_style());
}

// Summary: From verbose to ultra-concise
// 
// Before (current approach): 15+ lines of manual configuration
// Option 1 (predefined):    1 line
// Option 2 (fluent API):    8 lines, more readable
// Option 3 (size presets):  6 lines, less repetition
// Option 4 (theme preset):  1 line, zero configuration needed
```

### 4. Enhanced ComponentConfig with Theme Inheritance

Modify `ComponentConfig` to automatically inherit from theme defaults:

```cpp
struct ComponentConfig {
    // Existing fields...
    
    // Theme inheritance: everything inherits by default unless explicitly set
    // Just set the values you want to override, everything else comes from theme
};
```

### 5. Smart Auto-layout System

Enhance the autolayout system to work with theme defaults:

```cpp
namespace afterhours::ui::autolayout {

class SmartLayout {
public:
    // Calculate optimal size based on theme and content
    static ComponentSize calculate_optimal_size(
        ComponentType type,
        const std::string& content = "",
        const ComponentConfig& overrides = {}
    );
    
    // Auto-arrange components in a container
    static void auto_arrange_children(Entity& container, 
                                    const ComponentConfig& container_config);
    
    // Responsive layout adjustments
    static void apply_responsive_layout(Entity& root, float screen_width);
};

} // namespace afterhours::ui::autolayout
```

### 6. Simplified Component Creation

With the new system, component creation becomes much simpler:

```cpp
// Before: Manual configuration
imm::button(context, mk(parent), 
    ComponentConfig{}
        .with_size(ComponentSize{screen_pct(0.15f), screen_pct(0.07f)})
        .with_padding(Padding{screen_pct(0.01f), screen_pct(0.01f), 
                              screen_pct(0.01f), screen_pct(0.01f)})
        .with_label("Click me"));

// After: Automatic theme inheritance
imm::button(context, mk(parent), 
    ComponentConfig{}
        .with_label("Click me")); // Size, padding, etc. inherited from theme

// Or with minimal overrides
imm::button(context, mk(parent), 
    ComponentConfig{}
        .with_label("Click me")
        .with_auto_size(AutoSize::Content, AutoSize::Content)); // Auto-size based on content
```

## Implementation Plan

### Phase 1: Unified Theme System
1. Merge `Theme`, `ThemeManager`, and `UIStylingDefaults` into single `Theme` class
2. Modify `ComponentConfig` to support theme inheritance
3. Update existing `UIStylingDefaults::get()` calls to use `Theme::get()`
4. Implement fluent API for theme configuration

### Phase 2: Auto-layout Integration
1. Enhance autolayout system with smart sizing
2. Implement theme-based default sizing
3. Add auto-sizing algorithms for different component types

### Phase 3: Advanced Features
1. Theme variants and switching
2. Performance optimizations
3. Documentation and examples
4. Migration tools for existing code

## Benefits

1. **Eliminates manual sizing**: No more hardcoded pixel values in game code
2. **Consistent UI**: All components automatically follow the same design language
3. **Easy theming**: Switch between light/dark/compact themes with one line
4. **Responsive design**: Automatic adaptation to different screen sizes
5. **Maintainable**: Centralized theme configuration
6. **Flexible**: Easy to override specific components when needed

## Migration Path

1. **Immediate**: Merge existing classes into unified `Theme` class
2. **Short term**: Update kart code to use `Theme::set_default_theme()`
3. **Medium term**: Remove manual component configuration from kart code
4. **Long term**: Remove old `UIStylingDefaults` API entirely

## Example Usage

```cpp
// In kart main.cpp or initialization
void initialize_ui_theme() {
    // Set a comprehensive default theme
    afterhours::ui::Theme::set_default_theme(
        afterhours::ui::Theme::compact()
            .set_component_default(ComponentType::Button, 
                ComponentConfig{}.with_size(screen_pct(0.15f), screen_pct(0.07f)))
            .set_component_default(ComponentType::Slider,
                ComponentConfig{}.with_size(screen_pct(0.15f), screen_pct(0.07f)))
            .set_component_default(ComponentType::Checkbox,
                ComponentConfig{}.with_size(screen_pct(0.15f), screen_pct(0.07f)))
    );
}

// In UI creation code - no sizing needed!
void create_main_menu() {
    auto button = imm::button(context, mk(parent), 
        ComponentConfig{}.with_label("Start Game"));
    
    auto slider = imm::slider(context, mk(parent), 
        ComponentConfig{}.with_label("Volume"));
    
    // All components automatically sized and styled according to theme
    
    // Only override what you need:
    auto custom_button = imm::button(context, mk(parent), 
        ComponentConfig{}
            .with_label("Custom Size")
            .with_size(screen_pct(0.25f), screen_pct(0.1f))); // Override size, inherit everything else
}
```

## ComponentConfig Integration Decision

**Why not merge ComponentConfig with Theme?**

After careful analysis, keeping `ComponentConfig` separate from `Theme` is the right architectural choice:

### Simple Inheritance by Default

**The clean approach:**
```cpp
// Everything inherits from theme by default
ComponentConfig config;
config.with_label("Click me"); // Only set what you want to override

// If you want to override size:
config.with_size(screen_pct(0.2f), screen_pct(0.1f));

// If you want to override padding:
config.with_padding(Padding{screen_pct(0.02f)});

// Everything else (margin, font, colors) automatically comes from theme
```

**Benefits of this approach:**
1. **Simplest possible**: No inheritance flags or complex configuration
2. **Least error-prone**: Can't accidentally disable inheritance
3. **Most readable**: Intent is clear - set what you want, inherit the rest
4. **Most maintainable**: Less code to understand and debug
5. **Follows principle of least surprise**: Default behavior is what most users want

1. **Separation of Concerns**: 
   - `Theme` = Global defaults and styling rules
   - `ComponentConfig` = Per-instance configuration and overrides

2. **Performance**: 
   - `ComponentConfig` is copied frequently during component creation
   - `Theme` is read-only and shared across all components

3. **Flexibility**: 
   - Components can selectively inherit from theme (size but not padding, etc.)
   - Easy to override specific properties without affecting others

4. **Builder Pattern**: 
   - `ComponentConfig`'s fluent interface is valuable for component creation
   - Theme inheritance happens automatically in the background

**The integration happens at the usage level:**
```cpp
// ComponentConfig automatically inherits from Theme
auto button = imm::button(context, mk(parent), 
    ComponentConfig{}.with_label("Click me")); // Size, padding inherited from theme

// Easy to override specific properties
auto custom_button = imm::button(context, mk(parent), 
    ComponentConfig{}
        .with_label("Custom")
        .with_size(screen_pct(0.2f), screen_pct(0.1f)) // Override theme size
        .inherit_only_size()); // Still inherit padding, colors, etc. from theme
```

## Conclusion

This unified theming system will transform the afterhours UI library from a low-level component system into a high-level, theme-driven UI framework. By merging `Theme`, `ThemeManager`, and `UIStylingDefaults` into a single class while keeping `ComponentConfig` separate for flexibility, we get the best of both worlds: centralized theme management and per-component customization.

The result is cleaner, more maintainable game code that focuses on functionality rather than UI minutiae, while preserving the flexibility to override specific components when needed.