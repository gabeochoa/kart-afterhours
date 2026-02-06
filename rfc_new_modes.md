# RFC: New Game Modes for Kart Afterhours

## Overview

This RFC explores potential new game modes that could be implemented by leveraging the existing systems and components in Kart Afterhours. The goal is to identify emergent gameplay opportunities that require minimal new code while maximizing reuse of existing mechanics.

## Current Systems Analysis

### Core Components Available
- **Transform System**: Position, velocity, acceleration, collision detection
- **Health System**: Damage, invincibility frames, death/respawn
- **Weapon System**: 4 weapon types (Cannon, Shotgun, Sniper, MachineGun) with cooldowns
- **AI System**: Target selection, movement, shooting with difficulty levels
- **Collision System**: Mass, friction, restitution, absorption
- **Boost System**: Acceleration multipliers with decay
- **Wrap-Around System**: Screen edge wrapping for seamless movement
- **Tire Marks**: Visual trail system with time-based cleanup

### Existing Game Modes
- **Lives**: Last player standing with respawns
- **Kills**: Timed deathmatch with kill tracking
- **Hippo Collection**: Timed item collection race
- **Tag and Go**: Timed tag game with size scaling

### Map Types
- **Arena**: Open combat space
- **Maze**: Complex tactical layout
- **Race Track**: Speed-focused layout
- **Battle Arena**: Combat-optimized with cover
- **Tag and Go**: Tag-optimized layout

## Proposed New Game Modes

### 1. **King of the Hill** (KOTH)
**Concept**: Control a designated area to earn points over time.

**Reused Systems**:
- `Transform` + collision detection for hill control
- `HasHealth` for hill damage resistance
- Timer system from existing round types
- `HasLabels` for hill ownership display

**New Components Needed**:
```cpp
struct HillControl : BaseComponent {
    float control_time = 0.0f;
    std::optional<EntityID> current_controller;
    float control_radius = 50.0f;
    int points_per_second = 10;
};

struct HillScore : BaseComponent {
    int total_points = 0;
    float time_controlling = 0.0f;
};
```

**Implementation**: Hill entity with `HillControl` + `Transform`. Players in radius gain control. System updates scores based on control time.

---

### 2. **Capture the Flag** (CTF)
**Concept**: Steal enemy flags and return to base while defending your own.

**Reused Systems**:
- `Transform` + collision for flag pickup/drop
- `TracksEntity` for flag following
- `HasHealth` for flag health (can be destroyed)
- `CanWrapAround` for seamless map traversal

**New Components Needed**:
```cpp
struct Flag : BaseComponent {
    bool is_captured = false;
    std::optional<EntityID> carrier;
    vec2 home_position;
    float return_time = 0.0f;
};

struct FlagBase : BaseComponent {
    int team_id;
    bool has_flag = true;
};
```

**Implementation**: Flag entities with `Flag` component. Players can pick up flags (adds `TracksEntity`). Return to base for points.

---

### 3. **Survival Wave** (Horde Mode)
**Concept**: Survive increasingly difficult waves of AI enemies.

**Reused Systems**:
- `AIControlled` + `AIDifficulty` for enemy AI
- `SpawnHippoItems` pattern for wave spawning
- `HasHealth` + `HasMultipleLives` for survival
- Weapon systems for combat

**New Components Needed**:
```cpp
struct WaveManager : BaseComponent {
    int current_wave = 1;
    int enemies_per_wave = 5;
    float wave_delay = 10.0f;
    float next_wave_time = 0.0f;
};

struct EnemySpawner : BaseComponent {
    vec2 spawn_position;
    float spawn_cooldown = 2.0f;
    float next_spawn_time = 0.0f;
};
```

**Implementation**: Wave system spawns AI enemies. Players survive as long as possible. Difficulty increases each wave.

---

### 4. **Racing Mode** (Time Trial)
**Concept**: Complete laps around tracks for best times.

**Reused Systems**:
- `Transform` for movement and positioning
- `TireMarkComponent` for visual racing lines
- `CanWrapAround` for seamless track completion
- Timer system from existing rounds

**New Components Needed**:
```cpp
struct Checkpoint : BaseComponent {
    int checkpoint_number;
    float activation_radius = 30.0f;
};

struct RaceProgress : BaseComponent {
    int current_checkpoint = 0;
    int total_laps = 3;
    int current_lap = 0;
    float best_lap_time = -1.0f;
    float lap_start_time = 0.0f;
};
```

**Implementation**: Checkpoint entities with `Checkpoint` component. System tracks player progress through checkpoints and laps.

---

### 5. **Power-Up Battle Royale**
**Concept**: Players start with basic weapons, find power-ups to become stronger.

**Reused Systems**:
- `Weapon` system for weapon upgrades
- `HippoItem` pattern for power-up spawning
- `HasHealth` + `HasMultipleLives` for elimination
- `CanShoot` for weapon management

**New Components Needed**:
```cpp
struct PowerUp : BaseComponent {
    enum class Type { WeaponUpgrade, SpeedBoost, HealthRestore, Shield };
    Type type;
    float duration = 10.0f;
    float effect_strength = 1.5f;
};

struct PowerUpEffect : BaseComponent {
    std::optional<PowerUp::Type> active_effect;
    float remaining_time = 0.0f;
    float effect_multiplier = 1.0f;
};
```

**Implementation**: Power-up entities spawn randomly. Players collect them for temporary advantages. Last player standing wins.

---

### 6. **Team Deathmatch**
**Concept**: Two teams compete for kills with team-based scoring.

**Reused Systems**:
- `HasKillCountTracker` for team scoring
- `HasHealth` + `HasMultipleLives` for combat
- Weapon systems for team battles
- Timer system for round management

**New Components Needed**:
```cpp
struct TeamMember : BaseComponent {
    int team_id;
    raylib::Color team_color;
};

struct TeamScore : BaseComponent {
    int team_id;
    int total_kills = 0;
    int total_deaths = 0;
};
```

**Implementation**: Players assigned to teams. Team scores tracked separately. Team with most kills wins.

---

### 7. **Infection Mode**
**Concept**: One player starts as "infected" and spreads infection by touching others.

**Reused Systems**:
- `HasTagAndGoTracking` pattern for infection status
- `Transform` + collision for infection spread
- Timer system for infection rounds
- `ScaleTaggerSize` pattern for visual feedback

**New Components Needed**:
```cpp
struct InfectionStatus : BaseComponent {
    bool is_infected = false;
    float infection_time = 0.0f;
    std::optional<EntityID> infected_by;
};

struct InfectionRound : BaseComponent {
    int infected_count = 1;
    int survivor_count = 0;
    float round_time = 60.0f;
};
```

**Implementation**: Similar to Tag and Go but with infection theme. Infected players grow larger, survivors try to avoid contact.

---

### 8. **Domination** (Multi-Point Control)
**Concept**: Control multiple capture points across the map for territory control.

**Reused Systems**:
- `Transform` + collision for point control
- `HasHealth` for point health/damage
- Timer system for scoring
- `HasLabels` for point status display

**New Components Needed**:
```cpp
struct ControlPoint : BaseComponent {
    int point_id;
    float control_radius = 40.0f;
    std::optional<EntityID> controlling_team;
    float capture_progress = 0.0f;
    float capture_time = 5.0f;
};

struct TerritoryControl : BaseComponent {
    int team_id;
    int points_controlled = 0;
    float points_per_second = 5.0f;
};
```

**Implementation**: Multiple control points across map. Teams capture and hold points for continuous scoring.

---

### 9. **Gun Game** (Weapon Progression)
**Concept**: Players progress through different weapons with each kill.

**Reused Systems**:
- `Weapon` system for weapon progression
- `HasKillCountTracker` for progression tracking
- `CanShoot` for weapon management
- Timer system for round management

**New Components Needed**:
```cpp
struct WeaponProgression : BaseComponent {
    int current_weapon_index = 0;
    std::vector<Weapon::Type> weapon_progression;
    bool progression_complete = false;
};
```

**Implementation**: Players start with basic weapons. Each kill advances to next weapon. First to complete progression wins.

---

### 10. **Juggernaut Mode**
**Concept**: One player becomes increasingly powerful while others try to stop them.

**Reused Systems**:
- `HasHealth` + `HasMultipleLives` for juggernaut health
- `Transform` + collision for juggernaut detection
- Weapon systems for combat
- Timer system for power scaling

**New Components Needed**:
```cpp
struct JuggernautStatus : BaseComponent {
    bool is_juggernaut = false;
    float power_level = 1.0f;
    float power_growth_rate = 0.1f;
    int kills_as_juggernaut = 0;
};
```

**Implementation**: Juggernaut gains power over time. Other players must work together to defeat them before they become unstoppable.

## Implementation Priority

### High Priority (Easy Implementation)
1. **King of the Hill** - Minimal new components, reuses existing systems
2. **Team Deathmatch** - Simple team assignment, existing combat systems
3. **Racing Mode** - Checkpoint system, existing movement mechanics

### Medium Priority (Moderate Complexity)
4. **Capture the Flag** - Flag tracking system, base mechanics
5. **Infection Mode** - Infection spread logic, visual feedback
6. **Domination** - Multi-point control system

### Low Priority (Complex Implementation)
7. **Survival Wave** - Wave management, enemy spawning
8. **Power-Up Battle Royale** - Power-up system, effect management
9. **Gun Game** - Weapon progression, kill tracking
10. **Juggernaut Mode** - Power scaling, team coordination

## Technical Considerations

### Component Reuse Strategy
- Leverage existing `BaseComponent` inheritance
- Reuse `Transform`, `HasHealth`, `HasMultipleLives` patterns
- Extend existing systems rather than creating new ones
- Use `std::optional` for nullable relationships

### System Integration
- Follow existing `System<Components...>` pattern
- Use `PausableSystem` for game state management
- Integrate with `RoundManager` for mode switching
- Maintain compatibility with existing maps

### Performance Considerations
- Reuse existing collision detection systems
- Leverage `EntityQuery` for efficient entity filtering
- Minimize new component types to reduce memory overhead
- Use existing timer and countdown systems

## Conclusion

These proposed game modes demonstrate the flexibility and extensibility of the existing Kart Afterhours architecture. By leveraging current systems and components, we can create diverse gameplay experiences with minimal new code while maintaining performance and code quality.

The high-priority modes (KOTH, Team Deathmatch, Racing) could be implemented quickly to expand the game's variety, while the more complex modes provide long-term development goals that showcase the engine's capabilities.

Each mode follows the established patterns in the codebase and can be easily integrated into the existing round management system, providing players with new ways to enjoy the core kart combat mechanics.