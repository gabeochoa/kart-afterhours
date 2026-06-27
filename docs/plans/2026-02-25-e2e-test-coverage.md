# E2E Test Coverage Audit

**Date:** 2026-02-25

---

## All Screens

| Screen | Exists in Tests | Screenshot | Text Assertions | Input Nav | Notes |
|--------|:-:|:-:|:-:|:-:|-------|
| Main | yes | yes | no | no | Teleports via `goto_screen`, no `expect_text` |
| Settings | yes | yes | no | partial (arrow) | Has sliders, dropdowns, checkboxes — none tested |
| About | yes | yes | no | no | Just a screenshot |
| CharacterCreation | yes | yes | no | partial (arrow) | Team mode toggle not tested |
| RoundSettings | yes | yes | no | no | Navigation bar, checkbox_group, mode-specific settings not tested |
| MapSelection | yes | yes | no | no | Map grid, preview, round settings preview not tested |
| RoundEnd | yes (01) | yes | no | no | Can only be reached after a game — no test actually navigates here with real data |
| Pause | **NO** | **NO** | **NO** | **NO** | Only visible during gameplay, never tested |
| Debug | **NO** | **NO** | **NO** | **NO** | Only visible with debug toggle, never tested |

---

## Screen Details and What Should Be Tested

### Main Menu
**Elements:** play, about, settings, exit buttons
**Current test:** screenshot only (01, 02, 05)
**Missing:**
- `expect_text` for all button labels (play, about, settings, exit)
- Tab through buttons and verify focus changes via screenshots
- Enter on play -> verify CharacterCreation screen loads
- Enter on settings -> verify Settings screen loads
- Enter on about -> verify About screen loads

### Settings
**Elements:** back button, master/music/sfx volume sliders, resolution dropdown, language dropdown, fullscreen checkbox, post processing checkbox
**Current test:** screenshot + one arrow down/right (03)
**Missing:**
- `expect_text` for labels (master volume, music volume, sfx volume, resolution, language, fullscreen, post processing)
- Tab through each control
- Interact with slider (arrow left/right to change value)
- Open resolution dropdown, select option
- Open language dropdown, select option, verify UI updates
- Toggle fullscreen checkbox
- Toggle post processing checkbox
- Back button returns to previous screen

### About
**Elements:** back button, sprite icon row
**Current test:** screenshot only (01)
**Missing:**
- `expect_text` for back button
- Verify back button navigates correctly

### Character Creation
**Elements:** round settings button, back button, team mode checkbox, player grid (individual mode), team columns (team mode), add player/AI slots
**Current test:** screenshot + one arrow down/right (04)
**Missing:**
- `expect_text` for round settings, back, team mode labels
- Toggle team mode checkbox — screenshot both states
- Tab through player slots
- Click add player / add AI
- Navigate to round settings via button

### Round Settings
**Elements:** select map button, win condition navigation_bar (Lives/Kills/Hippo/TagAndGo), weapon checkbox_group, mode-specific settings (lives count, round time, hippo count, tag settings), back button
**Current test:** screenshot only (01, 05)
**Missing:**
- `expect_text` for select map, back labels
- Switch between win conditions via navigation_bar — screenshot each
- Toggle weapons on/off
- Mode-specific: adjust lives count, round time, hippo count, tag-and-go settings
- Select map button navigates correctly

### Map Selection
**Elements:** round settings preview (win condition, weapon icons, mode params), map grid buttons with hover/focus preview, random map button ("?")
**Current test:** screenshot only (01, 05)
**Missing:**
- Tab through map buttons — verify preview changes
- Select a map
- Random map button
- Verify round settings preview shows correct info

### Round End
**Elements:** title ("round end"), continue button, quit button, player results (individual or team columns with scores)
**Current test:** `goto_screen RoundEnd` screenshot only (01)
**Missing:**
- Cannot meaningfully test without a completed game
- `expect_text` for round end, continue, quit
- Test both individual and team mode results layouts

### Pause (NOT TESTED)
**Elements:** "paused" label, resume button, back to setup button, exit game button
**Missing:**
- Requires gameplay state to trigger
- Screenshot of pause overlay
- `expect_text` for paused, resume, back to setup, exit game
- Resume returns to game
- Back to setup ends game

### Debug (NOT TESTED)
**Elements:** toggle via debug key, shows debug info overlay
**Missing:**
- Low priority — developer-only screen

---

## Summary

| Metric | Count |
|--------|-------|
| Total screens | 9 (7 menu + pause + debug) |
| Screens with any test | 7 |
| Screens with text assertions | 0 |
| Screens with input navigation test | 2 (partial) |
| Screens with no test at all | 2 (pause, debug) |
| Interactive elements never tested | sliders, dropdowns, checkboxes, navigation_bar, checkbox_group, map grid |

---

## Animation Control for Tests

### Problem
All menu screens have staggered slide-in animations (up to ~0.5s delay per element). Screenshots taken during animations capture partially-visible, partially-transparent UI — making them useless for visual regression. The wiggle system also adds scale jitter on focused elements.

### Design: `disable_animations` / `enable_animations` e2e commands

Add a global flag checked by all 3 animation layers:

```cpp
// src/ui/animation_control.h
namespace animation_control {
inline bool disabled = false;
}
```

**Layer 1 — SlideIn (`ApplyInitialSlideInMask` + `UpdateUISlideIn`):**
When disabled, `ApplyInitialSlideInMask` becomes a no-op (elements start visible at final position). `UpdateUISlideIn` sets `translate_x = 0`, `opacity = 1.0` immediately — no animation created.

**Layer 2 — UIWiggle (`UpdateUIWiggle`):**
When disabled, skip all animation, keep `scale = 1.0`.

**Layer 3 — Inline animations in `ui_systems.cpp`:**
Guard `animation::one_shot()` and `animation::anim()` calls behind the flag. When disabled, use final values directly (slide_v = 1.0, card_v = 1.0, etc.).

### E2E commands

```
disable_animations    # set flag, all UI appears instantly
enable_animations     # clear flag, animations resume normally
```

### Test structure

Most tests start with:
```
disable_animations
goto_screen Main
wait 0.1
expect_text "play"
screenshot main_menu
```

Animation-specific tests omit the disable:
```
# Tests slide-in timing and stagger
goto_screen Main
wait 0.05
screenshot slide_in_01_early
wait 0.2
screenshot slide_in_02_mid
wait 0.5
screenshot slide_in_03_complete
```

---

## Recommended Test Plan

### Phase 1: Baseline screenshots with text assertions
Rework `01_all_screens.e2e` to add `expect_text` for key labels on every screen. This catches regressions in screen rendering, text, and layout.

### Phase 2: Settings interaction tests
New `settings_interactions.e2e` — tab through controls, interact with sliders and dropdowns, take screenshots at each state.

### Phase 3: Character creation flow
New `character_creation_flow.e2e` — test team mode toggle, player slot interaction, navigation to round settings.

### Phase 4: Round settings and map selection
New `round_settings_modes.e2e` — switch between win conditions, toggle weapons, adjust mode-specific settings.
New `map_selection_flow.e2e` — tab through maps, verify preview, select map.

### Phase 5: Full navigation flow via real input
New `full_input_navigation.e2e` — start at main menu, tab to play, enter, tab through character creation, navigate to round settings, pick map, all via keyboard input (no `goto_screen` teleporting).

### Phase 6: Screenshot baselines
- Add `scripts/compare_baselines.py` (port from wm_afterhours)
- Add `screenshot-baselines/` dir with committed golden PNGs
- Add `make update-baselines` and `make validate-screenshots` targets
- Add `make ci` target

### Phase 7 (stretch): Pause screen
Requires starting a game headlessly, which may need a `start_game` e2e command.
