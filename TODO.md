# TODO Documentation for Cart Chaos Game

## Summary Statistics
- **Total TODOs**: 36 items
- **🔴 High Priority**: 8 items (bugs, safety, core functionality)
- **🟡 Medium Priority**: 16 items (performance, architecture improvements)
- **🟢 Low Priority**: 12 items (features, enhancements)

## Priority Breakdown

### 🔴 High Priority (Critical/Safety/Core Bugs)
- Sound system architecture needs component conversion (prevents multiple sounds)
- Settings persistence not loading last used configuration
- Master volume logic may not properly cascade to other volumes
- AI collision detection and pathfinding missing
- Tag cooldown system needs audit for game balance
- Shield visual feedback missing for tag immunity

### 🟡 Medium Priority (Performance/Architecture)
- Music library inefficient looping over sounds
- Global render texture needs proper library organization
- Logging system lacks file output and color customization
- Resource cleanup system incomplete
- Entity color system missing for visual feedback
- Weapon firing direction logic incomplete

### 🟢 Low Priority (Features/Enhancements)
- Victory screen and game continuation options
- Enhanced visual effects (particles, animations)
- UI improvements (icons, layout)
- Game rule options (tag backs, announcements)
- Procedural content generation (obstacles)

---

## Core System TODOs

### 🔴 Sound System Architecture
**File**: `src/sound_systems.h:8`
**Issue**: Sound can only be played once in raylib, needs component-based aliases
```cpp
// TODO this needs to be converted into a component with SoundAlias's
// in raylib a Sound can only be played once and hitting play again
// will restart it.
// we need to make aliases so that each one can play at the same time
```
**Impact**: Multiple simultaneous sounds impossible, affects gameplay immersion

### 🟡 Music Library Performance  
**File**: `src/music_library.h:21`
**Issue**: Inefficient sound iteration
```cpp
// TODO SPEED: right now we loop over every sound again
```
**Impact**: Performance degradation with many sounds

### 🔴 Settings Persistence
**File**: `src/settings.cpp:85`
**Issue**: Settings don't load previous configuration on startup
```cpp
// TODO load last used settings
```
**Impact**: Poor user experience, settings reset every session

### 🔴 Master Volume Logic
**File**: `src/settings.cpp:115`
**Issue**: Unclear volume update dependency chain
```cpp
// TODO should these be updated after?
// because if not, then why are we doing these at all
```
**Impact**: Audio system may not work correctly

### 🟡 Logging System
**File**: `src/log/log.h`
- **Line 21**: `// TODO allow people to specify their own colors`
- **Line 33**: `// TODO log to file`
**Impact**: Limited debugging capabilities, no persistent logs

### 🟡 Global Render Texture Organization
**File**: `src/main.cpp:20`
**Issue**: Render texture not properly organized in library
```cpp
// TODO move into library or somethign
raylib::RenderTexture2D mainRT;
```
**Impact**: Poor code organization, potential memory management issues

---

## Game Mechanics TODOs

### 🔴 AI System
**File**: `src/systems.h`
- **Line 678**: `// TODO feels like we will need pathfinding at some point`
- **Line 685**: `// TODO make the ai have a difficulty slider`
**Impact**: AI opponents lack sophisticated behavior and customization

### 🔴 Tag System (Cat & Mouse Game Mode)
**File**: `src/systems.h:1317-1320`
**Issue**: Missing core game feedback systems
```cpp
// TODO: Add sound effect when cat tags mouse
// TODO: Add particle effect or visual feedback when tag occurs
// TODO: Consider different collision detection (maybe just front of car hits back of car)
```
**Impact**: Poor player feedback, unclear game state

### 🔴 Tag Cooldown System
**File**: `src/round_settings.h:66-68`
**Issue**: Game balance needs verification
```cpp
// TODO: audit cooldown time setting
// TODO: Add "tag back" rule option (allow/disallow tag backs)
// TODO: Add UI announcement of who is the current cat on game start
```
**Impact**: Game balance issues, unclear rules

### 🔴 Shield Visual Feedback
**File**: `src/systems.h:1224-1225`
**Issue**: Tag immunity not clearly communicated
```cpp
// TODO: Add pulsing animation to shield to make it more obvious
// TODO: Add countdown timer above shield showing remaining safe time
```
**Impact**: Players unaware of protection status

### 🟡 Victory System
**File**: `src/systems.h:1474-1475`
**Issue**: Game completion experience incomplete
```cpp
// TODO: Add victory screen showing final mouse times for all players
// TODO: Add option to continue playing (best of 3, etc.)
```
**Impact**: Abrupt game ending, no replay value

---

## Weapon System TODOs

### 🟡 Weapon Effects
**File**: `src/weapons.h`
- **Line 119**: `// TODO more poofs` (shotgun visual effects)
- **Line 191**: `// TODO add warning` (rocket launcher)
- **Line 198**: `// TODO add warning` (rocket launcher)
**Impact**: Weapons lack visual polish and player warnings

### 🟡 Weapon Direction Logic
**File**: `src/makers.cpp:33-52`
**Issue**: Incomplete firing direction implementation
```cpp
case Weapon::FiringDirection::Forward:
  // TODO
  off = {0.f, 0.f};
  angle = 0.f;
  break;
// TODO (another case)
```
**Impact**: Weapon aiming may not work correctly in all directions

### 🟡 Weapon Enum Utilities
**File**: `src/weapons.h:209`
**Issue**: Missing enum automation
```cpp
// TODO add a macro that can do these for enums we love
```
**Impact**: Manual enum maintenance, potential for errors

---

## UI System TODOs

### 🟡 UI Layout and Visualization
**File**: `src/ui_systems.cpp`
- **Line 193**: `// TODO the top left buttons should be have some top/left padding`
- **Line 551**: `// TODO: Add score tracking component`
- **Line 737**: `* TODO how to visualize this smaller? icons?`
- **Line 848**: `// TODO replace with actual strings`
**Impact**: UI lacks polish and proper spacing/labels

### 🟡 Settings UI
**File**: `src/round_settings.h:93,130`
- **Line 93**: `// TODO: Add tag cooldown setting to settings UI`
- **Line 130**: `// TODO Load previous settings from file`
**Impact**: Game configuration options not exposed to players

---

## Resource Management TODOs

### 🟡 Resource System
**File**: `src/resources.h`
- **Line 33**: `// TODO replace with struct?`
- **Line 39**: `// TODO add a full cleanup to write folders in case we need to reset`
**Impact**: Resource management not optimal, potential memory leaks

### 🟡 File Path Safety
**File**: `src/preload.cpp`
- **Line 70**: `// TODO how safe is the path combination here esp for mac vs windows`
- **Line 77**: `// TODO how safe is the path combination here esp for mac vs windows`
**Impact**: Cross-platform compatibility concerns

### 🟡 Preload Functionality
**File**: `src/preload.cpp:52`
**Issue**: Feature not working as expected
```cpp
// TODO this doesnt seem to do anything when in this file
```
**Impact**: Resource preloading may be ineffective

---

## Content Generation TODOs

### 🟢 Procedural Content
**File**: `src/makers.cpp:215`
**Issue**: Missing environmental variety
```cpp
// TODO create a rock or other random obstacle sprite
```
**Impact**: Maps lack variety and obstacles

### 🟢 Random Generation
**File**: `src/library.h:90`
**Issue**: Missing randomization utilities
```cpp
// TODO add random generator
```
**Impact**: Limited procedural content capabilities

---

## Visual System TODOs

### 🟡 Entity Visualization
**File**: `src/systems.h`
- **Line 153**: `// TODO add +1 here to auto gen extra players`
- **Line 1179**: `// TODO add color to entity`
**Impact**: Player identification and auto-scaling issues

### 🟡 Shader System
**File**: `src/main.cpp:108`
**Issue**: Entity-level shader support missing
```cpp
// TODO add support for entity level shaders
```
**Impact**: Limited visual effects capabilities

### 🟡 Component Configuration
**File**: `src/components.h:271`
**Issue**: Hard-coded values should be configurable
```cpp
-1.f; // TODO: Consider making this configurable per round
```
**Impact**: Gameplay parameters not tunable

---

## Notes and Documentation TODOs

### 📝 Code Documentation
**File**: `src/rl.h:37`
```cpp
// NOTE: For the linear spline we don't use subdivisions, just a single quad
```

**File**: `src/sound_library.h:60`
```cpp
// Note: Read note in MusicLibrary
```

**File**: `src/music_library.h:34-36`
```cpp
// This note is also referenced in sound library
// Note: this is set from settings
```

These notes indicate areas where cross-system coordination and documentation improvements are needed.

---

## Recommendation for Next Steps

1. **🔴 Address Critical Issues First**:
   - Fix sound system architecture for multiple simultaneous sounds
   - Implement settings persistence
   - Audit and fix master volume logic
   - Add basic AI pathfinding

2. **🟡 Improve Architecture**:
   - Reorganize global variables into proper libraries
   - Optimize music library performance
   - Add logging to file capability

3. **🟢 Enhance User Experience**:
   - Add victory screens and game continuation
   - Improve visual feedback for game states
   - Polish UI layout and add missing labels

4. **🔧 Code Quality**:
   - Add proper cross-platform file handling
   - Implement missing weapon direction logic
   - Add entity color system for better player identification