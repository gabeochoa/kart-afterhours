# Automatic Beautiful Layout Defaults - Implementation Plan

## Goal
Make layouts automatically look beautiful by default without requiring manual styling of every component. Apply design principles (8pt grid, typography scale, spacing rhythm) automatically while allowing selective overrides.

## Design Principles to Implement

1. **8pt Grid System**: All spacing in multiples of 8px (based on 720p)
2. **Typography Scale**: Mathematical font size relationships (based on 720p)
3. **Spacing Rhythm**: Vertical rhythm based on line height
4. **Visual Hierarchy**: Automatic size/spacing based on element type
5. **Proportional Relationships**: Golden ratio and proportional spacing
6. **Consistent Alignment**: Grid snapping and consistent alignment
7. **720p-Based Sizing**: All defaults use `screen_pct()` or `h720()`/`w1280()` for responsive scaling

## Files to Modify

### 1. `vendor/afterhours/src/plugins/ui/component_config.h`
- Add default spacing system (8pt grid)
- Add typography scale system
- Add automatic default application
- Add smart merging (manual overrides merge with defaults, don't replace)

### 2. `vendor/afterhours/src/plugins/autolayout.h` (minimal changes)
- Snap computed positions to 8pt grid
- Ensure spacing respects vertical rhythm

## Implementation Approach

### Default Spacing System (720p-Based)
Add to `ComponentConfig`:
```cpp
struct DefaultSpacing {
  // All spacing based on 720p, using screen_pct for responsive scaling
  static Size tiny() { return h720(8.0f); }    // 8px at 720p = screen_pct(8/720)
  static Size small() { return h720(16.0f); }   // 16px at 720p = screen_pct(16/720)
  static Size medium() { return h720(24.0f); }  // 24px at 720p = screen_pct(24/720)
  static Size large() { return h720(32.0f); }  // 32px at 720p = screen_pct(32/720)
  static Size xlarge() { return h720(48.0f); } // 48px at 720p = screen_pct(48/720)
  static Size container() { return h720(64.0f); } // 64px at 720p for containers
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

### Automatic Defaults Applied (720p-Based)
- **Spacing**: Medium padding (24px at 720p = `h720(24.0f)`), small margin (16px at 720p = `h720(16.0f)`) by default
- **Typography**: Base size 16px at 720p (`h720(16.0f)`), line height 1.5x
- **Accessibility**: Minimum font size `h720(18.67f)` (equivalent to 28px at 1080p) for readable text
- **Visual Hierarchy**: Larger elements get more spacing (all based on 720p)
- **Alignment**: Snap to 8pt grid (8px at 720p = `h720(8.0f)`)
- **Responsive Scaling**: All defaults automatically scale with screen resolution using `screen_pct()` or `h720()`/`w1280()`

### Manual Overrides
- Setting `.with_padding()` overrides default padding
- Setting `.with_font_size()` overrides default font size
- Unset values use defaults automatically
- **Accessibility Enforcement**: Font sizes below minimum (`h720(18.67f)`) should log warnings in non-debug builds

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

### After (automatic beautiful defaults with 720p-based responsive sizing):
```cpp
div(context, mk(entity, 0),
    ComponentConfig{}
        .with_label("Text")); 
// Automatically gets:
// - Medium padding: h720(24.0f) = 24px at 720p, scales proportionally
// - Small margin: h720(16.0f) = 16px at 720p, scales proportionally
// - Base font: h720(16.0f) = 16px at 720p, scales proportionally
```

### Or with selective overrides:
```cpp
div(context, mk(entity, 0),
    ComponentConfig{}
        .with_label("Text")
        .with_padding(Padding{
            .top = DefaultSpacing::large(),    // h720(32.0f)
            .left = DefaultSpacing::medium(),  // h720(24.0f)
            .bottom = DefaultSpacing::large(), // h720(32.0f)
            .right = DefaultSpacing::medium()  // h720(24.0f)
        })); // Only override padding, rest uses defaults
```

## Implementation Steps

### Step 1: Add Default Spacing System (720p-Based)
- Add `DefaultSpacing` struct with 8pt grid constants to `component_config.h`
- All functions return `Size` using `h720()` or `w1280()` for responsive scaling
- Functions: `tiny()`, `small()`, `medium()`, `large()`, `xlarge()`, `container()`
- All values are based on 720p resolution and scale proportionally

### Step 2: Add Typography Scale (720p-Based)
- Add `TypographyScale` struct with mathematical font size relationships
- Base size: 16px at 720p (use `h720(16.0f)` for responsive scaling)
- Ratio: 1.25 (Major Third scale)
- Functions: `size(level)` for different heading levels
- All font sizes use `h720()` or `screen_pct()` for responsive scaling
- Example: `h720(16.0f)` = 16px at 720p, scales proportionally at other resolutions
- **Accessibility**: Minimum font size is 18.67px at 720p (equivalent to 28px at 1080p)
  - Use `h720(18.67f)` or round to `h720(19.0f)` for minimum readable text
  - Body text should be at least `h720(16.0f)` (16px at 720p)
  - Small text/captions should not go below `h720(18.67f)` for accessibility

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

### Step 6: Add Grid Snapping (Minimal, 720p-Based)
- Add 8pt grid snapping to autolayout positioning
- Modify `compute_rect_bounds()` or positioning functions
- Snap computed positions to nearest 8px multiple (based on 720p)
- Grid unit: `h720(8.0f)` = 8px at 720p, scales proportionally

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
- **All defaults use 720p-based responsive sizing** (`h720()`, `w1280()`, `screen_pct()`) for proper scaling at any resolution

## Design Principles Reference

From research:

1. **8pt Grid System**: All spacing multiples of 8px at 720p (8, 16, 24, 32, 40, 48, etc.) using `h720()` for responsive scaling
2. **Generous White Space**: More space usually looks better
3. **Consistent Rhythm**: Vertical spacing follows typographic scale
4. **Visual Hierarchy**: Size and spacing create importance
5. **Proportional Relationships**: Use ratios (golden ratio, etc.)
6. **Alignment**: Snap to grid, consistent alignment
7. **Typography Scale**: Mathematical font size relationships (all based on 720p using `h720()`)
8. **Context-Aware Spacing**: Different spacing for different relationships
9. **720p-Based Responsive Design**: All defaults use `screen_pct()`, `h720()`, or `w1280()` to ensure proper scaling at any resolution
10. **Accessibility Minimums**: Minimum font size of 18.67px at 720p (28px at 1080p) for readable text, with warnings for text below this threshold

## References

- See `UI_LIBRARY_RESEARCH.md` section "Making Automatic Layouts Beautiful" for detailed design principles
- 8pt Grid System: https://spec.fm/specifics/8-pt-grid
- Typography Scale: https://type-scale.com/
- Material Design Spacing: https://material.io/design/layout/spacing-methods.html
- **Accessibility**: Minimum font size based on pharmasea implementation (28px at 1080p = 18.67px at 720p)

