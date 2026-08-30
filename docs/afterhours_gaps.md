# Afterhours Engine Gaps and Feedback

Feedback and feature requests for the afterhours engine, discovered while working on kart-afterhours.

Last re-triaged 2026-08-29 against afterhours `fc4d625`. Every entry below was
checked against the current submodule, not carried forward on trust.

**Score: 10 of 17 previously-open gaps are now closed.** All three Windows
blockers and every headless workaround but one are gone.

---

## Still open

### e2e: no `--screenshot-dir` flag
The runner still writes to a hardcoded `screenshots/` path (`src/e2e_integration.h:32`, `:65`).
This is the one gap that directly blocks a clean baseline workflow: `make update-baselines`
and `make validate-screenshots` have to write to the same directory and then `cp` out of it,
which is why neither target can safely skip a `clean-screenshots` first.

**Ideal:** `--screenshot-dir <path>`, or a settable `e2e::screenshot_directory`.

### e2e: no `--e2e-speed` flag
No way to scale wait durations. Useful for CI. wordproc's runner has one.

### `GetFontDefault()` returns an invalid font in headless mode
Still open. `backends/raylib/font_helper.h:33,88,96,107` return `raylib::GetFontDefault()`
with no `is_headless()` branch, and `plugins/ui/utilities.h:115-116` load it into
`DEFAULT_FONT` and `SYMBOL_FONT`. In headless the glyph data is missing/invalid.

**Current workaround (still in place):** after `init_ui_plugin`, override `DEFAULT_FONT`
and `UNSET_FONT` via manual atlas generation — `preload.cpp:144-209`.

**Related, still true:** CJK fonts are skipped in headless (`preload.cpp:174`), so a headless
screenshot with language set to Korean or Japanese renders with the English font.

### Checkbox internal layout overflow
`imm::checkbox` still builds a `checkbox_row` parent (`plugins/ui/imm_components.h:921`)
whose children are often taller than the row, producing
`checkbox label extends outside parent checkbox_row`. Not fixable from the caller side.

**Ideal:** `checkbox_row` auto-sizes to its children, or the children respect the parent height.

### `with_slide_in()` config option
Open as originally written, but effectively obsoleted: `Anim::on_appear().opacity().translate_x()`
covers the use case now. The real remaining item is on our side — adopt the declarative API and
delete `src/ui/animation_slide_in.h`. See the TODO.

### `UIStylingDefaults::apply_overrides` silently drops most visual fields
`apply_overrides` (`plugins/ui/component_config.h:920`) merges a caller's config onto a
registered default by forwarding an explicit, hand-maintained field list. Anything not on
that list is discarded without warning. Verified against the current submodule: it forwards
padding, margin, size, colors, label, corners, disabled/hidden, tabbing, font, texture,
absolute, flex and debug fields — and never forwards

    opacity, scale, translate_x, translate_y, flex_gap,
    border_config, shadow_config, text_shadow_config,
    on_draw_bg, on_draw_fg

all of which are real `ComponentConfig` members (`:65`, `:119-122`, `:130-131`, `:141-153`).

This only bites elements that *have* a registered default, i.e. everything routed through
`SetupGameStylingDefaults` — `imm::button`, slider, checkbox, dropdown, navigation_bar.

**Consequence beyond styling:** `animation_control::apply_slide_in()` animates via opacity
and translate, both dropped. **The slide-in animation has therefore been a no-op on every
button in the game.** `create_styled_button` has been building configs whose animation
fields are thrown away before they reach the renderer. This is a second, independent reason
`tests/e2e/10_slide_in_animation.e2e` cannot be testing anything (the first being that
`animation_control::disabled` is a global never cleared between scripts).

**Current workaround (in place):** `with_internal(true)` at the call site to bypass the
defaults merge, then re-apply the font by hand. Commented where used.

**Ideal:** one forwarding line per dropped field. The custom-draw hooks matter most — there
is no caller-side way to get `on_draw_bg` onto a styled button.

---

## Closed by the `12a4571 -> fc4d625` bump

### Animation: global instant-complete — **CLOSED**
`animation::set_instant(bool)` / `is_instant()` (`plugins/animation.h:102-103`). Deliberately
not an enable/disable flag: the animation still runs and `on_complete` still fires, so nothing
downstream has to branch.

### Animation: `clear_all()` — **CLOSED**

### e2e: built-in `disable_animations` / `enable_animations` — **CLOSED, and it shadowed ours**
Worth recording as a lesson rather than just a tick. afterhours registers
`HandleAnimationModeCommand` inside `register_builtin_handlers`, which we call *before*
`register_app_commands`. Both handlers `consume()` the command, so the engine's won and ours
silently stopped running — our hand-rolled slide-in and wiggle kept animating through every
screenshot, and run-to-run screenshot noise went from 6 screens to 8.

Fixed on our side by making `animation_control::disabled()` delegate to `animation::is_instant()`
so one flag drives both, and deleting our duplicate handlers.

**Feedback for the engine:** a consumer has no way to know a new builtin will start eating a
command it already handles, and nothing warns. Either warn on duplicate handling of the same
command name, or let `register_builtin_handlers` take an opt-out set.

### e2e: case-insensitive `expect_text` — **CLOSED** (`expect_text_i`)

### `with_disabled()` for interactive elements — **CLOSED**
`component_config.h:395`, `is_disabled()` at `:733`, plus `Theme::disabled_opacity`.
We use it zero times; the motivating case (disable "select map" until a win condition is
chosen) is still unimplemented on our side.

### `SINGLETON_FWD` in class scope — **CLOSED** (`SINGLETON_CLASS_FWD`)
No hand-patched vendor file needed.

### `key_codes.h` re-export — **CLOSED** (`e2e_testing.h:30`)

### Headless: `ProvidesCurrentResolution` returns (0,0) — **CLOSED**
`window_manager::headless_resolution` (`plugins/window_manager.h:62`) is the documented
fallback, defaults to 1280x720, and `fetch_current_resolution` warns once when it uses it.
Our per-frame patching system is deleted; we assign `headless_resolution` once before init.

### Headless: `std::bad_variant_access` on shutdown — **CLOSED**
`afterhours::shutdown()` (`src/shutdown.h`) does entities-then-backend, idempotent.
We did not adopt it: `Preload::~Preload()` already calls `graphics::shutdown()` after our
`delete_all_entities`, so our ordering is equivalent and switching would only move two calls.

### Windows cross-compilation — **ALL THREE CLOSED**
1. `path.string().c_str()` at every `ExportImage` site (`headless.h:156`, `windowed.h:123`,
   `drawing_helpers.h:519`).
2. `memory/arena.h:27` has `aligned_alloc_compat` with a `_aligned_malloc` branch.
3. `graphics/platform/headless_gl_windows.h` exists; the hard `#error` is gone, and there is
   an Emscripten stub too.

`make windows` now compiles clean through all of afterhours.

---

## Our side, not the engine's

Recorded here because they surfaced while chasing the above.

### `make windows` link: `IsKeyPressedRepeat` undefined
`vendor/raylib/raylib.h` is 5.5 and declares it (`:1174`), but `vendor/raylib/raylib.dll` and
`libraylibdll.a` do not export it — the vendored Windows binaries are an older raylib than the
vendored header beside them. Needs a real raylib 5.5 Windows build dropped in. This is the only
thing between us and a Windows binary.

### `log_error` aborted unconditionally
`src/log/log_macros.h` put `assert(false)` outside the macro's own `if` and unbraced, so the
macro was two statements and `if (x) log_error(...)` aborted whatever `x` was. Latent since it
was written; the bump walked into it when `runner.h:292` added a `log_error` inside a brace-less
`if`, aborting before the first e2e script. Now wrapped in `do/while(0)`.

Not an engine bug, but a note for the engine: a header-only library calling a macro the consumer
may have redefined is a sharp edge. `log_error` in a brace-less `if` is a reasonable thing to
write and it detonated a downstream project.

---

## Not yet evaluated

**Labels lost their free 5px inset.** `Theme::text_inset` replaces a hardcoded 5 and defaults to
0, which is a deliberate re-baseline: edge-aligned text shifts and wrapped text re-breaks. Our
screenshots move, but the suite still drifts run-to-run from the persisted-round-settings leak
(TODO tier 2), so the visual delta cannot be attributed until that is fixed. `-DAFTERHOURS_DEFAULT_TEXT_INSET=5.f`
restores the old pixels if we want them.
