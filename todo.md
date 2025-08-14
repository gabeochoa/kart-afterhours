## High-impact component/system extractions

- [ ] **Map card UI state/animation**
  - From: `ui_systems.cpp` (map selection: slide-in, pulse, hover/preview with static flags and rect checks)
  - To components: `UIAnimationState`, `UIHoverPreview`, `MapCard`
  - To systems: `MapCardAnimationSystem`, `MapHoverPreviewSystem`
  - Why: Remove statics, unify hover detection, make animations deterministic and testable

- [ ] **Round settings UI model**
  - From: `ScheduleMainMenuUI::round_settings` (+ helpers) in `ui_systems.cpp`
  - To components: `RoundSettingsUIModel` (active type, enabled weapons), `RoundTypeSettings` UI metadata
  - To systems: `RoundSettingsUISystem` (+ per-type: `LivesSettingsUI`, `KillsSettingsUI`, `HippoSettingsUI`, `TagSettingsUI`)
  - Why: Shrinks `ui_systems.cpp`, reduces switch/case churn, centralizes control wiring

- [ ] **Tag & Go visuals rendering**
  - From: `systems.h::render_tagger_indicator`
  - To components: `TaggerIndicatorStyle` (colors/sizes), reuse `HasTagAndGoTracking`
  - To systems: `TagAndGoVisualSystem` (place in `systems_roundtypes.h`)
  - Why: Decouple round-specific visuals from general render systems; enables pulse/countdown easily

- [ ] **Player join/leave and autofill**
  - From: `MatchKartsToPlayers` in `systems.h`
  - To components: `PlayerPresenceCache`, `DesiredLocalPlayers`
  - To systems: `PlayerJoinLeaveSystem`, `PlayerAutoFillSystem`
  - Why: Clear lifecycle, avoids full scans and off-by-one TODOs

- [ ] **Weapon firing side effects via events**
  - From: `weapons.h` lambdas in `Weapon::Config::on_shoot`
  - To components/events: `WeaponFired`, `RecoilConfig`, `ProjectileConfig`
  - To systems: `WeaponCooldownSystem`, `WeaponFireSystem`, `ProjectileSpawnSystem`, `WeaponRecoilSystem`, `WeaponSoundSystem`
  - Why: Decouple spawning/sounds/physics; easier to add weapons and test

- [ ] **Sound aliasing/concurrency**
  - From: `sound_systems.h` TODO and scattered `SoundLibrary::play`
  - To components: `SoundEmitter` (alias pool), `PlaySoundRequest`
  - To systems: `SoundPlaybackSystem`, `UISoundBindingSystem`
  - Why: Allow overlapping instances; centralize playback logic

- [ ] **Background music state/transition**
  - From: `BackgroundMusic` hard-coded track and seek hack
  - To components: `ActiveMusicTrack`, `MusicTransition`
  - To systems: `MusicSystem` (switch by `GameStateManager` screen; handle fade/seek)
  - Why: Robust switching without one-off hacks

- [ ] **Shader uniform updates**
  - From: scattered shader handling and TODOs (`shader_library.h`, platform toggle in `systems.h`)
  - To components: `HasShaderUniforms` (time, resolution, per-entity overrides)
  - To systems: `ShaderUniformSystem`
  - Why: Consistent per-frame updates; unify platform quirks in one place

- [ ] **Map preview orchestration**
  - From: UI hover logic + `map_system.cpp` preview generation
  - To components: `MapPreviewState` (selected/hovered/previous), `MapPreviewTexture`
  - To systems: `MapPreviewGenerationSystem`, `MapPreviewSelectionSystem`
  - Why: Demand-driven preview updates; UI decoupling and simpler regeneration

- [ ] **Settings persistence**
  - From: `settings.cpp` and `round_settings.h` TODOs/defaultizations
  - To systems: `SettingsPersistenceSystem` (load on startup; save on change/exit; includes `RoundManager` state)
  - Why: Consistent restore; less init code duplication

- [ ] **Resource iteration API**
  - From: `resources.h` multi-string callbacks and path safety TODOs
  - To data/types: `ResourceDescriptor { group, folder, name }`
  - To helpers: `ResourceWalker` utilities (normalize per-platform)
  - Why: Stronger types; safer paths; simpler callsites

- [ ] **AI parameterization**
  - From: `systems_ai.h` embedded constants per-mode
  - To components: `AIParams` (retarget radius, jitter by difficulty, chase weights), `AIMode`
  - To systems: mode-specific AI systems that read `AIParams`
  - Why: Tune behavior via data without code changes

## Lower-effort cleanups

- [ ] **UI navigation stack component**
  - From: `ScheduleMainMenuUI` local vector
  - To components: `NavigationStack`
  - To systems: `NavigationSystem`
  - Why: Consistent back/forward behavior across screens

- [ ] **Weapon metadata registry**
  - From: `WEAPON_ICON_COORDS`, names in `weapons.h`
  - To data: `WeaponRegistry` (icons, labels, defaults)
  - Why: Single source for UI + systems

- [ ] **Lives/Kills HUD renderers**
  - From: `systems.h::render_lives` and `render_kills`
  - To components: `HasLivesHUD`, `HasKillsHUD`
  - To systems: `LivesHUDSystem`, `KillsHUDSystem`
  - Why: Easier to swap to sprites/fonts later