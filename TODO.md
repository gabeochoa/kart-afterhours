# TODO

Findings from a full read of the project on 2026-08-29. Every item below was verified against
the source, not inferred — file:line references are accurate as of commit `28c5d21`.

Tiers are ordered by payoff. Tier 1 is things that are silently **broken** rather than unfinished.

`vendor/afterhours` is a git submodule shared with ~10 other projects in `~/p/`, so anything only
fixable upstream lives in Tier 4 and is recorded rather than acted on.

---

## Tier 1 — Broken

- [ ] **`RenderSpritesWithShaders` renders the previous frame's pointers.** Fixed, but the shape
  is worth remembering: `for_each_with` stored raw `const Transform*` / `HasShader*` etc. in a
  batch and `once()` rendered it, and `SystemManager::render` calls `once()` *before* the entity
  loop (`vendor/afterhours/src/core/system.h:513`). So the batch was always a frame stale, and
  destroying any entity with a sprite and a shader left every pointer in it dangling. Unreachable
  until eliminating a player could actually destroy a kart. Now renders from `after()`.
  Covered by `tests/e2e/14`, which segfaults without the fix. **Audit the other systems that
  cache component pointers across a hook boundary** — this one was found by accident.
- [x] ~~**`MatchKartsToPlayers` can rebuild eliminated players mid-round.**~~ — done, and now
  covered. Guarded to `is_menu_active()` (`systems.h:805`), because `make_player` puts `PlayerID`
  on the car itself and `ProcessDeath` destroys the car when lives run out.
  `tests/e2e/13d_eliminated_player_not_rebuilt.e2e` needs three karts to see it: a player missing
  while another player is still there and the round still running. Two players is the ceiling —
  at three, the "a player left" branch below fires in the menu and deletes the ones with no
  gamepad — so the third contender is a bot. Reverting the guard gives
  `expect_car_count (line 48): expected 2 karts but was 3`.
- [ ] **The "we are good" early return is off by one.** `existing_players.size() + 1 ==
  maxGamepadID.count()` with `count() == max_gamepad_available + 1` reduces to `players == max`,
  but steady state is `players == max + 1`. So the guard is essentially never true when things
  are correct, and true exactly once when a player is missing. Noticed while testing the above;
  not touched.

- [x] ~~**Menu and UI sound is dead.**~~ — done. `register_sound_systems` is called again, placed
  after `register_ui_systems` so `UIClickSounds` reads this frame's `HasClickListener.down` rather
  than last frame's. **Still unheard:** `preload.cpp` puts `InitAudioDevice()` inside the
  `Windowed` branch, so headless never opens an audio device and no test can confirm playback.
  Needs a windowed listen.
- [x] ~~**`CarRumble` has an empty body**~~ — deleted, along with its registration.
- [x] ~~**E2E failures cannot fail anything.**~~ — done. `main` returns `has_failed() ? 1 : 0`.
  Verified in both directions with a deliberately broken script 02, chosen over the last script
  specifically to check that a mid-suite failure is not swallowed.
- [x] ~~**Per-round state never resets.**~~ — done. `RoundManager::reset_car_trackers()`
  (`round_settings.h:325`) clears lives, kills, hippos and health on every car at round start.
  Covered by `tests/e2e/11_two_round_lives_reset.e2e`.
- [x] ~~**Cross-mode UB in TagAndGo.**~~ — fixed, and now under a discriminating test.
  `tests/e2e/12` holds the stale `is_tagger` flag but has nobody to tag, so it only reaches
  `get_active_rt` and not the transfer.
  `tests/e2e/12a_tagandgo_cross_mode_two_karts.e2e` parks a second kart on the first in Lives
  mode; reverting both halves gives `expected is_tagger == 1 but was 0`.
  `12b_tagandgo_tag_transfer.e2e` is its positive control — the same setup in TagAndGo, where the
  tag must move. Note `raylib::GetTime()` is 0 under `--headless` while `reset_temp_data` leaves
  `last_tag_time` at `-1`, so every runner reads as tagged one second ago and the 2s cooldown
  declines every tag. Both scripts backdate `last_tag_time` to get past it. **A headless run can
  never observe a tag land on its own** — worth remembering before writing any other TagAndGo
  test.
- [x] ~~**Single-player Lives mode ends instantly.**~~ — done. `CheckLivesWinFFA` tracks
  `most_contenders_seen` and only declares a one-survivor winner if there were ever two.
  Covered by `tests/e2e/13`.
- [x] ~~**`MarkEntitiesWithShaders` is registered twice**~~ — one registration remains.
- [x] ~~**`num_starting_lives` is read at car-creation time**~~ — moot. `reset_car_trackers`
  re-reads it from settings at the start of every round.
- [x] ~~**Settings only persist on exit.**~~ — done. `Settings::save_if_changed()` is polled once
  a second from `main.cpp`. It diffs against the last write rather than using a dirty flag,
  because `get_fullscreen_enabled` and `get_post_processing_enabled` return a `bool&` that
  callers mutate without going through a setter. Off under `--e2e` via
  `Settings::autosave_enabled`, so a test run cannot clobber the player's own file.
- [x] ~~**`strings::pre_translation` is an unlinkable declaration.**~~ — deleted, along with the two
  `strings::get_string()` overloads that read it. Zero callers confirmed;
  `translation_manager::get_string` is a different function and is unaffected. `<map>` and
  `<functional>` went with them.
- [x] ~~**Japanese string contains a Korean character.**~~ — `translation_manager.cpp:410` now
  reads `"鬼: {}秒"`.
- [x] ~~**Round-length dropdowns show raw enum identifiers.**~~ — done. The dropdowns are gone;
  the clock is a stepper reading `round_rules::time_option_label`, the one place the option is
  put into words, and it is translated. The two duplicate `"10s"/"30s"/"1m"` switches in
  `render_round_settings_preview` now call it too.
- [ ] **Untranslated UI text** — renders English regardless of language: `"Team Mode"`
  (`ui_systems.cpp:1096`), `"No players"` (`:1042`), `"Score: "` (`:2528`), `"Team A"`/`"Team B"`
  (`:1151,1153`), slot labels (`:653-660`).
  Round type names, weapon names and the hardcoded `"10s"/"30s"/"1m"` are done (see
  `strings::i18n::round_type_*`, `weapon_*`, `time_*`). Still English-only on the rules screen:
  the mode and weapon one-liners, the `HOW DO WE WIN` heading, the row labels and the panel
  header/footer — new copy, added as literals the way the character-select screen did.

## Tier 2 — Tests and CI

- [x] ~~Fix the e2e state leak~~ — done. `--e2e` now starts from `SettingsData` defaults and never
  writes the save file, so a screenshot depends on the build and nothing else. All 52 screenshots
  are byte-identical across runs. Also stops test runs clobbering the player's own settings.
- [x] ~~`screenshot-baselines/` has never existed~~ — done, and regenerated after the Memphis
  redesign: 59 baselines (18MB), `make validate-screenshots` passes 59/59, and all 59 are
  byte-identical across consecutive runs. Verified earlier that it fails at 3.09% on an injected
  change.
- [x] ~~**Test 10 tests nothing.**~~ — fixed, and it now captures the slide-in mid-flight.
  There were **three** reasons it tested nothing, not one: `disable_animations` is never cleared
  (correct as stated); the script never called `enable_animations`; and `UpdateUISlideIn` only
  retriggers on a screen *change*, while script 09 leaves the game on Main, so `goto_screen Main`
  was a no-op and every frame was post-animation. The script now detours via RoundSettings.
  **The gap doc's stronger claim was wrong**: `docs/afterhours_gaps.md:212-217` says the slide-in
  "has been a no-op on every button in the game" because `apply_overrides` drops opacity and
  translate. It drops them from the *config*, but the animation is driven by
  `UpdateUISlideIn`/`ApplyInitialSlideInMask` writing `HasUIModifiers` and `HasOpacity` onto the
  entity directly (`animation_slide_in.h:120-127`, `:255-262`), which never goes through
  `apply_overrides`. The main-menu buttons visibly slide in. That makes
  `animation_control::apply_slide_in` (`animation_control.h:18`) the dead half — for
  `create_styled_button` it is dropped, and for the two `keep_visuals` call sites it duplicates
  what the mask system already does one frame later.
  Now three screenshots, not four: headless dt is a fixed 0.167s and every command costs two
  frames, so the 0.43-0.55s animation is exactly three samples wide. A fourth is byte-identical
  to the third however the waits are arranged.
- [x] ~~**`05_full_menu_flow.e2e` is mislabeled**~~ — done. Now drives Main -> CharacterCreation
  -> RoundSettings -> MapSelection with `enter` and back with `escape`, asserting the focus holder
  at every step with `expect_focused`. `shift_tab` reaches the confirm button in one keystroke:
  it is last in the tab ring on both screens (forward would be 11 and 12 tabs). All four
  `flow_*` baselines are unchanged — focus auto-grabs the same first element whether the screen
  was entered by keyboard or by `goto_screen`.
- [x] ~~**`validate-screenshots` and `update-baselines` never clean `screenshots/`**~~ — both now
  depend on `clean-screenshots`.
- [ ] **`assert_no_overflow` is weaker than it looks.** Now used by scripts 09, 13a and 13b, but
  it checks only the *viewport*, not parent bounds — it could not have caught the
  `map_card`-in-`map_list` overflow it was originally suggested as coverage for, and a squeezed
  child never trips it. Established by deliberately breaking the Track Select layout two ways and
  watching the suite stay green. Keep it as a cheap net; do not read it as layout coverage.
- [ ] **Use the assertions that already exist.** `expect_focused`, `enter`, `escape` and
  `shift_tab` are now used by script 05. Still zero scripts: `set_slider`, `expect_slider`,
  `select_dropdown`, `expect_checkbox`, `expect_no_text`, `dump_ui`, `key`, `drag`, `resize`.
  `assert_no_overflow` is free coverage for the checkbox-overflow gap the docs already describe.
- [x] ~~**No CI at all**~~ — `.github/workflows/ci.yml` added: checkout with submodules, zig
  0.16.0, `brew install pkg-config raylib`, Pillow, `make build`, `make validate-screenshots`,
  screenshots uploaded as an artifact on failure. **It has never run on a real runner** — what is
  verified is that the same commands pass locally on macOS, and that the runner deps are the ones
  the build actually links (`build.zig:73-74` resolves raylib through pkg-config and links the
  OpenGL framework). Unproven until it runs: whether a headless macOS runner can create the CGL
  context, and whether Homebrew's raylib is still 5.5 on the day it runs.
  **The "macOS/Linux" note was wrong.** Headless GL is macOS-only:
  `vendor/afterhours/src/graphics/platform/headless_gl_linux.h` is an `assert(false)` stub. A
  Linux runner cannot run the suite at all, and `build.zig:74` links the OpenGL *framework*
  unconditionally, which would not compile there either. Both are Tier 4.
- [ ] **No unit tests anywhere.** Not attempted, deliberately. The named subjects — physics,
  weapons, AI, collision, win conditions — are all `System` subclasses in `src/systems/` that need
  entities, singletons and the afterhours runtime to say anything; standing that up is the e2e
  suite, which already covers win conditions, elimination, round reset and TagAndGo. What is left
  that is pure enough to unit test is `src/math_util.h`, 95 lines of one-line vector helpers.
  A test binary would also need a second root module in `build.zig` (src/ has a `main`). Worth
  doing when there is a non-trivial pure function to point it at; there isn't one today.
- [x] ~~**`compare_baselines.py` defects:**~~ — all three fixed. `--save-diffs` is now
  `BooleanOptionalAction` so `--no-save-diffs` exists. The manifest mechanism is **deleted**
  rather than given a `manifest.json`: it existed to allow a per-screen tolerance, every one of
  the 55 stable baselines currently matches to within 0.03%, and nothing has ever needed a
  looser one. With it gone the threshold has a single source, which makes the third defect (the
  JSON summary reporting `args.threshold`) correct by construction. Re-add it the day one
  screenshot genuinely cannot be made deterministic.

## Tier 3 — Cleanup

- [x] ~~Delete dead files (~570 lines).~~ — **already done in `464eebe`**, before this pass started.
  `multipass_integration.h`, `multipass_renderer.h`, `shader_pass_registry.h`, `utils.h`,
  `log/log_fakelog.h` do not exist. That commit also removed the `entity_test` / `entity_enhanced`
  loads from `preload.cpp`, so the two `.fs` files were orphaned on disk; deleted now, along with
  their `ShaderType` enum entries.
- [x] ~~Delete dead functions and systems (~200 lines).~~ — mostly **already done in `464eebe`**:
  `Weapon::apply_recoil`, `CanShoot::fire` and the pre-`ProjectileConfig` `make_poof_anim` /
  `make_bullet` overloads all grep to zero occurrences (the surviving `make_poof_anim` /
  `make_bullet` are the `ProjectileConfig` versions, called from `systems.h`). What was still
  there and is now deleted: `RenderRenderTexture` and `BeginPostProcessingShader`, 46 lines.
  `LetterboxLayout` / `compute_letterbox_layout` / `mainRT` stay — they have other callers.
- [ ] **`RenderDebugGridOverlay` (`systems.h:631`) is still defined and never registered.** Left in
  place deliberately. It is complete and self-toggling, but it toggles on
  `InputAction::ToggleUIDebug`, which already drives the afterhours UI debug overlay
  (`ui_systems.cpp:1969`). Registering it as-is makes one key drive two overlays. Wire it to its own
  action, or to the debug UI, before registering.
- [x] ~~`ui_helpers::create_control_group`~~ — deleted, zero callers after the Memphis redesign.
  `control_group_padding`, the module-level `Padding` it was named after, was also unreferenced and
  went with it.
- [x] ~~**~46MB of unused fonts tracked in git.**~~ — deleted from the working tree.
  `Sazanami-Hanazono-Mincho.ttf` (30MB), `NotoSansKR.ttf` (10MB), both Nerd Fonts (2.3MB each),
  `Gaegu-Bold.ttf` (3MB), `eqprorounded-regular.ttf`. 45.9MB out of the index. All three `FontID`
  cases resolve to the two `NotoSansMonoCJK*-Bold.otf` files that remain, so no language path
  changed. **History is not rewritten** — a fresh clone still pays for them. That call is the
  user's.
- [ ] **`SYMBOL_FONT` returns the Korean CJK font** (`preload.cpp:40`) rather than a Nerd Font.
  Confirmed still true. Not changed: both Nerd Fonts are now deleted, so "fixing" it would mean
  re-adding 2.3MB for a symbol set nothing currently asks for. Either delete the `SYMBOL_FONT`
  `FontID` case, or restore a Nerd Font and point it there — a decision, not a cleanup.
- [x] ~~Untrack files that shouldn't be tracked.~~ — `compile_commands.json`, root `settings.json`
  and `.vscode/{launch,settings,tasks}.json` are all out of the index. The two dead files
  (`compile_commands.json`, `settings.json`) are also off disk; `.vscode/` is left alone locally.
  Replaced with `compile_flags.txt`: zig emits no compile DB, and clangd's flat-flag file cannot go
  stale when a source file is added.
- [x] ~~`.gitignore` bug: `settings.json` has no leading slash~~ — now `/settings.json`. Note the
  original claim was half wrong: `.vscode/settings.json` and `.claude/settings.json` are matched by
  the `.vscode/` and `.claude` rules, not by this one. What it actually covered was any future
  `src/settings.json` and the like. There is no `TODO.md ` entry in `.gitignore` — never was.
- [x] ~~300MB of stale build artifacts on disk.~~ — `output/` and `output-win/` deleted, and
  `make clean` now removes them. `screenshots/` still has its own `clean-screenshots` target.
- [x] ~~`xmake.lua` is a stale third build system~~ — deleted, with `make xmake`, `cba`,
  `clean-cba`, `getxm` and `xm`.
- [x] ~~`tools/dependency_baseline.json` is stale~~ — **retired rather than regenerated.**
  `deps-check` diffs a hand-copied snapshot that nothing keeps current, so it has failed on every
  tree since the last rename and would go stale again on the next one; a lint nobody can pass is
  worse than no lint. Gone with it: the baseline, `tools/dep_config.example.json` (documents a
  `--config` flag `main()` has never had), and `deps-html` (byte-identical to `deps-dot`).
  `deps` and `deps-svg` remain as ad-hoc analysis and now build `dependency_graph` first, so they
  work on a clean checkout.
- [x] ~~`mcp.json` is broken~~ — points at `./zig-out/bin/kart`. The absolute `cwd` stays: the game
  loads `resources/` relative to cwd and MCP clients have no portable repo-root variable.
- [x] ~~`make output` never copies the executable~~ — it now depends on `build`, copies the
  executable, and only copies the raylib DLL under `TARGET=windows`.
- [x] ~~`HOW_TO_PLAY.md` tooling is stale~~ — both `./output/kart.exe` references are now
  `./zig-out/bin/kart`.
- [x] ~~Delete completed plan docs.~~ — `CLEANUP_PLAN.md` and `automatic.md` deleted, both
  re-verified first. `rfc_worker_threads.md` is now `adr_worker_threads.md`, with a Rejected status
  line and "Proposed Solution" retitled "What Was Tried".
- [x] ~~Unreferenced vendored headers~~ — `vendor/claylib.h` and `vendor/RaylibOpOverloads.h`
  deleted, 0 refs each.
- [ ] **Prune 24 remote branches**, most from 2024–2025 (`origin/box2d` and `origin/padding` from
  Dec 2024, a dozen `feature/*` and `cursor/*` from Aug 2025).
- [x] ~~`.PHONY` is missing several targets~~ — `count`, `countall`, `cppcheck`, `prof`, `leak`,
  `alloc` added; `cba`, `clean-cba`, `getxm`, `xm` and `brawlhalla` deleted instead.
- [ ] **`build.zig:52` collects afterhours plugins flat** while `src/` is walked recursively.
  Correct today (only `files.cpp` + `settings.cpp` exist) but it will silently drop a future `.cpp`
  in a plugin subdirectory.
- [ ] **`cfg.time_scale = 10.0f` is set unconditionally** (`preload.cpp:70`) with a "for headless"
  comment. Harmless today — only `backends/raylib/headless.h:112` reads it — but it's a trap.
- [ ] **UI code quality.** `src/ui/ui_systems.cpp` is 2773 lines, 96% of `src/ui/`.
  `animation_slide_in.h:137-215` is a verbatim 45-line copy-paste of `:87-131`. Slide-in silently
  skips any element past 25% of screen width (`:74-76`, `:161-164`), which is why map cards need the
  manual `apply_slide_mods` path (`ui_systems.cpp:226`). ~15 lines of manual mouse hit-testing are
  duplicated at `:2043-2057` and `:2134-2148`.

## Tier 4 — Blocked on afterhours

The submodule is shared with ~10 projects. Recorded here so they aren't re-derived; not actionable
from this repo.

- [x] ~~Re-triage `docs/afterhours_gaps.md`~~ — done 2026-08-29 against `fc4d625`. 10 of 17 closed.
  Still open: `--screenshot-dir` (the one that blocks a clean baseline workflow), `--e2e-speed`,
  headless `GetFontDefault()` (workaround still live at `preload.cpp:144-209`), checkbox overflow,
  and adopting `Anim::on_appear()` in place of our own slide-in.
- [x] ~~Windows blocked on three afterhours issues~~ — **all three fixed upstream.**
  `make windows` now compiles clean through afterhours and reaches the link.
- [ ] **Headless GL is not implemented on Linux.**
  `vendor/afterhours/src/graphics/platform/headless_gl_linux.h` is an `assert(false)` stub with a
  "use Xvfb as workaround" note, so `--headless` — which the entire e2e suite runs under — cannot
  come up on Linux. This is why CI is a macOS-only workflow. Fixing it upstream (EGL surfaceless
  context, the way `headless_gl_macos.h` uses CGL) would let CI move to a free Linux runner.
  `build.zig:74` would need the OpenGL framework link guarded to macOS at the same time.
- [ ] **`make windows` link fails on one symbol: `IsKeyPressedRepeat`.** `vendor/raylib/raylib.h`
  is 5.5 and declares it (`:1174`), but `vendor/raylib/raylib.dll` and `libraylibdll.a` don't
  export it — the vendored Windows binaries are an older raylib than the header beside them. Drop
  in a real raylib 5.5 Windows build. This is the only thing between us and a Windows binary.
- [ ] **Adopt `with_disabled()`** (`component_config.h:395`) — kart uses it zero times, and the
  case that motivated the original gap report (disable "select map" until a win condition is
  chosen) is still unimplemented.
- [ ] **kart is coded against the pre-upgrade afterhours UI API.** Uses 13 of ~30 widgets; zero uses
  of `setting_row`, `modal`, `toast`, `tab_container`, `radio_group`, `toggle_switch`,
  `progress_bar`, `tray`, `stepper`, `expand()`/`flex_grow`, `with_gap`, `ButtonVariant`, or the
  declarative animation API. Three of the four things `src/ui/`'s 500 lines of custom animation
  exist to provide now ship in the box: `Anim::on_appear().opacity().translate_x()`,
  `Anim::on_hover().scale()`, `with_disabled()`.
  `docs/plans/2026-02-25-afterhours-upgrade-review.md` step 2 is done; steps 3–9 grep to zero.
- [ ] **Theming RFC partially landed engine-side, unadopted here.** `Theme::Builder` + `Palette`
  auto-derivation + WCAG-AA check exist (`theme.h:362-560`), but there are no
  `Theme::light()/dark()/kart_style()` presets, `UIStylingDefaults` was never merged away, and kart
  still calls the old path with 6 hand-written `set_component_config` blocks
  (`ui_systems.cpp:38-108`) plus `pixels(400.f), pixels(40.f)` repeated at 6+ sites. Per-language
  fonts are driven by re-calling `set_default_font` (`ui_systems.cpp:2402`) instead of
  `theme.language_fonts`, which the engine now supports.
- [ ] **`UI_LIBRARY_RESEARCH.md` (80KB) recommendation not done.** Its Strategy 5
  (iterate-until-convergence + `fill_default_values()` for `-1.0` uncomputed layout values) targets
  `autolayout.h`; none of the named helpers exist there.
- [ ] **`rfc_afterhours_extractions.md`: 5 done, 1 partial, 5 open.** Done: files, i18n, settings,
  sound playback, camera. Partial: sound/music `Library<T>` (still in `src/library/`). Open: common
  components, `Transform`, UI-sound integration, shader pass registry, shader library. Two of its
  own constraints were violated by what shipped — it mandated header-only STB-style
  (`plugins/files.cpp` and `plugins/settings.cpp` exist) and a flat `afterhours_*.h` layout
  (everything went into `plugins/`).
- [ ] **i18n design limit.** Every format string uses positional `{}` while `translate_formatted`
  (`translation_manager.h:89-93`) does `fmt::vformat` with a *named* arg store, so `i18nParam` is
  inert and only one substitution per string is possible. Two call sites already misuse it
  (`ui_systems.cpp:1184-1187`, `:1257-1262`). Named placeholders are required for any language that
  reorders arguments.
