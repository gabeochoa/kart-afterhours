# Clean Up Non-Immediate UI Code

## Goal
Remove all non-immediate rendering code from `vendor/afterhours/src/plugins/ui/` to prepare for implementing automatic beautiful layout defaults. Keep only code that's part of the immediate mode rendering pipeline.

## Files to Remove

### 1. `vendor/afterhours/src/plugins/ui/providers.h`
- Contains `ProviderConsumer` and `HasDropdownStateWithProvider` patterns
- Appears to be a retained-mode pattern (not immediate mode)
- Not used anywhere in the codebase (only defined, never referenced)
- This is legacy code that doesn't fit the immediate mode architecture

### 2. `vendor/afterhours/src/plugins/ui/behaviors/` directory
- Empty directory with no files
- Can be safely removed

## Files to Update

### 3. `vendor/afterhours/src/plugins/ui.h`
- Remove the `#include "ui/providers.h"` line
- This is the backward compatibility header that includes all UI files
- Since `providers.h` is being removed, this include should be removed

### 4. Move files from `immediate/` to `ui/` directory
- Move all 5 files from `vendor/afterhours/src/plugins/ui/immediate/` directly into `vendor/afterhours/src/plugins/ui/`:
  - `component_config.h`
  - `element_result.h`
  - `entity_management.h`
  - `imm_components.h`
  - `rounded_corners.h`
- Update `immediate.h` to include files from new location (remove `immediate/` prefix from includes)
- Update any internal includes within these files if they reference each other (change `immediate/` paths to just the filename)
- Keep the `imm` namespace in all moved files (will be removed in a future update)
- Remove the now-empty `immediate/` directory

## Verification

After removal:
- Verify the codebase still compiles
- Check that no code references `ProviderConsumer`, `HasDropdownStateWithProvider`, or `make_dropdown`
- Ensure immediate mode API (`div()`, `button()`, etc.) still works correctly

## Immediate Mode Code (Keep)

These files are part of the immediate mode system and should be kept:
- Files moved from `immediate/` directory - immediate mode API (`div()`, `button()`, etc.)
- `rendering.h` - `RenderImm` system and helper functions
- `systems.h` - interaction handling systems
- `context.h` - `UIContext` for immediate mode
- `utilities.h` - system registration functions
- `components.h` - component definitions
- `theme.h`, `theme_defaults.h` - theming system
- `text_utils.h` - text utilities
- `animation_keys.h` - animation keys
