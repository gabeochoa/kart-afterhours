## RFC: Proposed Extractions from kart to afterhours

### Status
- Draft

### Context
afterhours is an ECS library that also provides minimal game-engine helpers. We want to move reusable, engine-agnostic (or engine-pluggable) modules from `kart` into `afterhours` while keeping dependencies low and avoiding hard coupling to raylib, using feature gates where needed (e.g., `AFTER_HOURS_USE_RAYLIB`).

### Goals
- Reduce duplication by extracting reusable subsystems into afterhours
- Keep core dependencies small (C++20, standard library, existing vendored: `magic_enum`, `fmt`, `nlohmann/json`)
- Maintain engine-agnostic design; gate raylib-dependent code
- Provide clear migration steps to minimize churn in `kart`

### Non-Goals
- Moving game rules, round logic, or weapon systems
- Forcing raylib into afterhours core

---

## Proposed Extractions

### DON'T HAVE TO BE PLUGINS (Pure Utilities - STB-style)

#### 1) Core Resource Registry
- **Move**: `src/library.h` (`Library<T>`
  name→resource store with prefix lookup and simple random selection)
- **Why**: Generic utility for textures, sounds, music, shaders, etc.
- **Deps**: standard library only (uses `type_name.h` from afterhours)
- **API notes**:
  - Keep `add/get/contains/lookup/get_random_match/unload_all`
  - Leave TODO hook for pluggable RNG (so `get_random_match` can be wired later)
- **Target**: `afterhours/core/library.h` (STB-style)

#### 2) Filesystem/Resource Paths Helper
- **Move**: `src/resources.{h,cpp}` (`Files` resource locator)
- **Why**: Cross-project resource discovery (game folder, resource folder, settings path, directory iteration)
- **Deps**: `std::filesystem`, optional `sago/platform_folders` (already vendored)
- **Engine**: none
- **API notes**:
  - Keep `game_folder()/settings_filepath()/relative_settings()`
  - Keep `fetch_resource_path()/for_resources_in_group()/for_resources_in_folder()`
  - Make `FilesConfig` customizable at construction
- **Target**: `afterhours/core/filesystem.h` (STB-style)

#### 3) Internationalization (i18n)
- **Move**: `src/translation_manager.{h,cpp}`, `src/strings.h`
- **Why**: Generic translation manager with enum keys, descriptions, and parameterized formatting
- **Deps**: `magic_enum`, `fmt`
- **Engine/UI**: decouple hard UI ties; provide optional callbacks for font enabling (no raylib required)
- **API notes**:
  - Keep `TranslationManager`, `TranslatableString`, enum `Language`
  - Replace direct font loading with a user-provided callback or UI-agnostic policy
- **Target**: `afterhours/core/i18n.h` (STB-style)

#### 4) Settings Store (JSON-backed)
- **Move**: Generalize `src/settings.{h,cpp}` to a reusable settings system
- **Why**: Persisting resolution, audio volumes, toggles, language
- **Deps**: `nlohmann/json`, `filesystem` (via Files helper), `magic_enum` for enums
- **Engine**: none; do not depend on raylib
- **API notes**:
  - Provide a small typed settings container + load/save
  - Offer observer hooks or a `refresh()` method so games can react
  - Exclude `RoundManager`-specific parts; let projects extend via JSON blob
- **Target**: `afterhours/core/settings.h` (STB-style)

#### 6) Sound and Music Libraries (Core)
- **Move**: `src/sound_library.h`, `src/music_library.h` - just the `Library<T>` wrappers
- **Why**: Reusable sound/music resource management
- **Deps**: Gate raylib I/O behind `AFTER_HOURS_USE_RAYLIB`; core APIs stay engine-agnostic
- **Engine**: raylib path provides `LoadSound/PlaySound/LoadMusicStream`
- **API notes**:
  - Keep `Library<T>`-based wrappers
  - Keep volume management
- **Target**: `afterhours/core/sound_library.h` (STB-style)

### MUST BE PLUGINS (ECS-Integrated)

#### 7) Common Components (Engine-Agnostic)
- **Move**: Selected components from `src/components.h` that are generic:
  - `CollisionConfig`
  - `HasLabels` (+ `LabelInfo`)
  - `CanWrapAround`
  - `TeamID`
  - `ManagesAvailableColors` (color type behind alias)
- **Why**: Widely reusable without game rules
- **Deps**: standard library; abstract color/vector types via afterhours developer type aliases to avoid raylib hard dep
- **Engine**: none (use `ColorType`/`Vector2Type` style aliases)
- **Target**: `afterhours/plugins/common_components/` (Plugin)

#### 8) Transform Component
- **Move**: `Transform` concept, but refactor to avoid direct raylib types
- **Why**: Fundamental component for many systems
- **Deps**: none; rely on `Vector2Type` and a lightweight `RectType`
- **Engine**: provide adapters when `AFTER_HOURS_USE_RAYLIB` is set
- **Target**: `afterhours/plugins/transform/` (Plugin)

#### 9) Sound Playback Systems (ECS Integration)
- **Move**: Sound playback systems (`SoundPlaybackSystem`) and aliasing logic (component `SoundEmitter`, request `PlaySoundRequest`)
- **Why**: These are ECS systems and components that integrate with the sound library
- **Deps**: afterhours core sound library; ECS system
- **Engine**: none beyond sound library's raylib gate
- **API notes**:
  - Keep `Policy` modes: Name/Enum/PrefixRandom/PrefixFirstAvailable/PrefixIfNonePlaying
  - Keep aliasing via `SoundEmitter`
- **Target**: `afterhours/plugins/sound/` (Plugin)

#### 10) UI Sound Integration
- **Move**: Thin integration that maps UI input/clicks to `PlaySoundRequest` (e.g., `UISoundBindingSystem`, `UIClickSounds`)
- **Why**: Useful but optional UX enhancement
- **Deps**: afterhours UI plugin; sound plugin
- **Engine**: none beyond sound plugin’s raylib gate
- **Target**: `afterhours/plugins/ui_sound/` (Plugin)

#### 11) Shader Pass Registry and Multipass Pattern
- **Move**: `src/shader_pass_registry.h`, `src/multipass_renderer.h`, `src/multipass_integration.h` pattern (not game shaders)
- **Why**: These are ECS systems that manage rendering passes and shader components
- **Deps**: core ECS; `magic_enum`
- **Engine**: provide implementations only when `AFTER_HOURS_USE_RAYLIB` (e.g., uniform binding, Begin/End modes)
- **API notes**:
  - Keep `HasShader` as a generic component with `ShaderType` extensibility
  - Keep pass enabling/disabling and debug info helpers
- **Target**: `afterhours/plugins/rendering/` (Plugin)

#### 12) Shader Library (ECS Integration)
- **Move**: `src/shader_library.h` with refactor:
  - Keep enum→resource mapping, uniform location cache APIs
  - Make filenames/ext lookup configurable
- **Why**: This provides ECS systems for shader management and uniform updates
- **Deps**: `magic_enum`
- **Engine**: raylib shader load behind `AFTER_HOURS_USE_RAYLIB`
- **Target**: `afterhours/plugins/rendering/` (Plugin)

#### 13) Camera Component
- **Move**: `HasCamera` pattern, but abstracted to remove direct raylib symbols in the header
- **Why**: This IS a component that needs ECS integration
- **Engine**: Provide raylib adapter under gate; keep component POD
- **Target**: `afterhours/plugins/camera/` (Plugin)

---

## Items That Should Stay in kart (Game-Specific)
- Round types, `RoundManager`, weapon systems, AI behavior parameters
- Input action enum and concrete mappings for gameplay
- Map systems, gameplay HUDs, per-mode UI logic
- Game-specific shaders and shader enums (library should provide extension points only)

---

## Dependencies and Gating
- Keep afterhours core free of raylib
- For raylib-backed features, wrap with `AFTER_HOURS_USE_RAYLIB`
- Maintain current vendored deps in afterhours: `magic_enum`, `fmt`, `nlohmann/json`, `sago`

---

## Migration Plan (Incremental)
1) Introduce new targets in afterhours (folders above), compile with gates
2) Move `Library<T>` and `bitset_utils` first (no external deps)
3) Move `Files` helper; wire kart to use `afterhours::Files`
4) Move i18n and Settings; refactor kart code to new namespaces
5) Move sound libraries + playback system; keep `SoundFile` enum in kart
6) Move optional UI sound integration
7) Move rendering pass registry and gated shader library; keep game shader list in kart
8) Move common components and (optionally) refactored Transform

Each step should be buildable independently. After stabilizing, deprecate the kart copies.

---

## Risks
- Raylib type leakage into core headers; mitigate with alias types and gates
- Over-extraction of game-specific logic; mitigate by keeping enums/data in kart
- Settings schema churn; mitigate by allowing project JSON extension

---

## Code Organization Improvements

### Current State Analysis
The afterhours library currently has a mixed organization:
- **Core ECS**: Well-organized in `src/` with clear separation (`entity.h`, `system.h`, `entity_query.h`, etc.)
- **Plugins**: Located in `src/plugins/` but some are quite large (e.g., `autolayout.h` ~1400 lines, `input_system.h` ~700 lines)
- **UI System**: Nested under `plugins/ui/` with many sub-files, creating deep include paths
- **Type Aliases**: Scattered in `developer.h` with fallback definitions

### Proposed STB-Style Single-File Libraries

Following the STB library philosophy of "single-file, header-only, no dependencies," we should create several focused single-file modules:

#### 1) Core Utilities (`afterhours_utils.h`)
- **Contents**: `bitset_utils.h`, `type_name.h`, `bitwise.h`, `font_helper.h`, `drawing_helpers.h`
- **Rationale**: Small, self-contained utilities that are widely used
- **Dependencies**: Standard library only
- **Size**: ~200-300 lines total

#### 2) Animation System (`afterhours_animation.h`)
- **Contents**: Current `plugins/animation.h` (already well-contained at ~300 lines)
- **Rationale**: Complete, self-contained animation system with easing, sequencing, and watchers
- **Dependencies**: Standard library only
- **Keep as-is**: Already follows STB pattern well

#### 3) Color Utilities (`afterhours_color.h`)
- **Contents**: Current `plugins/color.h` + color manipulation functions
- **Rationale**: Color operations are universally needed, small footprint
- **Dependencies**: Standard library + optional raylib gating
- **Size**: ~100-150 lines

#### 4) Resource Management (`afterhours_resources.h`)
- **Contents**: `Library<T>` template + `Files` helper + asset loading patterns
- **Rationale**: Generic resource management is a core need
- **Dependencies**: Standard library + filesystem
- **Size**: ~200-300 lines

#### 5) Settings System (`afterhours_settings.h`)
- **Contents**: JSON-backed settings with type safety and validation
- **Rationale**: Settings are needed by most applications
- **Dependencies**: `nlohmann/json`, filesystem
- **Size**: ~150-200 lines

#### 6) Internationalization (`afterhours_i18n.h`)
- **Contents**: Translation manager with enum keys and parameterized strings
- **Rationale**: i18n is increasingly important, self-contained
- **Dependencies**: `magic_enum`, `fmt`
- **Size**: ~200-250 lines

### Plugin Restructuring

#### Current Large Plugins → Modular Approach

**Input System** (`plugins/input_system.h` ~700 lines):
- Split into: `afterhours_input_core.h` (mapping, deadzone, gamepad detection)
- Keep: `plugins/input_system.h` as integration layer with ECS systems
- **Rationale**: Core input logic is reusable, ECS integration is project-specific

**Autolayout** (`plugins/autolayout.h` ~1400 lines):
- Extract: `afterhours_layout_core.h` (dimension calculations, constraint solving)
- Keep: `plugins/autolayout.h` as ECS component integration
- **Rationale**: Layout algorithms are generic, component management is ECS-specific

**UI System** (deeply nested under `plugins/ui/`):
- Consolidate: `afterhours_ui_core.h` (components, layout, theming)
- Keep: `plugins/ui/` for ECS-specific systems and rendering
- **Rationale**: UI concepts are universal, rendering is engine-specific

### Directory Structure Proposal

```
afterhours/
├── core/                    # STB-style single files
│   ├── afterhours_utils.h
│   ├── afterhours_animation.h
│   ├── afterhours_color.h
│   ├── afterhours_resources.h
│   ├── afterhours_settings.h
│   └── afterhours_i18n.h
├── ecs/                     # Core ECS (current src/)
│   ├── entity.h
│   ├── system.h
│   ├── entity_query.h
│   └── ...
├── plugins/                 # ECS integrations
│   ├── input_system.h       # ECS input integration
│   ├── autolayout.h         # ECS layout integration
│   ├── ui/                  # ECS UI integration
│   └── ...
└── adapters/                # Engine-specific implementations
    ├── raylib/
    │   ├── afterhours_raylib.h
    │   └── ...
    └── ...
```

### Benefits of STB-Style Approach

1. **Easy Integration**: Single `#include` gets entire feature
2. **Header-Only**: No compilation units, no linking required - everything is inline
3. **Selective Use**: Include only what you need
4. **Version Control Friendly**: Single file per feature, easy to track changes
5. **Documentation**: Each file can have comprehensive docs at the top
6. **Testing**: Each file can be unit tested independently
7. **Zero Build Complexity**: No CMake/Makefile needed for the library itself

### Migration Strategy

1. **Phase 1**: Extract utilities into STB-style files
   - Start with `afterhours_utils.h` (bitset, type_name, etc.)
   - Move `animation.h` to `afterhours_animation.h`
   - Create `afterhours_color.h`

2. **Phase 2**: Extract larger systems
   - Create `afterhours_resources.h` with `Library<T>` and `Files`
   - Create `afterhours_settings.h` and `afterhours_i18n.h`

3. **Phase 3**: Refactor plugins
   - Split large plugins into core + ECS integration
   - Update includes throughout codebase

4. **Phase 4**: Reorganize directory structure
   - Move files to new structure
   - Update examples to use new include paths
   - No build system changes needed (header-only)

### Converting .cpp Files to Header-Only

Several proposed extractions currently have `.cpp` files that need to be converted to header-only:

#### 1) `resources.cpp` → `afterhours/core/filesystem.h`
**Current Issues:**
- Constructor calls `ensure_game_folder_exists()`
- Platform-specific `#ifdef __APPLE__` for sago
- Uses `log_info()` for logging

**Header-Only Solution:**
```cpp
// In afterhours/core/filesystem.h
namespace afterhours {
namespace filesystem {

// Platform detection and fallback
#ifdef __APPLE__
#include <sago/platform_folders.h>
#else
namespace sago {
inline std::string getSaveGamesFolder1() { return ""; }
} // namespace sago
#endif

// Make constructor inline and move logic to initialization
struct Files {
    std::string root = "cart_chaos";
    std::string settings_file = "settings.json";
    
    // Inline constructor - no separate .cpp needed
    Files() { 
        ensure_game_folder_exists(); 
    }
    
    // All methods inline
    inline fs::path game_folder() const {
        const fs::path master_folder(sago::getSaveGamesFolder1());
        return master_folder / fs::path(root);
    }
    
    // ... rest of methods inline
};

} // namespace filesystem
} // namespace afterhours
```

#### 2) `translation_manager.cpp` → `afterhours/core/i18n.h`
**Current Issues:**
- Contains game-specific translation strings (should be removed)
- Uses `log_warn()` for missing translations
- Complex constructor logic

**Header-Only Solution:**
```cpp
// In afterhours/core/i18n.h
namespace afterhours {
namespace i18n {

// Generic translation system - NO game-specific strings
template<typename KeyType>
class TranslationManager {
public:
    static TranslationManager& get() {
        static TranslationManager instance;
        return instance;
    }
    
    // User provides their own translation maps
    inline void set_translations(const std::map<KeyType, TranslatableString>& translations) {
        current_translations = translations;
    }
    
    inline std::string get_string(KeyType key) const {
        auto it = current_translations.find(key);
        if (it != current_translations.end()) {
            return it->second.get_text();
        }
        return "MISSING_TRANSLATION";
    }
    
    // Support for parameterized strings
    template<typename... Args>
    inline std::string get_string(KeyType key, Args&&... args) const {
        auto it = current_translations.find(key);
        if (it != current_translations.end()) {
            return fmt::format(it->second.get_text(), std::forward<Args>(args)...);
        }
        return "MISSING_TRANSLATION";
    }
    
private:
    std::map<KeyType, TranslatableString> current_translations;
};

// Generic TranslatableString class
class TranslatableString {
public:
    std::string text;
    std::string description;
    
    inline TranslatableString(const std::string& t, const std::string& d = "") 
        : text(t), description(d) {}
    
    inline const std::string& get_text() const { return text; }
    inline const std::string& get_description() const { return description; }
};

} // namespace i18n
} // namespace afterhours
```

**Usage Example:**
```cpp
// User defines their own translation keys
enum class MyGameKeys { play, settings, exit };

// User provides their own translations
std::map<MyGameKeys, afterhours::i18n::TranslatableString> my_translations = {
    {MyGameKeys::play, {"Play Game", "Start a new game"}},
    {MyGameKeys::settings, {"Settings", "Configure game options"}},
    {MyGameKeys::exit, {"Exit", "Quit the game"}}
};

// User sets their translations
afterhours::i18n::TranslationManager<MyGameKeys>::get().set_translations(my_translations);

// Use the system
std::string text = afterhours::i18n::TranslationManager<MyGameKeys>::get().get_string(MyGameKeys::play);
```

#### 3) `settings.cpp` → `afterhours/core/settings.h`
**Current Issues:**
- Complex JSON serialization/deserialization
- Uses `log_error()`, `log_info()` for logging
- Template classes and structs

**Header-Only Solution:**
```cpp
// In afterhours/core/settings.h
namespace afterhours {
namespace settings {

// Move template classes inline
template <typename T> 
struct ValueHolder {
    using value_type = T;
    T data;
    
    inline ValueHolder(const T &initial) : data(initial) {}
    inline T &get() { return data; }
    inline const T &get() const { return data; }
    inline void set(const T &data_) { data = data_; }
    inline operator const T &() { return get(); }
};

// Make all methods inline
class Settings {
public:
    static Settings& get() {
        static Settings instance;
        return instance;
    }
    
    inline bool load_save_file(int width, int height) {
        // Move all logic inline
        this->data->resolution.width = width;
        this->data->resolution.height = height;
        // ... rest of logic
        return true;
    }
    
    // ... rest of methods inline
};

} // namespace settings
} // namespace afterhours
```

### Implementation Notes

- **Header Guards**: Use `#pragma once` consistently
- **Namespace**: Keep `afterhours::` namespace for all STB files
- **Dependencies**: Minimize external dependencies in STB files
- **Documentation**: Include usage examples at the top of each file
- **Testing**: Each STB file should be testable in isolation
- **Header-Only Constraint**: All code must be inline/template-based; no separate `.cpp` files
- **Template Heavy**: Use templates and `constexpr` functions to avoid ODR violations
- **Static Storage**: Use `static` variables in functions for singletons (like current animation manager)
- **Include Order**: STB files should be self-contained and not depend on each other's include order
- **Logging**: Replace `log_*()` calls with optional logging callbacks or remove entirely
- **Platform Code**: Use `#ifdef` blocks within the header file
- **Large Data**: Move static data to inline functions to avoid ODR violations

---

## Open Questions
- Do we want a pluggable RNG service in afterhours core for `get_random_match`?
- Should the Transform component be provided by afterhours or remain per-project with adapters?
- Where to draw the line for camera: minimal component vs full system support?
- Should we maintain backward compatibility with current plugin structure during migration?
- Which STB files should be included by default in the main `ah.h` header?
