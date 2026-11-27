# Afterhours Engine Gaps and Feedback

Feedback and feature requests for the afterhours engine, discovered while working on kart-afterhours.

Last re-triaged against afterhours `5383fc6c` (bump landed in 5d2f7af; e2e/audio adoptions in follow-up commits). Every entry below was
checked against the current submodule, not carried forward on trust.

**Score: 10 of 17 previously-open gaps are now closed.** All three Windows
blockers and every headless workaround but one are gone.

---

## Still open

Found while writing e2e tests against `5383fc6c` (2026-11-27). Every entry
below was checked against the current submodule; the kart-side workarounds
named in them are in the working tree at the time of writing. The first
three mislead in the same way: the script passes, or times out, for a
reason unrelated to what it claims to test.

### e2e: `expect_slider` tolerance is silently dropped by the script parser
The handler documents `expect_slider name value [tolerance]` and reads
`pos[2]` as the tolerance (`ui_commands.h:1000`, `:1017-1020`). The
script parser never delivers it: `expect_slider` is in
`two_arg_commands` (`runner.h:126-129`), and the parse branch reads
exactly 2 args (`runner.h:367-371`). A script asserting
`expect_slider max_speed 0.5 0.5` therefore runs with the default 0.01
tolerance and times out reporting the value 0.4737 — a difference of
0.026 against a tolerance of 0.5 that was never applied. The same
truncation applies to the inner command of `wait_until`, which re-parses
through the same table.

**Current workaround:** never pass a tolerance; assert the exact value a
`set_slider` just landed on (default tolerance only). That is what
the debug-screen probe did: `set_slider max_speed 0.8` followed by
`expect_slider max_speed 0.8` passes, whereas `expect_slider
max_speed 0.5 0.5` timed out at a value 0.026 away from expected.

**Ideal:** `expect_slider` needs an optional-arg parse (or a
`three_arg_commands`-style entry with the third arg optional), so the
documented tolerance reaches the handler. Any other two-arg command that
grows an optional third argument will hit the same table.

### e2e: `select_dropdown` ignores the option it requires
`HandleSelectDropdownCommand` requires `name and option`
(`ui_commands.h:870`), then never reads `pos[1]`: it clicks the centre
of the dropdown entity and consumes the command. The tray opens, no
option is selected, the selection does not change, and the script passes
believing it selected something. The tray options themselves are named
`dropdown_opt_{i}` per dropdown (`imm_containers.h:199`) with no
dropdown identity, so even the manual follow-up click needs
`under=<dropdown>` scoping to pick the right tray.

**Current workaround:** `select_dropdown <name> <option>` to open the
tray, then `click_ui dropdown_opt_N under=<dropdown>` to choose. Kart
probed this against the resolution dropdown (which also exposed a
kart-side null deref, fixed on our side, not the engine's).

**Ideal:** after opening the tray, the handler should click the tray
option whose label equals the requested option (over frames, like
`set_slider`'s press/move/release phases), and fail naming the
available options if none matches. A command that requires an argument
it never reads should not consume.

### e2e: `expect_no_text` can never pass on a screen with no visible labels
`HandleExpectNoTextCommand` retries while `seen_render == gen ||
registry.get_all().empty()` (`command_handlers.h:746`). The registry is
cleared before every render pass (`ui/utilities.h:369`) and `clear()`
bumps the generation (`visible_text.h:24-27`), so a fresh render with
zero labels is already definitive absence — but the `empty` clause
retries regardless of how many generations have moved. In gameplay,
where no UI labels render, the command times out after 30 frames
reporting an empty registry, even though the text is genuinely absent.
The generation check alone already distinguishes "not yet rendered"
from "rendered with nothing in it".

**Current workaround:** assert positively instead — e.g. `expect_text
"paused"` after Escape proves the game left the menu. Kart dropped
the `expect_no_text "select map"` gameplay assertion for exactly
this reason.

**Ideal:** drop the `|| registry.get_all().empty()` clause (or only
retry on empty while the generation has not moved). `expect_no_text`
should be provable on label-free screens, not only on screens that
happen to show other text.

### Capture: `draw_texture_pro` records nothing, so `expect_drawn` is blind to sprites
Every other primitive records into the capture buffer — `rectangle`,
`circle`, `line`, `triangle`, and `texture_quad`
(`backends/raylib/drawing_helpers.h:240,521,494,600,578`).
`draw_texture_pro` does not, on either backend
(`backends/raylib/drawing_helpers.h:688`,
`backends/sokol/drawing_helpers.h:1575`). `texture_manager::draw_texture_pro`
— the path every sprite draw in kart funnels through
(`plugins/texture_manager.h:247`) — therefore leaves no trace, and
`expect_drawn` / `expect_drawn_at` / `dump_draws` cannot see a car
sprite, only UI draws that happen to go through the other helpers.

**Current workaround:** kart's `11_draw_assertions.e2e` asserts recorded
UI draws only; sprites are covered by screenshot baselines.

**Ideal:** `capture::record("texture", dest, tint)` in both backends'
`draw_texture_pro`, matching the `texture_quad` op in shape. Shipping
builds already pay the one-branch check per draw, so this is the
same cost as the primitives that already record.

### Composite widgets lose their label before naming, so every instance shares a debug name
`add_missing_components` derives a debug name from `config.label` when
no explicit debug name is set (`component_init.h:709-715`). Composite
widgets clear `config.label` before `init_component`, so that branch
never fires for them:

- `slider()` clears the label (`imm_value.h:122`) and passes the
  literal fallback `"slider"` (`imm_value.h:131`) — every slider on
a screen is named `slider`.
- `checkbox()` clears the label (`imm_controls.h:183`) and passes
  `"checkbox_row"` (`imm_controls.h:190`) — every checkbox shares
  one name.
- `dropdown()` clears the label (`imm_containers.h:70-73`) and passes
  no fallback at all — a dropdown with no explicit debug name has
  an empty name, unaddressable by any DebugName lookup.

Name-based commands (`set_slider`, `expect_slider`, `expect_checkbox`,
`select_dropdown`, `focus_ui`, `click_ui`) then fail ambiguous or
not-found unless the caller sets `with_debug_name` on every instance.
An explicit debug name already wins (`component_init.h:709-710`), so
this is a defaults-only fix.

**Current workaround:** kart sets `with_debug_name` on every volume
slider, dropdown, checkbox, debug-screen slider, pause button,
round-settings nav bar and weapon group (`src/ui/ui_systems.cpp`).

**Ideal:** derive the default name from the saved label before it is
cleared — e.g. `"<label> slider"`, `"<label> dropdown"`,
`"<label> checkbox"` — so the common case (labelled widgets, no
explicit name) is addressable without per-instance boilerplate.

### e2e: `drag` is not a drag
`HandleDragCommand` (`command_handlers.h:373`) calls
`simulate_click` at the start point, then moves the pointer to the
end point in the same invocation. `simulate_click` auto-releases
after one frame (`test_input.h:119-122`, press with `auto_release`), so
by the time any system could see the pointer at the end point, the
button is already up: no drag gesture is ever delivered. `drag_to`
(`command_handlers.h:586`) does it correctly over three frames
(press, move while held, release), as does
`mouse_down_ui` + `mouse_move_ui` + `mouse_up`.

**Current workaround:** kart's drag test uses `drag_to` (slider landed
at 0.93) and the named mouse gestures (0.76); plain `drag` was not
used.

**Ideal:** make `drag` share `drag_to`'s phase machine, or document
it as click-then-move and steer scripts to `drag_to`. Two commands
with the same four arguments should not differ in whether the button
is held.

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

---

## Closed by the `fc4d625 -> 5383fc6c` bump

### e2e: `--screenshot-dir` flag — **CLOSED**
The shipped harness (`e2e_testing/harness.h: E2EArgs`, `parse_e2e_args`,
`configure_runner`) parses `--screenshot-dir`, and kart wires it through
`e2e_integration::init(argc, argv)` (`screenshot_path_for`, verified with
`--screenshot-dir /tmp/e2e-json`). `--results-json`, `--timeout` and
single-script selection (`--test-script` / `--test-script-dir`) came with it.

### e2e: `--e2e-speed` flag — **CLOSED (parsed, not applied)**
`parse_e2e_args` accepts `--e2e-speed` as a spelling of `--time-scale`.
Kart parses it but deliberately does not apply it: kart's loop is
variable-dt, not fixed-step, so scaling `dt` would speed up the game
itself. Applying it needs the fixed-step wiring (`apply_time_scale` /
`run_fixed_steps`), which kart does not have.

### Checkbox internal layout overflow — **CLOSED (kart-specific)**
"A labelled checkbox's toggle fits its row (kart)" in the changelog. The
`checkbox label extends outside parent checkbox_row` layout warning no
longer appears in e2e runs after the bump (verified in the `make e2e`
output of the bump commit).

### `with_slide_in()` config option — **CLOSED by the motion rewrite**
The declarative API it pointed at is itself gone. Kart's slide-in is now
`motion::Track` chains in `src/ui/animation_slide_in.h` (the file is
still hand-rolled, as the declarative widget sugar does not cover a
per-entity staggered slide-in driven by a system). No engine gap remains.

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
