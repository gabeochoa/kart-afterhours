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

**No longer hit here:** the Options screen was the last caller. Its toggles are now
`round_rules::toggle_row`, which is a button with an `on_draw_fg` knob.

### `imm::slider` with a label always overflows its own parent
`slider` (`plugins/ui/imm_components.h:1382`) gives itself `config.size`, then lays two
children in a Row inside it: `slider_text` at `config.size.scale_x(0.5f)` (`:1418`) and
`slider_background` at `config.size.x - 6px` (`:1433-1441`). 0.5 + ~1.0 of the parent width,
so the overflow is unconditional and roughly half the configured width — there is no value of
`config.size` that makes it fit, because both children are derived from that same size.

Verified: 462 occurrences each of `'slider_text' extends outside parent 'slider'` and
`'slider_background' extends outside parent 'slider'` in one e2e run, from the three volume
sliders. `child_size=[200,40]` and `[194,40]` against `parent_size=[400,25.6]` — over on both
axes, since neither child respects the parent's height either.

**Also unstylable.** `ComponentConfig::inherit_from` (`component_config.h:1008`) forwards only
`apply_inheritable_from`'s list (`:1016-1032`): alignment, disabled/hidden, tabbing, font,
internal, render layer, image alignment. No colour, no border, no `on_draw_*`. The track and
handle therefore take `Theme::Usage::Secondary` and `Primary` and cannot be overridden from the
caller at all. The handle is also 25% of the track wide and travels 0..75%, so it is a block,
not a knob.

**Workaround (in place on Options):** don't use it. `options::volume_row`
(`src/ui/ui_systems.cpp`) draws the meter in an `on_draw_fg` and attaches
`HasDragListener` + `HasLeftRightListener` to the element by hand — the same two components
`slider` attaches at `:1495` and `:1576`. Both are driven by generic systems
(`systems.h:1123`, `:1159`) keyed on the component, so a caller-built widget gets drag and
keyboard for free. `can_be_focused` (`systems.h:974`) is "has a click or drag listener", so the
element is tabbable on the same terms.

**Ideal:** give `slider_text` and `slider_background` complementary fractions of the parent, and
forward the visual fields through `inherit_from` so the track can be themed per call site.

### `with_opacity()` paints a dark rectangle over a transparent element
`colors::opacity_pct` (`plugins/color.h:148`) **assigns** `a = 255 * pct` rather than
scaling the colour's existing alpha. The background fill path
(`plugins/ui/rendering.h:1531-1540`) calls it whenever `effective_opacity < 1`, then draws
if `col.a > 0`.

So an element with `with_transparent_bg()` — which is `Color{0,0,0,0}`
(`component_config.h:347`) — plus any `with_opacity(v)` below 1 gets its alpha *raised*
from 0 to `255v` and is filled with **black at that alpha**. Asking for 55% opacity on a
transparent label paints a 55%-black box behind it.

Verified: a `with_transparent_bg().with_opacity(0.55f)` div over the `#1B1040` panel
sampled `(9,5,22)`; the identical div without the opacity call sampled the panel colour.
0.85 came out darker than 0.55, which is the giveaway — higher requested opacity, more
black.

**Current workaround (in place):** never combine `with_opacity` with a transparent
background. `round_rules::weapon_row` (`src/ui/ui_systems.cpp`) dims disabled weapon rows
by choosing a muted text/tint colour instead.

**Ideal:** `opacity_pct` should be `a = color.a * pct`. That is the meaning every caller
already assumes, and it makes the `col.a > 0` guard actually skip transparent fills.

### `with_slide_in()` config option
Open as originally written, but effectively obsoleted: `Anim::on_appear().opacity().translate_x()`
covers the use case now. The real remaining item is on our side — adopt the declarative API and
delete `src/ui/animation_slide_in.h`. See the TODO.

### `with_gap()` ignores the `Size` dim and reads the value as raw pixels
`ComponentConfig::with_gap` (`plugins/ui/component_config.h:605`) stores a full `Size`, and
`component_init.h:177` copies it to `UIComponent::desired_gap`. Every consumer then reads only the
scalar: `autolayout.h:678`, `:1088` and `:1306` all call
`resolve_pixels(widget.desired_gap.value, widget)`, and `resolve_pixels` (`:102`) does nothing but
multiply by `ui_scale`. Nothing ever looks at `desired_gap.dim`, unlike the size constraints, which
go through a dim-aware path (`:264`, `:340`, `:380`).

So a gap expressed in anything other than `pixels()` is silently wrong by that dim's conversion
factor. `ui::w1280(16.f)` is `screen_pct(16/1280)` (`layout_types.h:172-173`, `:147`), i.e.
`.value == 0.0125` — a 0.0125px gap. The `> 0.f` guards at `:1087` and `:1305` still pass, so it is
applied, just 1280x too small. It fails silently in the direction that looks like "gap is not
implemented".

Verified here: `map_selection`'s body hstack with `.with_gap(ui::w1280(16.f))` laid its two children
out at `x=36 w=420` and `x=456 w=732` — 420 + 732 = 1152 = the full parent width, zero gap inserted
(`dump_ui`, headless 1280x720). Swapping to `pixels(24.f)` produced exactly the 24px asked for, and
`09_map_selection_interactions.e2e` now pins it with `assert_ui map_preview_panel x=456`.

Consequence: this is why the Round Settings panels touched, and the character-select paint
swatches. Every `with_gap` in `src/` now passes `pixels()` — `ui_systems.cpp:703`
(`character_select::paint_palette`), `:1352` (`round_rules::mode_tabs`), `:2381`
(`round_settings_panels`), `:2524` and `:2580` (`map_selection`). Grep for `with_gap(ui::` before
adding another; a screen-relative gap compiles, applies, and does nothing visible.

**Workaround (in place on Track Select):** pass `pixels()`. It still scales with resolution, because
`resolve_pixels` applies `ui_scale` in Adaptive mode.

**Ideal:** resolve `desired_gap` through the same dim-aware path the size constraints use.

---

### `with_absolute_position()` resolves **x** against the screen *height*
`apply_modifiers` (`plugins/ui/component_init.h:431-443`) reads one number:

    float screen_height = 720.f;
    if (auto *pcr = ...ProvidesCurrentResolution>()) screen_height = pcr->current_resolution.height;
    float resolved_tx = resolve_to_pixels(config.translate_x, screen_height, scaling, uis);
    float resolved_ty = resolve_to_pixels(config.translate_y, screen_height, scaling, uis);

and hands `screen_height` to both axes. For an absolute element those two values *are* its
position (`:446-452`), so an x expressed as a fraction lands at `pct * height`, never
`pct * width`. Sizes do not have this problem — `ComponentSize` goes through the dim-aware
path, so the same `screen_pct` means different pixels for `x` and for `w` on the same element.

Verified here: the pause card at `.with_absolute_position(screen_pct(0.34375f), ...)` measured
`x=248` — `0.34375 * 720` — where centring a 400px card in 1280 needs 440 (`dump_ui`, headless
1280x720). Respelling it `ui::h720(440.f)` produced exactly `x=440`.

**Consequence beyond the one call site.** Every redesigned screen positions its content box at
`screen_pct(0.05f)` and sizes it `screen_pct(0.90f, 1.f)`. The size is dim-aware and gives 1152;
the position is not and gives 36, not 64. So all five screens sit in a 36px / 92px left-right
frame rather than 64 / 64 — off centre by 28px, on `ui_systems.cpp:1758`, `:2315`, `:2429`,
`:2697` and `:2959`. Not corrected: the five would have to move together and every baseline
with them.

**Workaround (in place on Pause):** spell an absolute x with `ui::h720(px)`, which is the same
"px at 720p, scales with the window" contract the resolver actually implements. Commented at the
call site, because `h720` on an x coordinate reads like a typo.

**Ideal:** resolve `translate_x` against width when the element is absolute, the way
`ComponentSize` already distinguishes the axes.

---

### `disable_rounded_corners()` gives you a rounded focus ring
`disable_rounded_corners` (`plugins/ui/component_config.h:484`) sets `rounded_corners` to an
all-zero `bitset<4>`. `apply_visuals` (`plugins/ui/component_init.h:331`) attaches
`HasRoundedCorners` only when `config.rounded_corners.value().any()` is true — so for an all-zero
bitset the component is never attached at all. `focus_ring_for` (`plugins/ui/rendering.h:252`) then
tests `entity.has<HasRoundedCorners>()`, sees false, and falls back to `context.theme.corner_radius`
and `theme.rounded_corners`.

Net effect: asking for square corners gets a square *border* and a theme-**rounded** focus ring
inside it. The two disagree only while the element is focused, which is why it reads as a stray
outline rather than a corner setting.

"All corners off" and "nothing specified" are the same value here, so the config cannot express the
difference. Found on the track tiles, where a selected tile drew a square butter frame with a
rounded cream ring inside it.

**Workaround (in place):** `.with_rounded_corners(RoundedCorners{}).with_corner_radius(0.f)` —
corners on, radius zero. That passes `.any()`, so the component attaches and the ring matches.

**Ideal:** attach `HasRoundedCorners` whenever `rounded_corners.has_value()`, regardless of `.any()`.

**Still affected in our code** (square border, rounded focus ring): `ui_systems.cpp:309`, `:368`,
`:395`, `:1632`, `:3004`. Only the track tile at `:1530` uses the workaround.

---

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
