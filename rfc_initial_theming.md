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

### 1. Theme Structure

Create a comprehensive `Theme` struct that includes both colors and sizing information:

```cpp
struct Theme {
    // Colors (existing)
    Color font;
    Color darkfont;
    Color background;
    Color primary;
    Color secondary;
    Color accent;
    Color error;
    
    // New: Component sizing defaults
    ComponentSize button_size;
    ComponentSize slider_size;
    ComponentSize checkbox_size;
    ComponentSize dropdown_size;
    ComponentSize navigation_bar_size;
    
    // New: Spacing and typography
    Padding default_padding;
    Margin default_margin;
    float default_font_size;
    std::string default_font_name;
    
    // New: Layout defaults
    float default_component_spacing;
    float default_section_spacing;
    
    // New: Responsive breakpoints
    std::vector<float> breakpoints; // screen widths where theme changes
    
    // New: Theme variants
    std::string name;
    std::string variant; // "light", "dark", "compact", "spacious"
    
    // Constructor with sensible defaults
    Theme();
    
    // Factory methods for common themes
    static Theme light();
    static Theme dark();
    static Theme compact();
    static Theme spacious();
};
```

### 2. Global Theme Management

Add a global theme manager to the afterhours UI library:

```cpp
namespace afterhours::ui {

class ThemeManager {
public:
    static ThemeManager& get();
    
    // Set the default theme for the entire application
    void set_default_theme(const Theme& theme);
    
    // Get the current default theme
    const Theme& get_default_theme() const;
    
    // Set theme for specific component types
    void set_component_theme(ComponentType type, const ComponentConfig& config);
    
    // Get merged component config with theme defaults
    ComponentConfig get_component_config(ComponentType type, 
                                       const ComponentConfig& overrides = {}) const;
    
    // Theme switching
    void switch_theme(const std::string& theme_name);
    void register_theme(const std::string& name, const Theme& theme);
    
private:
    Theme default_theme;
    std::map<std::string, Theme> registered_themes;
    std::map<ComponentType, ComponentConfig> component_overrides;
};

} // namespace afterhours::ui
```

### 3. New API for Setting Default Theme

Replace the current manual configuration with a simple theme setting:

```cpp
// In kart code, replace SetupGameStylingDefaults with:
void setup_default_theme() {
    auto& theme_manager = afterhours::ui::ThemeManager::get();
    
    // Option 1: Use a predefined theme
    theme_manager.set_default_theme(afterhours::ui::Theme::compact());
    
    // Option 2: Create a custom theme
    afterhours::ui::Theme kart_theme;
    kart_theme.button_size = ComponentSize{screen_pct(0.15f), screen_pct(0.07f)};
    kart_theme.slider_size = ComponentSize{screen_pct(0.15f), screen_pct(0.07f)};
    kart_theme.default_padding = Padding{screen_pct(0.01f), screen_pct(0.01f), 
                                        screen_pct(0.01f), screen_pct(0.01f)};
    kart_theme.default_font_size = 24.0f;
    
    theme_manager.set_default_theme(kart_theme);
}
```

### 4. Enhanced ComponentConfig with Theme Inheritance

Modify `ComponentConfig` to automatically inherit from theme defaults:

```cpp
struct ComponentConfig {
    // Existing fields...
    
    // New: Theme inheritance control
    bool inherit_from_theme = true;
    bool inherit_size = true;
    bool inherit_padding = true;
    bool inherit_margin = true;
    bool inherit_font = true;
    
    // New: Responsive sizing
    std::map<float, ComponentSize> responsive_sizes; // breakpoint -> size
    
    // New: Auto-sizing hints
    enum class AutoSize {
        None,
        Content,        // Size based on content
        Parent,         // Size based on parent container
        Screen,         // Size based on screen dimensions
        Adaptive        // Smart sizing based on context
    };
    
    AutoSize auto_size_x = AutoSize::None;
    AutoSize auto_size_y = AutoSize::None;
    
    // Enhanced builder methods
    ComponentConfig& with_auto_size(AutoSize x, AutoSize y);
    ComponentConfig& with_responsive_size(float breakpoint, const ComponentSize& size);
    ComponentConfig& without_theme_inheritance();
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

### Phase 1: Core Theme System
1. Implement `Theme` struct with sizing defaults
2. Create `ThemeManager` singleton
3. Modify `ComponentConfig` to support theme inheritance
4. Update `UIStylingDefaults` to use the new theme system

### Phase 2: Auto-layout Integration
1. Enhance autolayout system with smart sizing
2. Implement responsive breakpoints
3. Add auto-sizing algorithms for different component types

### Phase 3: Advanced Features
1. Theme variants and switching
2. Responsive design support
3. Performance optimizations
4. Documentation and examples

## Benefits

1. **Eliminates manual sizing**: No more hardcoded pixel values in game code
2. **Consistent UI**: All components automatically follow the same design language
3. **Easy theming**: Switch between light/dark/compact themes with one line
4. **Responsive design**: Automatic adaptation to different screen sizes
5. **Maintainable**: Centralized theme configuration
6. **Flexible**: Easy to override specific components when needed

## Migration Path

1. **Immediate**: Add new theme system alongside existing `UIStylingDefaults`
2. **Short term**: Update kart code to use `ui::set_default_theme()`
3. **Medium term**: Remove manual component configuration from kart code
4. **Long term**: Deprecate old `UIStylingDefaults` API

## Example Usage

```cpp
// In kart main.cpp or initialization
void initialize_ui_theme() {
    // Set a comprehensive default theme
    afterhours::ui::ThemeManager::get().set_default_theme(
        afterhours::ui::Theme::compact()
            .with_button_size(screen_pct(0.15f), screen_pct(0.07f))
            .with_default_padding(screen_pct(0.01f))
            .with_default_font_size(24.0f)
    );
}

// In UI creation code - no sizing needed!
void create_main_menu() {
    auto button = imm::button(context, mk(parent), 
        ComponentConfig{}.with_label("Start Game"));
    
    auto slider = imm::slider(context, mk(parent), 
        ComponentConfig{}.with_label("Volume"));
    
    // All components automatically sized and styled according to theme
}
```

## Conclusion

This theming system will transform the afterhours UI library from a low-level component system into a high-level, theme-driven UI framework. It eliminates the need for manual sizing while providing flexibility for custom designs. The result is cleaner, more maintainable game code that focuses on functionality rather than UI minutiae.