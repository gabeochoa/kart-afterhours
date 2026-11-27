# Afterhours bump: `fc4d625` → `5383fc6c` — onboarding

Status: **§2 landed.** The bump + motion migration + re-baseline are
committed (see §6 step 3). §3/P1 work follows in later commits. The
evidence for each §2 item is recorded inline below.

- Old: `fc4d6253` — "Merge origin/main" (the `12a4571 -> fc4d625` bump in
  `d5e9083`, then `d31bf73` adopted what it provided).
- New: `5383fc6c` — "Merge pull request #491 … fix/ci-raylib6-and-stubs"
  (`origin/main` tip at pull time).
- Distance: **1,955 commits** (`git -C vendor/afterhours rev-list --count
  fc4d625..origin/main`). Kart's own `main` was already up to date
  (`git pull --ff-only origin main` → "Already up to date").
- Source of truth for everything below: `vendor/afterhours/CHANGELOG.md`
  at the new tip (540 lines; the old tip's changelog was 83 lines).
  Changelog entries are quoted/paraphrased, not re-derived from commits.
- Baseline check: checking the submodule back out at `fc4d6253` and running
  `zig build` passes. At `5383fc6c` it fails with 70 errors (clang's
  per-TU cap; the real count in `ui_systems.cpp` is hidden behind it).

## 1. What was pulled

```sh
git pull --ff-only origin main          # kart: already up to date
git -C vendor/afterhours fetch origin
git -C vendor/afterhours checkout -q 5383fc6c
git status --short                      # M vendor/afterhours (uncommitted)
```

No kart source was changed by the pull itself. The submodule pointer was
left uncommitted until §2 compiled and §2.4 re-baselined; that commit
is the bump+migration commit.

## 2. Blocking migration todo (must-do, in order)

### 2.1 Rewrite kart's animation usage on `afterhours::motion` — REQUIRED

The animation plugin was rewritten and every old call is a compile error
(changelog: "The animation plugin is rewritten around `afterhours::motion`").
Kart uses the old API in exactly these places:

- `src/ui/animation_key.h:25-45` — `EasingType::Hold` / `EaseOutQuad` in
  `make_map_card_slide` / `make_round_end_card_stagger` sequence builders.
- `src/ui/animation_slide_in.h:99-251` — `animation::anim(UIKey, id)`,
  `.from().sequence({...})`, `animation::get_value(UIKey, id)`.
- `src/ui/animation_ui_wiggle.h:69-217` — same `anim` / `get_value` pattern,
  per-entity.
- `src/ui/ui_systems.cpp:797-888, 1318-1329, 2001-2171, 2202-2210, 2759-2761`
  — `one_shot`, `clamp_value`, `manager<UIKey>().get_value`,
  `animation::anim(...).to(v, dur, EasingType)`,
  `register_update_systems<UIKey>` / `<CompositeKey>`.

Mapping (from the changelog, verify against
`vendor/afterhours/src/plugins/animation/{store,track,timeline,spring,plugin}.h`
before writing code):

| Old | New |
|---|---|
| `animation::anim<Key>(k)` / `anim(k, id)` | `motion::anim(key, entity.id)` / `motion::anim(key)`; returns `Track<float>` |
| `.to(v, dur, EasingType)` / `.sequence({...})` | `.from(v).to(target, Spring::snappy())` for springs; `.to(target, Timeline{.keys={{0,0},{0.3f,1}}, .curve=curves::ease_out_quad})` for timed ease; `.then(...)` to chain; `.delay(s)` |
| `get_value` / `clamp_value` | `.value()` / `.value_or(d)` on the track; `.active()` instead of `is_active` |
| `manager<Key>().get_value` / `one_shot` | track on the entity (`motion::HasTracks`); no manager, no `clear_all` |
| `register_update_systems<Key>(sm)` | `animation::register_update_systems(systems)` once, no key type |
| `Spring::from_freq_decay(freq, decay)` | reproduces the old UI spring numbers |
| Widget sugar | `.on_hover({.scale=1.05f}, Spring::snappy())`, `.on_press`, `.on_focus`, `.on_appear({.opacity={0.f,1.f}})`, `.on_state(bool, ...)`, `.on_change(stamp, ...)` |
| `animation::set_instant` | unchanged; `src/ui/animation_control.h:14` (`disabled()`) keeps working |

Notes: tracks live on the entity and die with it. `Track::reverse()` /
`seek(t)` and `motion::Reveal` now exist for close-transitions (see §3).
Keep the staggered delays (0.02–0.05 s/row, 0.18–0.25 s eases) — re-derive
them as timelines, don't drop them to springs silently, or the slide-in
and round-end cards change feel. E2E script `tests/e2e/10_slide_in_animation.e2e`
already depends on this behaviour (and on `enable_animations`, which the
changelog says kart lost to a shadowing handler — see §2.5).

### 2.2 Delete kart's `distance_sq` — REQUIRED, one line

`src/math_util.h:13` defines global `distance_sq(vec2, vec2)`. Upstream now
defines `afterhours::distance_sq(Vector2Type, Vector2Type)` with a global
re-export (`vendor/afterhours/src/developer.h:231-235`), and kart's copy is
a hard error ("declaration conflicts with target of using declaration").
Delete kart's definition; all call sites (`src/query.h`, `src/systems.h`,
`src/systems/systems_ai.h`, etc.) keep working unqualified. `vec2` here is
`raylib::Vector2` = `Vector2Type`, so there is no overload distinction to
preserve.

### 2.3 Upstream warnings that are errors under kart's flags — investigate, don't blanket-suppress

`zig build` at the new tip also fails inside vendored code (full list from
the error dump; baseline tip builds clean, so these are new):

- `vendor/afterhours/src/plugins/e2e_testing/runner.h:775`,
  `perf_commands.h:211` — dangling-`else` (add explicit braces).
- `vendor/afterhours/src/plugins/settings_json_preserve.h:146` — unused
  variable `j`.
- `vendor/nlohmann/json.hpp:24392-24465` — deprecated literal-operator
  whitespace (`operator "" _json`), surfaced through the new
  settings-preservation include chain.

Fix upstream if the fix is accepted there (preferred: the dangling-else
and unused-variable ones are one-liners); otherwise add only the narrow
`-Wno-…` flags for those diagnostics in `build.zig`'s `NOFLAGS`, with a
comment naming the upstream file. Do not add `-w` / `-Wno-everything`:
it would hide kart's own warnings too.

### 2.4 Re-baseline screenshots — REQUIRED after 2.1–2.3

Visual changes that move pixels even with no kart code change (changelog
"Visual — re-baseline" + "Breaking"). Expect several of the 52 baselines
in `screenshot-baselines/` to move; re-run the suite and read the new
failures before re-baselining (`make ci`, then `make update-baselines`
only for diffs that are the documented change):

- **Default font is now Roboto Regular**, not raylib's pixel font
  (`get_default_font()` returns bundled `Roboto-Regular.ttf`). Kart loads
  its own CJK fonts in `preload.cpp:27-40`, so screens using those faces
  move less — but any screen falling back to the default face, and all
  text metrics, change.
- **Labels lost their free 5px inset**: `Theme::text_inset` defaults to 0
  (was hardcoded 5). Edge-aligned text shifts, wrapped text re-breaks,
  auto-fit grows. Restore per-widget with `with_text_inset()` or build
  `-DAFTERHOURS_DEFAULT_TEXT_INSET=5.f` only as a temporary hold.
- **Labels clip to their own box**; wrapped lines that don't fit are
  dropped instead of spilling over the widget below.
- **Scroll views draw a draggable scrollbar** on overflowing axes
  (`show_scrollbar=false` opts out).
- **Cross-axis `expand()` stretches** instead of collapsing to 0;
  previously invisible separators/fills now have size.
- **Cross-axis overflow is centred/aligned**, not dumped at `FlexStart`.
- **Scroll-view children are laid out by autolayout** (margined children
  move by one margin; `justify`/`align`/`gap` now honoured).
- **Dropdown trays / menus / popovers render in a reserved overlay band**
  (`OVERLAY_BASE`, 1000 over the anchor), so popups no longer lose to a
  `text_area` built after them.
- **Menu rows**: shortcut + label each sit one pad in from the edge;
  rows take `with_padding`, background usage, hover colour.
- **Toasts wrap, dismiss on click, pause on hover, merge repeats, cap at 3.**
- **A word wider than the wrap width breaks between grapheme clusters**
  instead of overflowing/clipping (long paths/URLs/hashes).
- **Default font loads rasterize at display pixels** (hidpi atlas scaling);
  **glyph fallback across loaded faces per codepoint**; uncovered
  codepoints draw U+FFFD/`?` instead of blank.
- **`Dim::Percent` charges the child's own margin**; **grid-snap extents**
  follow snapped placement (containers grow a few px).
- **`Dim::Text` measures the weighted face + letter spacing** actually
  drawn, so bold/letter-spaced labels get wider.

### 2.5 Behaviour/breaking audit — check each, most are no-ops for kart

Tick each after reading the changelog entry; evidence of kart exposure
is noted where found by grep at the old tip:

- [x] **`Margin` field order** is now `{top, left, bottom, right}`.
      Kart's uses are all designated initializers
      (`src/ui/ui_systems.cpp:408, 1098, 1738, …, 2675`) — safe, but grep
      again for any positional `Margin{a,b,c,d}` before closing.
- [x] **Explicit `Padding{}` now means zero padding**; only unset padding
      takes the widget default (`padding_explicitly_set` flag). Kart uses
      designated `Padding{.top=…}` everywhere found — safe, unless some
      site passes `Padding{}` expecting the default.
- [x] **`HasLabel::font_name` deleted** — read/set on `UIComponent`.
      Kart never touches `HasLabel::font_name` (its `HasLabels` in
      `components.h:377` and `RenderLabels` are kart's own) — likely no-op.
- [x] **Tab order follows the UI tree**, not entity allocation order;
      initial grab takes the first element in tree order. E2E scripts
      pressing Tab N times (e.g. `02_menu_navigation.e2e`,
      `04/05_*.e2e`) may need recounting. Focus scopes (new, §3) land in
      the same change — absorb both at once.
- [x] **Escape has one owner per press**: field blur consumes `MenuBack`;
      modal close defers to open menu/popover/dropdown/field; stacked
      Escape peels topmost-first. Kart relies on Escape for pause/menu
      back — play-test one press per layer.
- [x] **E2E name/text lookups fail on ambiguity** (`click_ui`, `click_text`,
      `expect_focused`, `assert_ui`, …). Scope with trailing
      `under=<debug_name>`. Kart's scripts use repeated labels (menu
      rows, map cards) — expect new failures; scope them, don't rename
      widgets to dodge it.
- [x] **E2E commands run serially**; a handler on only one of several
      `SystemManager`s now times out in the others instead of passing
      against a later state. Register handlers everywhere needed, or in
      a shared pack (`register_shared_handlers`, §3).
- [x] **An injected press no system read now fails the step**
      ("injected press … was never read"). Expect new failures for
      presses aimed at an inactive state — most mean the script is
      wrong, per the changelog.
- [x] **A held gamepad axis fires `inputs_pressed` once**, on deadzone
      crossing, not every frame. Kart reads `inputs_pressed()` in
      `src/ui/navigation.cpp:58,70`, `src/ui/ui_systems.cpp:665,1649`,
      `src/systems/systems.h:372,685,1297,1692` — audit any site that
      used it for continuous stick/trigger movement; those must read
      `inputs()`.
- [x] **`tunable<T>` moved to `afterhours::developer::`** — kart has no
      `tunable` use found; no-op if confirmed.
- [x] **`RNGStream` / `RandomEngine` draws are bit-stable across STLs,
      every value changes.** Kart has no `RNGStream`/`RandomEngine` use
      found; no replays/golden draws to re-baseline. Confirm, then close.
- [x] **`SystemProfileHook::end` takes entities visited** (third `int`
      param). Kart has no custom profile hook found; no-op if confirmed.
- [x] **`colors::to_hsl` / `to_rgb` / `get_highlighted` deleted**; use
      `to_hsv`/`from_hsv`/`lighten`. No kart use found.
- [x] **Non-raylib stub audio functions deleted** — raylib build, no-op.
- [x] **`HasScrollView::viewport_size` is `std::optional`**; use
      `viewport_or_zero()`. Kart has no `viewport_size` use found.
- [x] **`with_letter_spacing(0)` clears inherited spacing**;
      `config.letter_spacing` is now `optional<float>`. No kart use
      found; direct reads would be a compile error, so the build
      decides this one.
- [x] **Ctrl+Shift+Z no longer also fires Ctrl+Z** (most-specific chord
      wins). Kart's chords: verify none relied on both firing.
- [x] **`with_disabled(true)` now also blocks clicks on controls whose
      label lives in child labels** (via `IsDisabled` / `ui::is_disabled`).
      Kart's disabled cards/rows may newly stop receiving clicks they
      previously got — check the settings/character-creation screens.
- [x] **Settings files keep comments** (see §3): a load→save cycle now
      patches only changed values. Kart's `Settings::write_save_file`
      (`src/settings.cpp:67`) rewrites the whole file today; confirm the
      new behaviour doesn't fight kart's round-settings JSON blob.
- [x] **`animation::set_instant` unchanged**; scroll easing now snaps
      under instant motion. `10_slide_in_animation.e2e` + the
      `disable_animations` shadowing fix (changelog: kart "lost its own
      `disable_animations` … and nothing said so"; a handler registered
      after the built-ins now warns) — re-check that script actually
      disables animations after the bump.

## 3. Full changelog inventory, filtered

Legend: **LEVERAGE** = kart has the problem / hand-rolled equivalent /
direct use case, with evidence. **ADOPT-IF-NEEDED** = real API, no
current kart need — don't build a feature around it. **—** = no kart
relevance found. **BLOCKING** = §2. Everything below is in changelog
order within its section, so it can be ticked against
`vendor/afterhours/CHANGELOG.md` at `5383fc6c`.

### 3.1 New APIs in the top "Unreleased" block

- [ ] **LEVERAGE — `imm::prompt_bar` / `imm::keycap`** (`PromptItem{key,label,on_press}`).
      Kart has no on-screen key-prompt bar; every screen re-teaches its
      keys in prose (see `HOW_TO_PLAY.md`). Add one bar to the pause /
      round-end screens first; don't retrofit all screens at once.
- [ ] **— Subtree rotation inheritance** (rotated container rotates
      subtree: rendering, images, focus rings, hit-testing). No rotated
      UI in kart.
- [ ] **— Window events queue** (`Resized`, `ScaleChanged`, `Minimized`,
      …). Kart reads resolution via its singleton; no need found.
- [ ] **— `text_input` without host `Text*` enum names.** Kart's input
      mapping already supplies enums; no-op.
- [ ] **LEVERAGE — `virtual_list_keyed` / `virtual_list_range`, logical
      scroll anchors by key.** Kart's lists are short (maps, players,
      settings rows — all built in full). No 10k list. Only revisit if
      a long list (e.g. replays / leaderboard) appears. The underlying
      `virtual_list` culling fix (§4) is free either way.
- [ ] **— Glyph atlas-packing failure reporting** (`FontManager::packing_failures`).
      Diagnostic only; check once if CJK glyphs ever go missing.
- [ ] **— Flex shrink content floor.** Free layout fix; no action.
- [ ] **— Drag-group reorder Escape-cancel + edge autoscroll.** Kart has
      no drag-reorder UI.
- [ ] **— `with_text_offset(x, y)` / fixed right/centre-aligned offset.**
      No text-offset use in kart.
- [ ] **— Scale-aware `on_draw_bg`/`on_draw_fg`** (`DrawContext{rect, scale}`).
      Kart's custom draw callbacks: check if any assume unscaled rects;
      otherwise no-op.
- [ ] **— Word selection policy** (`WordPolicy::Prose/Identifier/Path`).
      Kart has no text editing beyond fields.
- [ ] **ADOPT-IF-NEEDED — Global text scale** (`Theme::text_scale`).
      Multiplies every resolved font size without scaling boxes. This is
      the accessibility / big-text setting kart doesn't have; a settings
      toggle for it is a one-field change when wanted. Don't add the
      toggle in the bump PR.
- [ ] **— Rotated hit testing / `with_rotation(deg)`.** No rotated UI.
- [ ] **— Font unload / unbounded Sokol font tracking.** Raylib build.
- [ ] **— Row baseline alignment** (`AlignItems::Baseline`). Free if
      kart rows mix text sizes; no row found that needs it.
- [ ] **LEVERAGE — `imm::disclosure`** (expand/collapse section, chevron,
      persists state). Kart's settings screens are long flat lists
      (`ui_systems.cpp` settings sections). Use for advanced round
      settings / debug sections when one is next edited — not a
      wholesale restyle.
- [ ] **— `imm::scroll_to_item(list, index)`** for `virtual_list`. See
      virtual-list row above.
- [ ] **— `imm::icon_row` config forwarding.** Kart uses `icon_row`
      (7 uses); forwarding is additive, no change needed.
- [ ] **— UTF-8 ingest sanitization.** Free hardening for paste/external
      text; no action.
- [ ] **LEVERAGE — Versioned storage codecs** (`storage::Codec<T>`,
      migrations, `DecodeStatus::{Ok,Migrated,Corrupt,TooNew}`,
      `peek_version`). Kart's save is unversioned JSON
      (`SettingsData::from_json` uses `.at()`, throws on missing fields;
      `TODO.md` Tier 1: settings only persist on exit). Adopt for the
      save format on the next save-format change — stamp a version now
      if touching `settings.cpp` anyway, so a future migration has
      something to key on. Don't migrate the existing file format in
      the bump PR.
- [ ] **— Styled labels honour `TextOverflow::Ellipsis`.** Free; kart
      has no styled-label ellipsis use found.
- [ ] **— Generation-checked focus restore** (`capture_focus()` /
      `restore_focus(handle)`). Kart's modals/menus: adopt if a
      focus-loss-on-close bug is ever filed; not a proactive rewrite.
- [ ] **— `imm::popover` runs its body on the dismissal frame.**
      Free fix; no action.
- [ ] **ADOPT-IF-NEEDED — `EntityHelper::ScopedCollection`.** Only
      useful for multi-collection code; kart is single-collection
      (`AFTER_HOURS_UI_SINGLE_COLLECTION`).
- [ ] **— `tab_container(..., active_config)`.** Kart has no tab
      container use found (its "tabs" are separate screens).

### 3.2 Behaviour block

- [ ] **— Bundled default font reloads across GL context shutdown.**
      Diagnostic/free; see §2.4 for the font change itself.
- [ ] **— E2E mouse-down polls mark injected presses read.** Free.
- [ ] **LEVERAGE — E2E bus-aware volume commands** (`set_volume` /
      `expect_volume`). Pairs with §3.4 audio buses; add to a settings
      e2e script once buses land, not before.
- [ ] **LEVERAGE — E2E UI lookups wait for a render checkpoint.**
      Free reliability for kart's batched-tick scripts; re-run the
      suite and delete any `wait`-based workarounds only if a script
      actually has one — don't go hunting.
- [ ] **— Default UI font Roboto.** See §2.4.

### 3.3 "Added" block (glow / prompt / settings-comments / fades)

- [ ] **ADOPT-IF-NEEDED — Soft glow** (`with_glow(color, radius)`,
      `draw_radial_glow`). Kart's Memphis/menu UI (`feature/memphis-menu-ui`
      branch history; `ui_systems.cpp` menu styling) is the one place a
      glow belongs — hover glow on the selected menu card, if a visual
      pass ever happens. No gameplay use.
- [ ] **— E2E `screenshot_fresh`.** Kart's screenshot plumbing is in
      `e2e_integration.h:96-114` (hand-rolled render-texture copy).
      Prefer `expect_screenshot` (§3.5) over adopting `screenshot_fresh`
      alone; don't run both capture paths.
- [ ] **— Free `has_singleton<T>()` / `get_singleton<T>()`.** Cosmetic;
      adopt opportunistically when editing a call site.
- [ ] **LEVERAGE — Settings files keep their comments** (JSON `//`/`/* */`
      comments + trailing commas; load→save patches only changed values).
      Kart's save file is machine-written JSON today, so the immediate
      win is nil — but it unblocks hand-editable settings / documented
      defaults. Note it in `settings.cpp` when next touched; don't
      reformat the save file for it.
- [ ] **LEVERAGE — Loop fades for sounds and music** (`fade_to`,
      `stop_with_fade`, `set_fade_level` on `SoundLibrary`/`MusicLibrary`).
      Directly serves the dead menu-music path: `src/systems/sound_systems.cpp:102-108`
      plays `menu_music` with no fade. Pair with §3.4 (buses) and the
      Tier-1 "menu sound is dead" fix (`register_sound_systems` never
      called, `TODO.md` Tier 1) — fades are not a substitute for
      registering the systems.

### 3.4 Performance / internal block

- [ ] **— Membership lists rebuild per component; queries/systems walk
      per-component membership lists; `gen_count()` streaming.** All
      automatic. Kart has ~102 `whereHasComponent`/`for_each` sites;
      this is the bump's free perf win. No code change; no benchmark
      needed unless a frame profile says otherwise.
- [ ] **— `TrackIfComponentWillBeRendered` / `ComputeVisualFocusId`
      require only `UIContext`.** Automatic.
- [ ] **— Snapshot `DTO` → `Serializable` rename.** Kart has no snapshot
      use found.
- [ ] **LEVERAGE — E2E `reset_profile` + `dump_profile --json`.**
      Kart has no profiling workflow in-repo; adopt only if a perf
      investigation starts. The JSON form is the machine-readable one.
- [ ] **— Sokol MSAA render targets.** Raylib build; no-op.
- [ ] **— Theme-bypass lint.** Upstream-only script; kart doesn't run
      afterhours' `make check-boundaries`. Skip.
- [ ] **LEVERAGE — `EventQueue<Event, Phase>`** (ordered phases, stable
      within phase, replay/netcode-safe). Kart's weapons/events are
      direct system calls (`components_weapons.h`, `systems.h` weapon
      handlers) with no event queue. Only adopt if a replay/netcode or
      ordered-phase bug appears; do not refactor weapons onto it
      speculatively.
- [ ] **LEVERAGE — Settings presets that flip to Custom**
      (`SettingsPreset`, `active_preset_name`, `settings_preset_stepper`).
      Kart's round settings are a preset-shaped problem: round type +
      lives + time options edited independently, with no "Custom"
      indicator (`ui_systems.cpp:1762/1799` dropdowns, `round_settings.h`).
      When the round-settings screen is next edited, derive the stepper
      from the values struct instead of hand-tracking preset identity.
- [ ] **— E2E `expect_sound_level` / `expect_sound_pan`.** See §3.5.

### 3.5 New APIs block (the long one)

In changelog order; the LEVERAGE items are the ones with a kart call site:

- [ ] **LEVERAGE — Sound closed captions** (`set_caption`, `last_caption()`).
      Accessibility text for sounds, no rendering. Kart has no captions;
      cheap to add for honk/boost/weapon sounds later. P3.
- [ ] **LEVERAGE — `settings_table.h`** (`settings_fields<T>`,
      `settings_to_json` / `settings_from_json`, generic
      `set_setting`/`expect_setting` e2e commands). Replaces kart's
      hand-written `SettingsData::to_json`/`from_json`
      (`src/settings.h:25-93`: nested `audio` object, `.at()` throws,
      `Pct` wrappers). This is the highest-value settings adoption:
      missing fields keep their value, unknown fields ignored, type
      mismatches fail before assignment. Blocked on the format
      decision in §3.1 (storage codecs): do the table + version stamp
      together, one save-format change, not two. `round_settings`
      stays a raw `nlohmann::json` blob (nested structs keep hand-written
      ADL per the changelog).
- [ ] **— Gesture availability API.** No pinch UI in kart.
- [ ] **LEVERAGE — Generic 3D E2E commands** (`set_camera`/`expect_camera`,
      `set_player_pos`/`expect_player_pos`, `raycast`/`expect_hit`).
      Kart is a 2D top-down racer; camera/player pose are 2D transforms.
      Only adopt if a 3D assertion is ever needed — the 2D equivalents
      would be hand-rolled. P3 / probably never.
- [ ] **LEVERAGE — `--time-scale` fixed-step wiring** (`apply_time_scale`,
      `run_fixed_steps`). Kart's sim is variable-dt; check `main.cpp`
      tick structure before claiming this fits. Investigate, don't
      assume: if kart has no fixed step, this is a —, not a LEVERAGE.
- [ ] **LEVERAGE — E2E `resize_steps` / `wait_resized`.** Kart has a
      resolution dropdown (`ui_systems.cpp:2329`) and resize-sensitive
      layout; a resize e2e script is currently missing. Add one script
      exercising 1280×720 → 1080p → back, asserting `expect_window_size`.
      (Changelog notes a real script bug of exactly this shape upstream:
      a script that resized to 1080p and never resized back.)
- [ ] **LEVERAGE — Parallel multi-process E2E runner**
      (`tools/e2e_parallel_runner.py`, script headers in
      `script_headers.h`). Kart has 10 e2e scripts run serially via
      `make e2e`. Adopt the runner + headers when suite time hurts;
      first add the headers (`timeout:`, `requires:`, `batch:`) so
      scripts declare needs — the single-process runner ignores them.
- [ ] **LEVERAGE — `UIContext::is_hovered/is_focused/is_focus_visible(name)`.**
      Kart's render/game systems that know a widget's debug name but not
      its entity currently re-derive hover/focus by hand (check
      `mcp_integration.h:64` label lookup and menu hotspot code). Adopt
      at the next such site; don't sweep.
- [ ] **LEVERAGE — Accessible names/roles/values** (`with_accessible`,
      `HasAccessibility`, `expect_accessible`, `dump_ui` role/name/value).
      Kart's icon-only buttons (image buttons, `imm::image_button`) are
      exactly the wordless controls this audits. Add
      `expect_accessible` assertions for the menu's icon-only controls
      in one e2e script; no platform AT bridge exists yet, so this is
      test semantics only.
- [ ] **— Presented-frame capture** (`capture_presented_frame`,
      `screenshot_presented`). Kart's screenshots capture the canvas
      render texture (`e2e_integration.h`). Only adopt if a letterbox/
      window-chrome assertion is needed.
- [ ] **— `snapshot_to_collapsed` (flamegraph folded stacks).** Only
      with a profiling workflow (§3.4).
- [ ] **— Raylib font-load segfault fix, draggable-parent/button
      arbitration, drag-group scrolled-strip hit-testing, crash-handler
      re-entry fix, `with_disabled` (see §2.5), menu text-colour
      inheritance, `Dim::Text` weight/spacing, chord-modifier reader,
      Ctrl+Shift+Z specificity, pedantic `graphics_common.h`, default-font
      reach, clang/MSVC-STL fix, NONE-backend text estimate,
      `translation.h` `<cstdlib>`, letter-spacing-zero, CJK 4-byte
      decode, `SKIP_ENTITY_QUERY_MODIFICATIONS` build fix, wheel
      hit-testing + Shift+wheel horizontal scroll, sokol frame dt,
      atomic file writes, Metal screenshot leak, particle drag,
      scroll-easing instant snap, scheduled-toast fade, sokol headless
      `run()`, `Library::load` duplicate leak, const `should_run`
      gating, instanced-draw VAO cleanup, nested popover first-frame
      anchor, font glyph coverage, full-atlas warning, sokol GPU pool
      sizes, headless-resize Metal leak, `has_child_of` cast cut,
      text-measure memo, `imm::mk()` no-alloc.** All free fixes; no
      kart action except: delete any kart workaround for the wheel /
      scroll-view / toast items if one is found (none found by grep;
      `floatinghotel`/`hanabi` workarounds named in the changelog are
      not kart's), and note `files::write_string_atomic` durability
      for the save file (§3.1 codecs).
- [ ] **LEVERAGE — `capture::enable()` + `expect_drawn` / `expect_not_drawn`
      / `expect_drawn_at` / `dump_draws`** (draw attribution by
      `entity_id`/`layer`, registered in `register_ui_commands`).
      Kart's e2e asserts on text and UI rects only; nothing asserts
      that a world draw happened. This is the missing assertion class
      for weapons/particles/map rendering. Add one script asserting a
      known world draw (e.g. a car sprite) after the bump, using the
      frame-boundary semantics (§3.6 capture boundary). P1 after §2.
- [ ] **LEVERAGE — Focus origin reporting** (`dump_focus`, `expect_focused`
      names the actual holder). Free debugging for the Tab-order change
      (§2.5); use while re-counting Tab scripts.
- [ ] **— `with_corner_radius(px)`.** Kart uses `with_roundness`
      fraction; adopt per-widget only where a pixel radius is meant
      (check `diff_renderer`-style sites — kart has none named in the
      changelog warning, which names floatinghotel files).
- [ ] **— `imm::mk_keyed`.** See virtual-list row (§3.1).
- [ ] **— `HasScrollView::anchor_scroll`.** See virtual-list row; only
      for prepend-above-viewports.
- [ ] **— `measure_config` / public text measuring** (`measure_text_wrapped`,
      `wrap_text`). Kart sizes text via the UI itself; adopt if a
      windowing/variable-height list is built (see `measure_config`
      rationale). No current site.
- [ ] **— `animation::set_instant` / `clear_all` / e2e
      `disable_animations`.** `set_instant` unchanged; the e2e
      commands are the ones §2.1/§2.5 already cover.
- [ ] **— `expect_text_i`.** Add to a script only where case-insensitive
      matching is genuinely wanted.
- [ ] **— Trackpad pinch** (`get_pinch_delta`, `is_pinching`, macOS
      gestures opt-in). Kart has no zoom. —.
- [ ] **— Right-click** (`UIContext::is_right_click`, e2e `right_click`).
      Kart has no context-menu/right-click gameplay; the e2e
      `right_click_ui`/`right_click_text` (§3.6) matter only if one is
      added.
- [ ] **— Synchronized scroll views** (`sync_group`). Kart has no paired
      scroll views.
- [ ] **LEVERAGE — `whereIndexed<C>(key)` / `add_index<C>(key_fn)` /
      `indexed<C>(key)` + `invalidate_indexes()`.** The changelog's
      motivating query — "the children of this entity" — matches kart's
      `parent_id` component (`components.h:394-402`, projectiles/absorbers
      keyed by parent). Before adopting: profile. Kart's entity counts
      per frame are small (cars + projectiles + pickups); an index pays
      off after 2 lookups/rebuild and costs a rebuild on collection
      change. Only add an index if a scan shows up in a profile, and
      remember the loud rule: assigning an indexed field needs
      `invalidate_indexes()`, or the index silently goes stale.
      The automatic `whereID` O(1) seeding and membership-list walks
      (§3.4) already cover the common case for free.
- [ ] **— `developer::allocation_counters()` / `feed_alloc_counters`.**
      Only with a profiling workflow.
- [ ] **— Profiling spike ring** (`spike_threshold_ms`, `Collector::spikes()`).
      Only with a profiling workflow.
- [ ] **LEVERAGE — `add_singleton<Component>(args...)`.** Replaces the
      four-call sequence kart hand-rolls in `preload.cpp:190-230`
      (`createPermanentEntity` + `addComponent` +
      `registerSingleton` + `merge_entity_arrays` for input, window
      manager, translation, texture, sound, camera singletons).
      Mechanical, low-risk, do it in the bump PR once §2 compiles.
- [ ] **— `core/math3d.h` Vec3/Quat/Mat4.** Raylib-only 3D math; kart
      is 2D. —.
- [ ] **— Sokol font atlas shrink / atlas bytes.** Sokol build; —.
- [ ] **ADOPT-IF-NEEDED — Raw `AudioStream` callback wrapper.**
      Procedural audio; kart has none. Deferred upstream: playback
      position query is still missing (R-152).
- [ ] **LEVERAGE — Interned asset handles** (`Library::intern(name)` →
      `Handle`, `get(Handle)`). Kart looks up sounds/music/models by
      string name per frame in hot paths (`systems.h:1273-1275`,
      `intro.cpp:62-72`, `sound_systems.cpp`). Intern once at load,
      look up by integer per frame. Small, mechanical; do it for the
      sound/music libraries in the audio pass (§3.5 buses), not
      separately.
- [ ] **— `attachment.h`** (single-parent attach/detach/world-pose).
      Kart's parent relation is the `parent_id` field + ad-hoc code;
      only migrate if that code grows a second relation (sockets,
      cycle checks).
- [ ] **— Canonical snapshots** (`snapshot::canonicalize`,
      `first_difference`). Only if deterministic replay/checksum work
      starts; kart has none.
- [ ] **LEVERAGE — `with_slider_style`** (`SliderStyle{track,fill,handle,
      handle_size, track_thickness}`). Kart hand-styles its volume
      sliders (`ui_systems.cpp:462-469` `create_volume_slider`, used
      at :2294-2321). Adopt to delete the overlay/track workaround
      pattern if kart has one — verify the current slider is the plain
      25%-block default the changelog describes before rewriting it.
- [ ] **— `crash::install(report_path)`** (crash report file + 48-line
      log tail). Kart has no crash handler installed (check `main.cpp`
      before claiming); adding it is a one-call reliability win for
      playtest reports. P2, needs a `report_path` decision (save dir).
- [ ] **LEVERAGE — Audio buses, positional `play_at`, procedural sounds.**
      The big audio adoption, in one pass with fades (§3.3) and interned
      handles (above):
      - Buses: `load_on_bus`, `add(name, sound, bus)`,
        `load_pool(file, name, copies, bus)`, `set_bus_volume(bus, v)`;
        dispatched gain = master × category × bus. Kart's three
        volumes (`SettingsData`: master/music/sfx, `settings.h:25-93`)
        map onto buses — but don't rewire settings until
        `settings_table.h` lands (§3.5), or the volume plumbing gets
        changed twice.
      - Positional: `set_listener` + `play_at(name, position)` with
        inverse-distance falloff + stereo pan; `spatial_gain()` for the
        math. Kart's weapon/honk/boost sounds (`systems.h:1273`,
        `systems_common.h:31`) are all full-volume today; `play_at`
        from the firing car's position is the gameplay win.
      - Procedural: `load_sound_from_samples` / `add_samples` — no
        current need; note only.
      - Deferred upstream (don't plan around): sokol audio backend,
        looping ambient sources tied to entities, playback-position.
- [ ] **LEVERAGE — Audio test recording log + E2E sound commands + WAV export**
      (`set_recording`, `play_log()`, `expect_sound_played`,
      `export_wav`). Kart has zero sound assertions in e2e today, and
      the Tier-1 dead-menu-sound bug survived exactly because nothing
      asserted a play. Add `expect_sound_played` for menu music / a UI
      click to the menu-flow script as the regression test for the
      Tier-1 fix. This is P1, gated on registering the sound systems.
- [ ] **— Focus scopes** (`with_focus_scope`, nested scopes, tab-order
      exposure). Kart's screens are single-pane; the pause/menu split
      doesn't need scoping today. Revisit if a dialog-over-game screen
      traps Tab wrongly after §2.5.
- [ ] **— `with_single_line()` for `text_area`.** Kart has no text-area
      use found.
- [ ] **— Pointer ownership** (`claim_pointer_ownership`). Native-menu
      use case (hanabi); kart has none.
- [ ] **LEVERAGE — `expect_screenshot` E2E command**
      (`HandleExpectScreenshotCommand`, 0.05% / 8-channel threshold,
      `--update-baselines`). Kart's screenshot comparison is the
      hand-rolled `compare_baselines.py` + `make ci` flow over 52
      baselines (`TODO.md` Tier 2, "verified it fails at 3.09% on an
      injected change"). `expect_screenshot` is the in-engine
      equivalent — but kart's baselines already exist and pass; do
      NOT migrate the whole suite. Use it for new screens only, or
      not at all. Decide explicitly; the two systems must not both
      own the same baselines.
- [ ] **— `pathfinding::set_neighbors_fn / set_distance_fn / set_step_cost_fn`.**
      Kart's AI (`systems_ai.h`) steers directly, no grid pathfinding.
- [ ] **— `with_on_draw_over(fn)`.** Kart has no overlay-draw
      workaround found (the changelog's hanabi trailing-overlay-entity
      pattern is not kart's). —.
- [ ] **— `trigger_volume` plugin.** Single-player AABB volumes with
      enter/exit edges, `phase()`, `presence`, `progress()`. Kart's
      zones are map geometry / collision; only adopt if a capture-zone /
      king-of-the-hill mode is added. Note: multiplayer replication is
      explicitly the game's job.
- [ ] **— Window fit/centre/letterbox geometry helpers**
      (`fit_window_size`, `window_centre_position`,
      `letterbox_content_size`). Pure math; kart's window manager
      already owns letterboxing. Adopt only when hand-derived geometry
      is next touched.
- [ ] **— `rect::inset/cut/align_within/intersection/bounding/contains/is_empty`.**
      Kart has `rect_center`/`calc`/`is_point_inside` in `math_util.h`;
      adopt `align_within`/`cut` opportunistically, don't rewrite
      `math_util.h` for it.
- [ ] **— `imm::label` / `label_muted`.** Kart hand-rolls div+label+font
      chains; the changelog cites 97 wm screens doing the same. Adopt
      in new UI code; sweeping existing labels is churn.
- [ ] **— Blob shadows / `WorldLabel` / `draw_world_label`.** 3D /
      nameplate features; kart is 2D top-down with its own
      `RenderLabels` (`systems.h:2008`). —.
- [ ] **— Wrapper holes round 2** (`make_image`, `encode_image`,
      `export_image`, `window_state()`, test-aware `get_key_pressed`,
      right-button hold). `export_image`/`encode_image` may simplify
      kart's screenshot path (`e2e_integration.h`, `mcp_integration.h:27`);
      check before adopting — don't keep two PNG writers.
- [ ] **— `Entity::toggleComponent<T>(bool)` / `toggleTag(tag, on)`.**
      Mechanical cleanup for kart's add/remove pairs; grep for
      `addComponentIfMissing` + `removeComponentIfExists` pairs and
      convert opportunistically. Low priority.
- [ ] **— Blend modes in batched UI renderer** (`with_blend_mode`).
      No blend-mode UI in kart found.
- [ ] **— `Entity::user_data` (uint64_t stamp, shares `id`'s cache line).**
      Perf micro-opt; only if a per-frame stamp scan shows up.
- [ ] **— `ByteCache<K,V>`.** Kart has no byte-budgeted cache; the
      token/blob caches cited are floatinghotel's. —.
- [ ] **— `resolved_text_origin`.** Only for caret/IME overlays; kart
      has none.
- [ ] **LEVERAGE — `HasScrollView::scroll_to` / `ui::scroll_to_entity`.**
      Kart's scroll views (settings lists, map selection) have no
      scroll-to-item behaviour; adopt when keyboard-focus reveal or a
      "jump to selected map" is wanted. The focus-reveal use is
      already wired internally.
- [ ] **— Layout↔window coordinate conversion**
      (`layout_to_window`, `window_to_layout`, rect forms). Kart's
      draggable panels / mouse mapping: adopt if hand-derived
      letterbox/zoom math is found in `systems.h` mouse code; the
      changelog cites floatinghotel's version, verify kart's before
      claiming the win.
- [ ] **— File drop** (`RunConfig::enable_file_drop`,
      `file_drop::take_dropped_files()`). No drag-and-drop feature in
      kart (e.g. dropping a custom map file would be new product
      work). —.
- [ ] **— `IncrementalLoader`.** Kart loads everything at startup
      (`preload.cpp`); a loading screen with per-frame slicing is new
      product work, not a bump adoption. P3 at most.
- [ ] **— `with_on_submit(fn)` for `text_input`/`text_area`.** Kart's
      fields submit via listeners; adopt at the next field that needs
      a per-frame closure (the changelog's "first closure wins forever"
      trap is real, but kart has no reported instance).
- [ ] **LEVERAGE — `reset_for_new_session()`** (deterministic world
      restart: entities, singletons, ids restart at 0, index buckets
      cleared; `session_generation()` to invalidate caches). This is
      the upstream-shaped answer to `TODO.md` Tier 1 "per-round state
      never resets" — but the semantics are loud: no other live
      collection/entity may survive, all pre-reset handles must be
      discarded, systems/statics are NOT reset. Evaluate against
      `RoundManager::reset_for_new_round` (`round_settings.h:336-341`)
      in a spike: if kart's cars/systems can be rebuilt from scratch
      per round, this deletes the carried-over kills/lives/health bug
      class; if cars must outlive rounds, it does NOT fit and the
      Tier-1 fix stays a per-component reset. Do not adopt on the
      changelog alone.
- [ ] **— Draw order** (`register_render_systems_after_ui`). Kart's
      render order works today; only adopt if a hand-drawn layer must
      paint over imm content.
- [ ] **— `with_escape_blurs(false)`.** Only if a slash/@-mention/
      autocomplete overlay is built over a field. —.
- [ ] **— `timer::next_deadline`.** Only for on-demand frame loops.
- [ ] **— `LineChartOptions` styling/domains.** Kart has no charts.
- [ ] **— `with_icon_size`.** Kart's icon+text buttons use the 60%
      default; adopt only where a 24px-in-48px case is found.
- [ ] **— `with_placeholder_color`.** Kart's fields use the default
      muted hint; adopt per-field only if a screen needs a distinct
      hint colour (none found).
- [ ] **— `with_draw_only()`.** Decorative elements pay 3 components
      instead of 5. Adopt in new decorative UI; sweeping is churn.
- [ ] **— Model library core** (`TextureRegistry` dedup, glTF bounds
      without mesh decode). Kart is 2D sprites; —.
- [ ] **— `bug_note::capture`.** Playtest bug notes (markdown + PNG +
      build id + context). Kart has no in-game bug-report key; cheap
      to bind once `crash::install` path (§3.5) is decided. P3.
- [ ] **— Interior lighting zones.** 3D lighting math; kart is 2D. —.
- [ ] **— `prefabs::define/spawn`.** Kart's `make_car`/`make_*` in
      `makers.cpp` are hand-rolled prefab builders with overrides.
      `prefabs::spawn(collection, name, overrides)` matches that shape
      (override callback runs last and wins; unknown name = empty
      `OptEntity`). Evaluate when `makers.cpp` is next extended — a
      named registry would help the character-creation variants — but
      don't migrate `make_car` speculatively; its component set is
      large and mode-dependent.
- [ ] **— Render counters for the profiler.** Only with a profiling
      workflow.
- [ ] **— `app_states` plugin** (`State<E>`, `TransitionSystem`,
      `run_if(in_state(...))`). Kart already has `GameStateManager`
      (`game_state_manager.h`) + one `SystemManager` per … — check:
      the changelog's pitch is "one manager + `run_if` gates instead of
      a manager per state". If kart already gates with `should_run`
      per screen (`ui_systems.cpp:600,1468,1640`, `systems.h:35,366,680`),
      this is a refactor with no new capability. Only adopt if a new
      state or a state-transition bug (hooks on exit/enter, same-state
      no-op) justifies it. Deferred upstream: guards, timed
      transitions, state stack — kart would still hand-roll those.
- [ ] **— Saved input bindings name their enums.** Automatic on
      re-save; kart's bindings are in the settings save. Verify an old
      integer-form save still decodes (changelog says it does) — add
      to the §2.5 checklist if kart persists bindings.
- [ ] **LEVERAGE — `motion::Reveal`.** 0..1 open/close progress for
      panels/sections, caller sizes/clips/fades by `eased()`. Use for
      the disclosure/settings-panel open-close motion (§3.1) instead
      of hand-rolling a reverse float — the changelog explicitly calls
      out that hand-rolled pattern. Pair with the motion rewrite.
- [ ] **— `eye_position`, `SceneLights`, nonblocking native dialogs,
      focus-scoped shortcuts (`with_shortcut`), precise scroll speed,
      macOS Cmd-chord forwarding.** No kart need found for any (no 3D
      lights, no native dialogs, no focus-scoped shortcuts, no
      trackpad-scroll tuning, no macOS menu-less app). —.
- [ ] **LEVERAGE — Background jobs handed back on your thread**
      (`jobs::Pool`, `submit(work, deliver)`, `drain()` on the pool
      thread, `cancel(id)`). Kart does save I/O and any future health
      checks on the main thread (settings only persist on exit,
      `TODO.md` Tier 1). This is the upstream-shaped way to move save
      I/O off the frame without touching the ECS from a worker. Pair
      with the settings persistence fix, not the bump. Note the
      constraint: `drain`/`shutdown` from another thread aborts.
- [ ] **— `RenderView` + `PostChain`, `InstanceBatcher::drain`,
      `motion::Track::reverse/seek`, `IntroSequence`, `form` data-bound
      rows, `CachedSingleton`, blur shader file, `texture_is_usable`,
      UI frame-phase split, `scrollbar_gutter`, divider hit band,
      `capture_async` PNG encode, named system groups, per-draw texture
      filter, per-instance tint, instanced VAO unbind, profiling
      `ScopedZone`, shimmer presets, `skeletal` sampling math,
      `with_takes_focus(false)`.** Individually:
      - **System groups** (`register_update_system(sys, "world")`,
        `set_group_enabled`): LEVERAGE-adjacent — kart gates whole
        slices (world vs debug overlay vs menus) via per-system
        `should_run` today. One `set_group_enabled("world", false)`
        replaces a family of gates; adopt when a slice toggle is next
        added, and compose it with `should_run` (both must pass).
      - **`IntroSequence`**: kart has `intro.cpp` (its own intro
        sequence with fades/skips). Evaluate against it when the
        intro is next edited; don't rewrite a working intro for it.
      - **`form` rows** (`form::slider/checkbox/toggle/text/integer`):
        data-bound settings rows read/write the bound value each
        frame — kart's settings screen hand-writes every row
        (`ui_systems.cpp` settings builders). Adopt for new settings
        rows; the settings shell/list/rect-bridge are deferred
        upstream, so a full settings-screen rewrite is out of scope.
      - **`with_takes_focus(false)`**: adopt if a disclosure clicked
        while typing ever steals the caret — kart has no composer.
      - **`scrollbar_gutter` / divider hit band / `texture_is_usable` /
        frame-phase split / `capture_async`**: — / free / adopt only
        on a demonstrated need (kart's screenshots are synchronous
        and fine).
      - **Blur shader file** (`resources/shaders/blur.fs` now ships;
        `with_blur` previously drew unblurred without it): kart's
        post-processing (`post_processing_enabled`, `TODO.md`) — check
        whether kart supplies its own `blur.fs`; if it relied on the
        missing file, this bump fixes unblurred output silently.
        Verify visually in §2.4.
      - **The rest**: no kart need (no instancing, no skeletal, no
        shimmer skeletons, no sokol, no per-draw filters).

### 3.6 e2e block

- [ ] **LEVERAGE — `expect_window_size W H`.** See `resize_steps` (§3.5).
- [ ] **LEVERAGE — Draw-capture frame boundary** (`capture::begin_frame`,
      `expect_drawn` sees world draws on every backend). Prerequisite
      for §3.5 `expect_drawn`; verify kart's custom frame loop calls
      `begin_frame()` or the UI-pass clearing semantics apply
      (`e2e_integration.h:tick` / `main.cpp` loop).
- [ ] **LEVERAGE — `register_command(sm, name, fn)` / typed
      `register_command<Args...>` + `PendingE2ECommand::expect`.**
      Replaces kart's two hand-rolled `System<PendingE2ECommand>`
      classes (`e2e_commands.h:14-108`: argument-count checks,
      `magic_enum` parsing, `cmd.fail` boilerplate, per command).
      Mechanical, high value for every future game command. Do it in
      the e2e pass, after §2 compiles.
- [ ] **LEVERAGE — One-call e2e setup** (`testing::E2EApp::create` /
      `attach`, `E2EArgs`/`parse_e2e_args`/`configure_runner` in
      `harness.h`). Kart hand-writes ~150 lines of this in
      `e2e_integration.h` (runner, screenshot callback, reset callback,
      builtin handlers, unknown handler, cleanup, `tick`/`post_render`/
      `print_results`, plus `main.cpp` `--e2e` branching). Migrate
      once, carefully: kart's screenshot callback is custom
      (render-texture copy, `e2e_integration.h:96-114`) and must be
      carried over via `screenshot_source` / the deferred screenshot
      handler — don't lose headless capture. Keep kart's
      `set_screenshot_callback` semantics until `expect_screenshot`
      (§3.5) is decided.
- [ ] **LEVERAGE — `wait_until` / `wait_until_wall` / `wait_wall`.**
      Kart's scripts use fixed `wait` / frame counts; `wait_until`
      retries an assertion until it passes (with the last mismatch as
      the error) and is the fix for flaky timing assertions. Add to
      scripts as they flake; don't rewrite all waits.
- [ ] **LEVERAGE — `register_shared_handlers` + `register_unknown_handler`
      that waits.** Only if kart grows a second `SystemManager`/state
      with its own handlers (today: one manager, `e2e_integration.h:76-96`).
      Note for the record; don't restructure for it.
- [ ] **LEVERAGE — Gamepad injection** (`gamepad_connect` /
      `gamepad_button` / `gamepad_down`/`up` / `gamepad_axis`).
      Kart is a gamepad-first game (`preload.cpp:44` loads gamepad
      mappings; `systems.h:817` checks gamepad availability) and has
      **zero** gamepad e2e coverage. This is the highest-value e2e
      addition in the bump: add one script driving menu navigation /
      driving with a virtual pad. P1 after §2.
- [ ] **LEVERAGE — `hold_key` / `release_key`** (hold modifiers across
      Tab presses), **`mouse_down`/`middle_down` press-edge fix,
      gestures on a named element** (`mouse_down_ui`/`mouse_move_ui`/
      `scroll_wheel_ui` with offsets), **`right_click_ui` /
      `right_click_text`**, **`wait_frames` frame-count fix,
      `type` Unicode fix, screenshot-capture failure fails the script,
      `is_e2e_running()`** (keep FPS/clocks out of screenshots),
      **E2E results JSON** (`results()`, `write_results_json`,
      `--results-json`, `[PASS]/[FAIL]` lines), **timeout naming**,
      **clipped-element click centre**, **terminal commands**,
      **graphics-layer key injection**, **`dump_ui` resolved spacing**,
      **`--capture-scale` supersampled capture**, **`expect_terminal_*`**.
      Mostly free / small: adopt `is_e2e_running()` if kart's FPS
      readout ever pollutes a baseline; adopt `wait_frames` semantics
      awareness when a timing script flakes; the rest are available
      commands, not refactors. Terminal commands: kart has no terminal
      console — —.
- [ ] **— Perf: runner's own pending-command list; capture/glyph
      warnings one branch when off; `Effect::set` uniform cache.**
      Free; no action.

### 3.7 Docs block (no code)

- [ ] Read, don't implement: `docs/macos-desktop.md` (native desktop
      integration stays app-side — no new API), `docs/letterboxing.md`
      (resizing/scaling/pillarbox — kart's window manager already owns
      it; pong's re-derivation is the cautionary tale), `docs/wire-versioning.md`
      (library ships no transport; game's wire label owns schema bumps),
      minimal-consumer builds (`examples/catalog/builds/`:
      `pure_ecs` / `logic_only_ui` / `offscreen_render`). The last is
      useful if kart ever splits a headless logic test out of the game.
- [ ] **Gap docs already-work list**: don't rebuild styled-span wrap,
      `\n` hard breaks, ellipsis+`expand()`, `with_styled_label`,
      `with_font_size(FontSize::Small)`, or a resolved-layout-tree
      walker (`UIComponent::children` is visible to render systems /
      systems after `register_after_ui_updates`). Note the walker
      point for `mcp_integration.h`'s UI inspection if it re-derives
      the tree.

## 4. Fixes in the bump that delete (potential) kart workarounds — verify none exist

The changelog names downstream workarounds for many fixes; by grep, kart
has none of the named ones (no `floatinghotel`/`hanabi`/`wm`/`cartographer`
workaround code was found in `src/`). Still verify these four by hand,
because a workaround by another name wouldn't grep:

- [x] Scroll-view squash (`solve_violations` exempting scroll views) —
      kart pinned `strictness` anywhere? Grep found none; confirm.
- [x] Horizontal scroll content-width over-report — kart's tables
      (round results?) had a fudge factor on `content_size.x`? None found.
- [x] Mouse-wheel scroll hit-testing / Shift+wheel — kart swapped out
      `HandleScrollInput`? None found.
- [x] Toast fade / scheduled-toast opacity — kart uses engine toasts?
      None found; if kart hand-rolls toasts, the engine ones (§2.4)
      may replace them.

## 5. Gaps in `docs/afterhours_gaps.md` this bump closes

`docs/afterhours_gaps.md` was last re-triaged against `fc4d625`. Do not
edit it in the bump PR (keep the re-triage as its own pass once §2
compiles), but these entries are closed or part-closed by the changelog
at `5383fc6c` — verify each against the new code before moving it to
the "Closed" section:

- **e2e: no `--screenshot-dir` flag — CLOSED.** The shipped harness
  (`e2e_testing/harness.h`: `E2EArgs`, `parse_e2e_args`,
  `configure_runner`) adds `--screenshot-dir` (and `--e2e-speed`,
  closing the next entry too). Kart's hardcoded `screenshots/` paths
  in `e2e_integration.h:32,65` can now be replaced by the flag.
- **Checkbox internal layout overflow — CLOSED (kart-specific).**
  "A labelled checkbox's toggle fits its row (kart)" — the toggle
  height is pinned to the row's height intent. Verify the layout
  warning is gone in §2.4 instead of assuming it.
- **`GetFontDefault()` invalid in headless — PARTLY.**
  `load_font_for_string` now has the headless atlas fallback its
  siblings had (flagged "(kart)" in the changelog). Kart's
  workaround in `preload.cpp:144-209` (manual atlas generation +
  CJK skipping) still needs re-checking; the entry is not
  wholesale closed.
- **`with_slide_in()` — obsoleted further.** The declarative
  `with_animation(Anim::on_appear()…)` it pointed at is itself gone
  in the motion rewrite; the replacement is the widget sugar in §2.1
  (`.on_appear`, `.on_state`). The remaining item is still kart's:
  delete/replace `src/ui/animation_slide_in.h` in the §2.1 rewrite.

## 6. Suggested order of work

1. §2.1 motion rewrite + §2.2 `distance_sq` + §2.3 warning flags → `zig build` green.
2. §2.4 screenshot re-baseline + §2.5 audit (Tab/Escape/E2E ambiguity,
   gamepad `inputs_pressed`) → `make e2e` + `make ci` green.
3. Commit the bump + migration as one commit (message names the old/new
   SHAs and the motion rewrite; do not split the pointer from the fix,
   or `main` is red between them).
4. P1 leverage, in this order: `register_command` + `E2EApp` (§3.6),
   audio buses/fades/`play_at` + sound-system registration (Tier-1 fix)
   + `expect_sound_played` regression script, `expect_drawn` world-draw
   script, gamepad-injection script, `add_singleton` mechanical pass.
5. P2: `settings_table.h` + storage-codec version stamp (one save-format
   change), `reset_for_new_session` spike, `jobs::Pool` for save I/O,
   `form` rows / `SettingsPreset` on the next settings-screen edit,
   `prompt_bar` + `disclosure` + `with_slider_style` on the next UI edit.
6. Everything marked — / ADOPT-IF-NEEDED: only on a demonstrated need,
   with the evidence noted in §3. Do not turn this document into a
   feature backlog by ticking those boxes speculatively.
