# Afterhours Engine Gaps and Feedback

Feedback and feature requests for the afterhours engine, discovered while working on kart-afterhours.

---

## Animation System

### No global disable/instant-complete flag
The animation plugin has no built-in way to disable animations or instantly complete them. This is needed for:
- E2E testing (screenshots need UI at final state, not mid-animation)
- Accessibility (reduce motion preferences)
- Development (skip animations during rapid iteration)

**Current workaround:** Game-side `animation_control::disabled` flag checked manually in every animation system and at every `animation::one_shot()` / `animation::anim()` call site. This is error-prone and requires touching every animation usage.

**Ideal:** `animation::set_instant_mode(true)` that makes all animations complete immediately (duration=0, jump to final value). This way the animation code stays identical — it still runs — but everything resolves in one frame.

### No way to clear/reset all animation tracks
When switching screens, stale animation tracks from the previous screen persist. There's no `animation::clear_all()` or per-key `animation::clear<Key>()`.

---

## E2E Testing

### No built-in `disable_animations` / `enable_animations` commands
These are common enough across projects (kart, wm_afterhours, wordproc) that they should be built-in e2e commands in the framework rather than reimplemented per-project.

### No `--screenshot-dir` CLI flag
The e2e runner saves screenshots to a hardcoded `screenshots/` path. Other projects (wordproc, wm_afterhours) pass `--screenshot-dir` to control output location, which is needed for baseline comparison workflows (`make update-baselines` vs `make validate-screenshots` writing to different dirs).

### No `--e2e-speed` CLI flag  
The wordproc runner supports `--e2e-speed` to multiply wait durations. Useful for running tests faster in CI. This should be built into the afterhours e2e runner.

### `expect_text` needs case-insensitive option
Game text is often displayed in different cases depending on styling. A case-insensitive `expect_text` variant (or a flag) would reduce false failures.

---

## UI Components

### Elements with `with_opacity(0.0f).with_translate(-2000.0f, 0.0f)` as slide-in initial state
This is a common pattern in apps using afterhours UI: create elements hidden off-screen, then animate them in. The initial hidden state is baked into ComponentConfig, which means the element is invisible if the animation system doesn't run (headless, disabled, etc).

**Suggestion:** A `with_slide_in()` config option that the layout system understands — elements start hidden and the slide-in system handles the rest, rather than requiring manual opacity/translate setup.

### No `with_disabled()` for interactive elements
There's no way to disable a button/checkbox/dropdown without removing it. A `with_disabled(bool)` that grays out and skips interaction would be useful for settings that depend on other settings (e.g., disable "select map" until a win condition is chosen).

---

## Build / Integration

### `SINGLETON_FWD` and `ENTITY_ID_GEN` fixes in latest
The latest afterhours fixes `SINGLETON_FWD` (static -> inline) and `ENTITY_ID_GEN` (static -> inline). These are important correctness fixes that affect any multi-translation-unit project. Should be called out in release notes.

### Missing `#include` for `key_codes.h`
Had to manually include `<afterhours/src/core/key_codes.h>` for e2e navigation commands. The `e2e_testing.h` header could re-export this since e2e commands commonly need key constants.

---

## Headless Mode

### `ProvidesCurrentResolution` returns (0,0) in headless mode
`window_manager::fetch_current_resolution()` calls `raylib::GetRenderWidth()`/`GetRenderHeight()` which return 0 in headless mode (no window). This causes `UIContext::screen_width/screen_height` to be 0, which makes all percentage-based layout sizes resolve to 0 and layout positions become NaN.

**Current workaround:** Register a custom update system after `window_manager::register_update_systems()` that overrides the resolution with the configured screen dimensions when zero is detected.

**Ideal:** `fetch_current_resolution()` should check `graphics::is_headless()` and return the configured render texture dimensions instead of querying a non-existent window. Alternatively, `add_singleton_components` should accept a resolution and `CollectCurrentResolution` should skip fetching in headless mode.

### `GetFontDefault()` returns an invalid font in headless mode
`init_ui_plugin` calls `get_default_font()` which uses `raylib::GetFontDefault()`. In headless mode this returns a font with missing/invalid glyph data, causing crashes in `DrawTextEx` and incorrect measurements.

**Current workaround:** After calling `init_ui_plugin`, override `DEFAULT_FONT` and `UNSET_FONT` in FontManager with fonts loaded via manual atlas generation (`LoadFontData` + `GenImageFontAtlas` + `LoadTextureFromImage`).

**Ideal:** `init_ui_plugin` should detect headless mode and load fonts via the manual atlas path automatically, or provide a hook for custom font loading.

### `std::bad_variant_access` crash on shutdown due to static destruction order
The raylib backend stores its state in a `std::variant<std::monostate, RaylibWindowed, RaylibHeadless>` inside a function-local static. When the program exits, static destructors run in undefined order across translation units. If entity destruction or other static cleanups happen after the backend variant is destroyed, `std::visit` on the dead variant throws `bad_variant_access`.

**Current workaround:** Explicitly call `EntityHelper::delete_all_entities_NO_REALLY_I_MEAN_ALL()` and then `Preload_single.reset()` (which calls `graphics::shutdown()`) before `main()` returns, ensuring deterministic teardown order.

**Ideal:** The engine should provide a `graphics::cleanup_all()` or similar that handles entity collection teardown and backend shutdown in the correct order, or the backend variant should use a sentinel state that is safe to visit after destruction.

### `SINGLETON_FWD` macro fails inside struct/class scope
`SINGLETON_FWD(Type)` expands to `inline std::shared_ptr<Type> Type_single;` which is invalid inside a struct body (needs `static inline`). This affects `sound_system.h` where `SoundLibrary` and `MusicLibrary` use `SINGLETON_FWD` inside `struct sound_system`.

**Current workaround:** Manually expand the macro with `static inline` in the vendor file.

**Ideal:** Either provide a `SINGLETON_FWD_STATIC(Type)` variant for use inside class/struct scope, or change `SINGLETON_FWD` to use `static inline` (which works at both namespace and class scope).
