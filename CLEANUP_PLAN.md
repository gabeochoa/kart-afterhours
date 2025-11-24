# Code Cleanup After Plugin Migrations

## Overview

After reviewing the last 10 commits (plugin migrations for translation, camera, sound, collision systems) and the vendor/afterhours submodule, several cleanup opportunities were identified to simplify code and reduce verbosity.

## Issues Found

### 1. Overly Complex Workaround in `sound_library.cpp`

**File**: `src/sound_library.cpp`

- Lines 5-22: Unnecessary template-based workaround with anonymous namespace and `sound_library_internal` namespace
- The `get_sound_resource_path_impl` template function and wrapper namespace can be removed
- Can directly use `afterhours::files::get_resource_path` instead

**Current code**:
```cpp
namespace {
  template<int = 0>
  std::filesystem::path get_sound_resource_path_impl(const std::string& group, const std::string& name) {
    struct FilesAccessor {
      typedef ::afterhours::files FilesType;
      static std::filesystem::path get_path(const std::string& g, const std::string& n) {
        return FilesType::get_resource_path(g, n);
      }
    };
    return FilesAccessor::get_path(group, name);
  }
}

namespace sound_library_internal {
  std::filesystem::path get_sound_resource_path(const std::string& group, const std::string& name) {
    return get_sound_resource_path_impl(group, name);
  }
}
```

**Fix**: Replace all `sound_library_internal::get_sound_resource_path` calls with direct `afterhours::files::get_resource_path` calls.

### 2. Redundant JSON Wrapper Functions in `settings.cpp`

**File**: `src/settings.cpp`

- Lines 13-47: Anonymous namespace with `to_json_impl`/`from_json_impl` functions
- Lines 49-66: Public `to_json`/`from_json` functions that just call the impl versions
- This indirection is unnecessary - can inline the impl functions directly into the public functions

**Current code**: The impl functions are in an anonymous namespace and the public functions just call them. This adds unnecessary indirection.

**Fix**: Remove the anonymous namespace and impl functions, move their logic directly into the public `to_json`/`from_json` functions.

### 3. Code Duplication in `sound_systems.cpp`

**File**: `src/sound_systems.cpp`

- Lines 42-66: `UIClickSounds::for_each_with` contains logic for handling clicks
- Lines 69-108: `UIClickSounds::process_derived_children` duplicates the same logic
- The duplication can be eliminated by extracting the common logic into a helper function

**Current code**: The click handling logic (checking `was_rendered_to_screen`, `is_menu_active()`, querying for `SoundEmitter`, creating `PlaySoundRequest`) is duplicated in both methods.

**Fix**: Extract the click sound logic into a private helper method that both functions can call.

### 4. Out-of-Place Global Variable Declarations

**File**: `src/preload.cpp`

- Lines 216-217: Global variable declarations at the end of the file seem misplaced
- These appear to be plugin-internal variables that shouldn't be in the game code

**Current code**:
```cpp
std::shared_ptr<afterhours::sound_system::SoundLibrary> afterhours::sound_system::SoundLibrary_single;
std::shared_ptr<afterhours::sound_system::MusicLibrary> afterhours::sound_system::MusicLibrary_single;
```

**Fix**: Verify if these are needed. If they're plugin internals, they should be in the plugin, not the game code. If they're needed for some reason, add a comment explaining why.

### 5. Reduce Namespace Verbosity in `.cpp` Files

**Files**: Many `.cpp` files use fully qualified `afterhours::` or `::afterhours::` names
- The user prefers to avoid `afterhours::` everywhere when possible
- Using `using namespace afterhours` in `.cpp` files is acceptable and reduces verbosity
- Files that could benefit: `settings.cpp`, `sound_library.cpp`, `sound_systems.cpp`, and others that use many afterhours types

**Fix**: Add `using namespace afterhours;` at the top of `.cpp` files and remove unnecessary `afterhours::` prefixes where it doesn't cause ambiguity.

### 6. New Plugins Available in afterhours Submodule

**Review of vendor/afterhours commits shows new plugins:**

**Timer Plugin** (`timer.h`):
- Provides `HasTimer` component for cooldowns/timers with auto-reset, pause/resume
- Game currently has manual cooldown logic:
  - `Weapon` class has `cooldown` and `cooldownReset` fields with manual `pass_time()` method
  - `WeaponCooldownSystem` manually decrements cooldowns
  - Tag cooldowns in `RoundTagAndGoSettings` use manual time tracking
  - Boost cooldowns in `AIBoostCooldown` component
- **Migration opportunity**: Could replace manual cooldown logic with `HasTimer` component
- **Status**: Optional - larger refactoring, can be deferred

**Pathfinding Plugin** (`pathfinding.h`):
- Provides A* and BFS pathfinding with async support
- Game has TODO comment in `systems_ai.h` line 15: "TODO feels like we will need pathfinding at some point"
- AI currently uses simple target selection without pathfinding
- **Status**: Not currently needed, but available for future use

## Implementation Plan

### Priority 1: Code Simplification (Focus Here)

1. **Simplify `sound_library.cpp`**: 
   - Remove template workaround
   - Use `afterhours::files::get_resource_path` directly
   - Add `using namespace afterhours` to reduce verbosity

2. **Simplify `settings.cpp`**: 
   - Remove redundant wrapper functions
   - Inline impl logic
   - Add `using namespace afterhours` to reduce verbosity

3. **Refactor `sound_systems.cpp`**: 
   - Extract duplicate click handling logic into helper
   - Ensure `using namespace afterhours` is used consistently

4. **Review `preload.cpp`**: 
   - Verify and potentially relocate/remove global variable declarations

5. **Reduce namespace verbosity**: 
   - Add `using namespace afterhours` to `.cpp` files that heavily use afterhours types
   - Remove unnecessary `afterhours::` prefixes where it doesn't cause ambiguity

### Priority 2: Plugin Migrations (Optional - Can Defer)

6. **Timer plugin migration** (Optional): 
   - Consider migrating weapon cooldowns to use `afterhours::timer::HasTimer` component
   - This would replace existing manual cooldown code in `Weapon` class and `WeaponCooldownSystem`
   - Larger refactoring - can be done later

### Future/Not Needed Yet

7. **Pathfinding plugin**: 
   - Available but not needed
   - Game has TODO comment but no current pathfinding implementation
   - Keep in mind for future if AI pathfinding is added

## Testing

After each change:
- Verify the code compiles
- Ensure functionality remains unchanged
- Check that no new warnings are introduced

## Files to Modify

### Priority 1 Files:
- `src/sound_library.cpp` - Simplify workaround, add namespace
- `src/settings.cpp` - Remove redundant wrappers, add namespace
- `src/sound_systems.cpp` - Extract duplicate logic, add namespace
- `src/preload.cpp` - Review global variables
- Other `.cpp` files with heavy `afterhours::` usage

### Priority 2 Files (Optional):
- `src/weapons.h` - Consider timer plugin migration
- `src/systems.h` - Consider timer plugin migration for `WeaponCooldownSystem`

