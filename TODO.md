# TODO

Findings from a full read of the project on 2026-08-29. Every item below was verified against
the source, not inferred — file:line references are accurate as of commit `28c5d21`.

Tiers are ordered by payoff. Tier 1 is things that are silently **broken** rather than unfinished.

`vendor/afterhours` is a git submodule shared with ~10 other projects in `~/p/`, so anything only
fixable upstream lives in Tier 4 and is recorded rather than acted on.

---

## Tier 1 — Broken

- [ ] **Menu and UI sound is dead.** `register_sound_systems` (`src/systems/sound_systems.cpp:159`)
  is defined and declared but never called — `main.cpp:18` includes the header, no call site exists.
  Silently disables `BackgroundMusic` (menu music never plays), `UISoundBindingSystem`,
  `UIClickSounds` and `CarRumble`. Lost in `e373360` when the sound system moved to an afterhours
  plugin. Weapon/honk/boost sounds still work — those go through `sound_system::PlaySoundRequest`,
  which *is* registered (`main.cpp:77`).
- [ ] **`CarRumble` has an empty body** — `src/systems/sound_systems.cpp:21-24`, `for_each_with`
  contains no statements.
- [ ] **E2E failures cannot fail anything.** `e2e_integration::has_failed()`
  (`src/e2e_integration.h:125`) is defined and never called. `main.cpp:240-243` prints results and
  returns 0. A failed `expect_text`, a `click_button` that hits nothing, a 30s timeout — all exit 0.
  `make e2e` is incapable of failing. Return non-zero from `main` when `has_failed()`.
- [ ] **Per-round state never resets.** `RoundManager::reset_for_new_round()`
  (`src/round_settings.h:336-341`) resets only countdown / round time / temp data on the *active*
  settings object. `HasKillCountTracker::kills`, `HasMultipleLives::num_lives_remaining`,
  `HasHippoCollection::hippos_collected` and `HasHealth::amount` are set once in `make_car`
  (`src/makers.cpp:262`) and never cleared. Cars outlive rounds, so round 2+ starts with
  carried-over scores.
- [ ] **Cross-mode UB in TagAndGo.** `HandleTagAndGoTagTransfer`
  (`src/systems/systems_tagandgo.h:27`) guards on `is_game_active()` and `is_tagger` but not on
  `active_round_type == TagAndGo`, then calls `get_active_rt<RoundTagAndGoSettings>()`.
  `get_active_rt` (`src/round_settings.h:215-232`) is a 4-way switch where every branch is the
  identical unchecked `static_cast<T&>` — it reads as type-checking and isn't. `is_tagger` is only
  cleared by `reset_temp_data()` on the active settings, so: play TagAndGo, switch to Lives, and a
  stale tagger makes this reinterpret a `RoundLivesSettings` as a `RoundTagAndGoSettings`.
- [ ] **Single-player Lives mode ends instantly.** `CheckLivesWinFFA`
  (`src/systems/systems_lives.h:32`) declares a winner at `players_with_lives.size() == 1`, and
  `main.cpp:94` always spawns exactly one player.
- [ ] **`MarkEntitiesWithShaders` is registered twice** — `src/main.cpp:151` and `:169`.
- [ ] **`num_starting_lives` is read at car-creation time** (`src/makers.cpp:265`), so changing the
  Lives setting after players are added has no effect on existing cars.
- [ ] **Settings only persist on exit.** `Settings::write_save_file()` is called from just
  `main.cpp:299` and `ui_systems.cpp:2400` (language change). Volume, fullscreen, post-processing,
  resolution and all round settings are lost on crash or force-quit.
- [ ] **`strings::pre_translation` is an unlinkable declaration.** `src/strings.h:82` declares it
  `extern`; nothing defines it. `strings::get_string()` (`:85-100`) reads it, so any call is a link
  error. Zero callers — delete both.
- [ ] **Japanese string contains a Korean character.** `src/translation_manager.cpp:337` uses
  `"鬼: {:.1f}초"`. `초` is Korean for "seconds"; Japanese wants `秒`. Copy-paste from the Korean
  table at `:231`.
- [ ] **Round-length dropdowns show raw enum identifiers.** `src/ui/ui_systems.cpp:1762` and `:1799`
  render `magic_enum::enum_names<RoundSettings::TimeOptions>()` directly, so the user sees
  `Seconds_10`, `Seconds_30`, `Minutes_1`.
- [ ] **Untranslated UI text** — renders English regardless of language: `"Team Mode"`
  (`ui_systems.cpp:1096`), `"No players"` (`:1042`), `"Score: "` (`:2528`), `"Team A"`/`"Team B"`
  (`:1151,1153`), slot labels (`:653-660`), round type names (`:1186`, `:1863`), weapon names
  (`weapons.h:197`), hardcoded `"10s"/"30s"/"1m"` (`:1240-1250`, `:1284-1294`).

## Tier 2 — Tests and CI

- [x] ~~Fix the e2e state leak~~ — done. `--e2e` now starts from `SettingsData` defaults and never
  writes the save file, so a screenshot depends on the build and nothing else. All 52 screenshots
  are byte-identical across runs. Also stops test runs clobbering the player's own settings.
- [x] ~~`screenshot-baselines/` has never existed~~ — done. 52 baselines committed (quantized,
  16MB), `make ci` passes 52/52, and verified it fails at 3.09% on an injected change.
- [ ] **Test 10 tests nothing.** `screenshots/anim_0{1..4}*.png` are byte-identical.
  `tests/e2e/10_slide_in_animation.e2e:2` claims animations are enabled, but
  `animation_control::disabled` is a plain global (`src/ui/animation_control.h:6`) never cleared
  between scripts, and 01–09 each call `disable_animations`. An `enable_animations` command exists
  (`src/e2e_commands.h:87`); script 10 never calls it.
- [ ] **`05_full_menu_flow.e2e` is mislabeled** — claims to test keyboard navigation but uses
  `goto_screen` for every transition, making it a duplicate of 01. This is Phase 5 of
  `docs/plans/2026-02-25-e2e-test-coverage.md`, still undelivered.
- [ ] **`validate-screenshots` and `update-baselines` never clean `screenshots/`** — only `e2e`
  depends on `clean-screenshots` (`makefile:62-65`), so `make e2e && make ci` copies stale PNGs.
  Less dangerous now that runs are deterministic (a stale PNG equals a fresh one unless the build
  changed), but still wrong across a rebuild.
- [ ] **Use the assertions that already exist.** Zero scripts use `set_slider`, `expect_slider`,
  `select_dropdown`, `expect_checkbox`, `expect_focused`, `expect_no_text`, `assert_no_overflow`,
  `dump_ui`, `enter`, `escape`, `key`, `drag`, `resize`. `assert_no_overflow` is free coverage for
  the checkbox-overflow gap the docs already describe.
- [ ] **No CI at all** — no `.github/`, `make ci` is local-only. Worth adding once baselines land.
  Note headless GL is macOS/Linux only (see Tier 4).
- [ ] **No unit tests anywhere.** Zero coverage of physics, weapons, AI, collision, win conditions.
  `automatic.md`'s one unfinished line was "add simple verification tests".
- [ ] **`compare_baselines.py` defects:** `--save-diffs` is `store_true, default=True` so it's
  always on (`:86`); a `manifest.json` `default_tolerance` silently overrides `--threshold`
  (`:105`); the JSON summary reports `args.threshold` rather than the effective per-screen value
  (`:174`). The per-screen `overrides` mechanism is implemented but unreachable — it lives in
  `screenshot-baselines/manifest.json`, which doesn't exist.

## Tier 3 — Cleanup

- [ ] **Delete dead files (~570 lines).** `src/multipass_integration.h` (0 includers),
  `src/multipass_renderer.h` and `src/shader_pass_registry.h` (only included by the dead ones),
  `src/utils.h` (0 includers; also defines a non-`inline` free function in a header),
  `src/log/log_fakelog.h` (0). `shader_pass_registry.h:27` declares
  `constexpr static std::vector<RenderPass>` as a class member — ill-formed, so it provably has
  never been compiled. Deleting these orphans the `entity_test` / `entity_enhanced` shaders loaded
  at `preload.cpp:103,114`.
- [ ] **Delete dead functions and systems (~200 lines).** `make_poof_anim` / `make_bullet`
  pre-`ProjectileConfig` overloads (`makers.cpp:34,83`), `Weapon::apply_recoil` (`weapons.h:69`,
  reimplemented inline at `systems.h:1261`), `CanShoot::fire` (`weapons.h:185`).
  Defined-but-never-registered: `RenderRenderTexture` (`systems.h:456`),
  `BeginPostProcessingShader` (`:475`), `RenderDebugGridOverlay` (`:674` — complete and working,
  just never registered).
- [ ] **~46MB of unused fonts tracked in git.** `preload.cpp:27-40` loads exactly two:
  `NotoSansMonoCJKkr-Bold.otf` and `NotoSansMonoCJKjp-Bold.otf`. Unreferenced:
  `Sazanami-Hanazono-Mincho.ttf` (30MB), `NotoSansKR.ttf` (10MB), `SymbolsNerdFont-Regular.ttf` and
  `-Mono-` (2.3MB each), `Gaegu-Bold.ttf` (3MB), `eqprorounded-regular.ttf` (94KB, commented out at
  `:30`). The four largest files in the repo. Deleting only helps future clones unless history is
  rewritten — decide which.
- [ ] **`SYMBOL_FONT` returns the Korean CJK font** (`preload.cpp:37`) rather than either bundled
  Nerd Font. Looks like a bug or a leftover.
- [ ] **Untrack files that shouldn't be tracked.** `compile_commands.json` — all 9 entries point at
  deleted xmake temp paths under `/var/folders/`, not one real source file, so clangd gets nothing;
  regenerate from zig. `settings.json` at root — the game's *save file*, committed, on an older
  schema, and dead (saves go to `~/Library/Application Support/Cart Chaos/`).
  `.vscode/{launch,settings,tasks}.json` — tracked despite `.gitignore:32`; `launch.json:11` points
  at `output/kart.exe` with a `C:\msys64\...\gdb.exe` debugger path.
- [ ] **`.gitignore` bug:** `settings.json` has no leading slash, so it matches at any depth and
  silently covers `.vscode/settings.json` and `.claude/settings.json`.
- [ ] **300MB of stale build artifacts on disk.** `output/` (143MB of pre-zig `.o` files plus a
  45MB leftover `output/kart.exe`), `output-win/`, `screenshots/` (48MB). All gitignored, but
  `make clean` only removes `zig-out` and `.zig-cache`, so they never go away.
- [ ] **`xmake.lua` is a stale third build system** — already broken, listing `src/*.cpp` and
  `src/ui/*.cpp` but missing `src/library/` and `src/systems/`, both of which have real `.cpp`
  files. `:87` has an `after_build` hook that launches the game; `:81` hardcodes
  `F:/RayLib/lib/raylib.dll`. Delete it along with `make xmake` and `make cba`.
- [ ] **`tools/dependency_baseline.json` is stale, so `make deps-check` fails on any tree.** It
  references `intro.h`, `systems_roundtypes.h`, `ui_button_wiggle.h`, `ui_slide_in.h` — all renamed
  or split — and records 64 systems where `src/` declares 59. Also: `deps-dot`/`deps-svg`/`deps-html`
  invoke `./dependency_graph` with no build dependency and it's gitignored, so they fail on a clean
  checkout; `deps-html` and `deps-dot` are byte-identical commands; `tools/dep_config.example.json`
  documents a `--config` flag that doesn't exist in `main()`.
- [ ] **`mcp.json` is broken** — points at `./output/kart.exe`, no longer a build output, and
  hardcodes an absolute `cwd` under `$HOME`, committed to git. Real path is `zig-out/bin/kart`.
  Worth deciding whether MCP and e2e both need to exist: both inject input, capture frames, and
  dump the UI tree.
- [ ] **`make output` never copies the executable** (`makefile:90-93`) — it creates
  `output/resources/` and runs `cp vendor/raylib/*.dll` (a Windows DLL, on macOS). That's why
  `output/kart.exe` only ever exists as a leftover.
- [ ] **`HOW_TO_PLAY.md` tooling is stale** — says `./output/kart.exe`; the path is now
  `zig-out/bin/kart`. `make MCP=1` still works. Gameplay sections are accurate.
- [ ] **Delete completed plan docs.** `CLEANUP_PLAN.md` — every item verified done in vendor
  (`ui/providers.h` gone, `ui/behaviors/` gone, `ui/immediate/` flattened,
  `ProviderConsumer`/`make_dropdown` zero refs). `automatic.md` — all 6 steps landed
  (`apply_automatic_defaults()` at `component_config.h:752`, presets at `:885`, grid snapping at
  `autolayout.h:231`). Both still present as TODO lists. `rfc_worker_threads.md` is a postmortem of
  a reverted attempt ("don't do this") that reads like an active RFC — retitle as an ADR.
- [ ] **Unreferenced vendored headers:** `vendor/claylib.h` (16KB, 0 refs, added in `0e0f9d0` and
  never used), `vendor/RaylibOpOverloads.h` (24KB, 0 refs).
- [ ] **Prune 24 remote branches**, most from 2024–2025 (`origin/box2d` and `origin/padding` from
  Dec 2024, a dozen `feature/*` and `cursor/*` from Aug 2025).
- [ ] **`.PHONY` is missing** `count`, `countall`, `cppcheck`, `cba`, `clean-cba`, `prof`, `leak`,
  `alloc`, `getxm`, `xm`, `brawlhalla`. Also `make brawlhalla` copies the binary over
  `F:\SteamLibrary\...\Brawlhalla.exe` — presumably a joke, still one typo from surprising someone.
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
