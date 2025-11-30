# Automatic Beautiful Layout Defaults - Implementation Plan

## Goal
Make layouts automatically look beautiful by default without requiring manual styling of every component. Apply design principles (8pt grid, typography scale, spacing rhythm) automatically while allowing selective overrides.

## Design Principles to Implement

1. **8pt Grid System**: All spacing in multiples of 8px
2. **Typography Scale**: Mathematical font size relationships
3. **Spacing Rhythm**: Vertical rhythm based on line height
4. **Visual Hierarchy**: Automatic size/spacing based on element type
5. **Proportional Relationships**: Golden ratio and proportional spacing
6. **Consistent Alignment**: Grid snapping and consistent alignment

## Files to Modify

### 1. `vendor/afterhours/src/plugins/ui/immediate/component_config.h`
- Add default spacing system (8pt grid)
- Add typography scale system
- Add automatic default application
- Add smart merging (manual overrides merge with defaults, don't replace)

### 2. `vendor/afterhours/src/plugins/autolayout.h` (minimal changes)
- Snap computed positions to 8pt grid
- Ensure spacing respects vertical rhythm

## Implementation Approach

### Default Spacing System
Add to `ComponentConfig`:
```cpp
struct DefaultSpacing {
  static float tiny() { return 8.0f; }    // 1 unit
  static float small() { return 16.0f; }   // 2 units
  static float medium() { return 24.0f; }  // 3 units
  static float large() { return 32.0f; }  // 4 units
  static float xlarge() { return 48.0f; } // 6 units
};
```

### Automatic Default Application
Modify `ComponentConfig` constructor or add `apply_defaults()`:
- Automatically apply spacing defaults based on element type
- Apply typography scale defaults
- Apply visual hierarchy defaults

### Smart Merging
Modify `ComponentConfig::apply_overrides()` or create `merge_with_defaults()`:
- Manual values override defaults
- Unspecified values use defaults
- Preserves current explicit override behavior when values are set

### Layout Presets
Add helper functions for common beautiful layouts:
- `magazine_style()` - Generous spacing, typography scale
- `card_style()` - Rounded corners, padding, margin
- `form_style()` - Consistent field spacing
- `auto_spacing()` - Context-aware spacing

## Minimal Code Changes

### ComponentConfig Changes
1. Add static default spacing constants
2. Add `apply_automatic_defaults()` method (called automatically)
3. Enhance `apply_overrides()` to merge intelligently
4. Add preset helper methods

### Autolayout Changes (minimal)
1. Add grid snapping to `compute_rect_bounds()` or positioning
2. Ensure spacing calculations respect 8pt grid

## Default Behavior

### Automatic Defaults Applied
- **Spacing**: Medium padding (24px), small margin (16px) by default
- **Typography**: Base size 16px, line height 1.5x
- **Visual Hierarchy**: Larger elements get more spacing
- **Alignment**: Snap to 8pt grid

### Manual Overrides
- Setting `.with_padding()` overrides default padding
- Setting `.with_font_size()` overrides default font size
- Unset values use defaults automatically

## Example Usage

### Before (manual styling everywhere):
```cpp
div(context, mk(entity, 0),
    ComponentConfig{}
        .with_padding(Padding{24, 24, 24, 24})
        .with_margin(Margin{16, 16, 16, 16})
        .with_font_size(16.0f)
        .with_label("Text"));
```

### After (automatic beautiful defaults):
```cpp
div(context, mk(entity, 0),
    ComponentConfig{}
        .with_label("Text")); // Automatically gets beautiful spacing/typography
```

### Or with selective overrides:
```cpp
div(context, mk(entity, 0),
    ComponentConfig{}
        .with_label("Text")
        .with_padding(DefaultSpacing::large())); // Only override padding, rest uses defaults
```

## Implementation Steps

### Step 1: Add Default Spacing System
- Add `DefaultSpacing` struct with 8pt grid constants to `component_config.h`
- Functions: `tiny()`, `small()`, `medium()`, `large()`, `xlarge()`, `container()`

### Step 2: Add Typography Scale
- Add `TypographyScale` struct with mathematical font size relationships
- Base size: 16px
- Ratio: 1.25 (Major Third scale)
- Functions: `size(level)` for different heading levels

### Step 3: Add Automatic Default Application
- Add `apply_automatic_defaults()` method to `ComponentConfig`
- Called automatically when component is created
- Applies spacing/typography defaults based on element type
- Respects element hierarchy (headings get more spacing, etc.)

### Step 4: Enhance Merging Logic
- Modify `ComponentConfig::apply_overrides()` or create `merge_with_defaults()`
- Manual values override defaults
- Unspecified values use defaults
- Preserves current behavior when values are explicitly set

### Step 5: Add Layout Presets
- Add preset helper methods:
  - `magazine_style()` - Generous spacing, typography scale
  - `card_style()` - Rounded corners, padding, margin
  - `form_style()` - Consistent field spacing
  - `auto_spacing()` - Context-aware spacing

### Step 6: Add Grid Snapping (Minimal)
- Add 8pt grid snapping to autolayout positioning
- Modify `compute_rect_bounds()` or positioning functions
- Snap computed positions to nearest 8px multiple

## Testing

Add simple verification tests:
- Default spacing is applied automatically
- Manual overrides work correctly
- Merging preserves unspecified defaults
- Grid snapping works

## Key Principle

**Minimal Changes, Maximum Impact**: 
- Add defaults and merging logic
- Don't change existing behavior when values are explicitly set
- Make unspecified values beautiful by default
- Keep builder pattern exactly as is

## Design Principles Reference

From research:

1. **8pt Grid System**: All spacing multiples of 8px (8, 16, 24, 32, 40, 48, etc.)
2. **Generous White Space**: More space usually looks better
3. **Consistent Rhythm**: Vertical spacing follows typographic scale
4. **Visual Hierarchy**: Size and spacing create importance
5. **Proportional Relationships**: Use ratios (golden ratio, etc.)
6. **Alignment**: Snap to grid, consistent alignment
7. **Typography Scale**: Mathematical font size relationships
8. **Context-Aware Spacing**: Different spacing for different relationships

## References

- See `UI_LIBRARY_RESEARCH.md` section "Making Automatic Layouts Beautiful" for detailed design principles
- 8pt Grid System: https://spec.fm/specifics/8-pt-grid
- Typography Scale: https://type-scale.com/
- Material Design Spacing: https://material.io/design/layout/spacing-methods.html

