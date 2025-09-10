# RFC: Worker Thread System Execution

## Overview
This RFC documents the attempt to implement worker thread execution for game systems to improve performance by running non-raylib systems in parallel with the main thread.

## Problem Statement
The game currently runs all systems sequentially on the main thread. Some systems (like AI logic, weapon cooldowns, game state checks) don't require raylib/OpenGL context and could potentially run on a separate thread to improve performance.

## Proposed Solution

### 1. SystemManager API Extensions
Add worker thread registration functions to `SystemManager`:

```cpp
// Worker thread registration (for systems that don't touch raylib)
void register_worker_update_system(std::unique_ptr<SystemBase> system);
void register_worker_update_system(const std::function<void(float)> &cb);
```

### 2. System Classification
Systems were classified into two categories:

**Worker Thread Safe (no raylib calls):**
- `AISetActiveMode` - Updates AI mode based on round type
- `AIUpdateAIParamsSystem` - Updates AI parameters
- `AITargetSelection` - AI target selection logic
- `AIVelocity` - AI movement calculations
- `AIShoot` - AI shooting decisions
- `WeaponCooldownSystem` - Weapon cooldown timers
- `WeaponFireSystem` - Weapon firing logic
- `WeaponRecoilSystem` - Weapon recoil calculations
- `ProcessDamage` - Damage processing
- `ProcessDeath` - Death state processing
- `CheckLivesWin*` - Win condition checks for lives mode
- `CheckKillsWin*` - Win condition checks for kills mode
- `CheckHippoWin*` - Win condition checks for hippo mode
- `CheckTagAndGoWin*` - Win condition checks for tag and go mode

**Main Thread Required (raylib/OpenGL calls):**
- `Shoot` - Uses raylib for projectile creation
- `SkidMarks` - Renders skid marks
- `WeaponSoundSystem` - Plays weapon sounds
- `ProjectileSpawnSystem` - Creates visual projectiles
- `UpdateRenderTexture` - Updates render textures
- `UpdateShaderValues` - Updates shader uniforms
- `MarkEntitiesWithShaders` - Applies shader effects

### 3. Execution Model
Two approaches were attempted:

**Approach 1: Parallel Execution**
```cpp
void run(const float dt) {
  auto &entities = EntityHelper::get_entities_for_mod();
  fixed_tick_all(entities, dt);
  
  // Run worker systems in parallel with main thread systems
  std::thread worker_thread([&]() {
    run_worker_systems(entities, dt);
  });
  
  tick_all(entities, dt);
  
  // Wait for worker thread to complete
  worker_thread.join();
  
  EntityHelper::cleanup();
  render_all(dt);
}
```

**Approach 2: Sequential Execution**
```cpp
void run(const float dt) {
  auto &entities = EntityHelper::get_entities_for_mod();
  fixed_tick_all(entities, dt);
  
  // Run worker systems first (they can modify entities)
  run_worker_systems(entities, dt);
  
  // Then run main thread systems
  tick_all(entities, dt);
  
  EntityHelper::cleanup();
  render_all(dt);
}
```

## Problems Encountered

### 1. Data Race Issues
**Problem:** When running systems in parallel, both the worker thread and main thread were accessing and modifying the same entity collection simultaneously, causing data races and corruption.

**Symptoms:**
- Car colors not displaying correctly
- AI not working properly
- Inconsistent entity state

**Root Cause:** The ECS architecture assumes single-threaded access to entities. Concurrent modification of entity components without proper synchronization leads to undefined behavior.

### 2. Raylib Thread Safety
**Problem:** Raylib/OpenGL context must be used on the main thread only.

**Symptoms:** Crashes when raylib functions were called from worker threads.

**Root Cause:** OpenGL context is tied to the thread that created it. Calling OpenGL functions from other threads causes crashes.

### 3. System Dependencies
**Problem:** Some systems that appeared to be "worker thread safe" actually had hidden dependencies on systems that use raylib or modify shared state.

**Example:** `WeaponFireSystem` might trigger visual effects that require raylib calls, even though the core logic doesn't directly use raylib.

### 4. Entity Snapshot Complexity
**Problem:** Creating a snapshot of entities for the worker thread to use is complex and potentially expensive.

**Issues:**
- Deep copying entity data is memory intensive
- Snapshot synchronization timing is tricky
- Changes made by worker thread need to be merged back

## Lessons Learned

### 1. ECS and Threading
The current ECS architecture is designed for single-threaded access. Adding threading requires careful consideration of:
- Entity access patterns
- Component modification synchronization
- System execution order dependencies

### 2. System Classification is Hard
Determining which systems are truly "worker thread safe" is more complex than just checking for raylib calls. Hidden dependencies and side effects can make systems unsafe for parallel execution.

### 3. Performance vs Complexity Trade-off
The performance benefits of worker threads may not justify the added complexity, especially when:
- The game already runs smoothly on a single thread
- The overhead of thread synchronization might negate performance gains
- The risk of bugs and data corruption is high

## Alternative Approaches

### 1. System Batching
Instead of threading, group related systems and optimize their execution order to improve cache locality and reduce redundant work.

### 2. Async I/O
Use async I/O for file operations, network requests, or other blocking operations that don't require the main game loop.

### 3. Job System
Implement a job system where individual tasks (not entire systems) can be parallelized, with better control over dependencies and data access.

### 4. Render Thread Separation
Keep the current update systems on the main thread but move only the rendering pipeline to a separate thread with proper synchronization.

## Conclusion

The worker thread approach was reverted due to data race issues and the complexity of ensuring thread safety in the ECS architecture. The current single-threaded approach is more maintainable and reliable.

If threading is needed in the future, consider:
1. Redesigning the ECS architecture with threading in mind from the start
2. Using a job system instead of system-level threading
3. Implementing proper synchronization primitives for entity access
4. Creating a clear separation between read-only and write operations

## Files Modified
- `vendor/afterhours/src/system.h` - Added worker thread API and execution logic
- `src/main.cpp` - Moved systems to worker thread registration

## Revert Command
```bash
git reset --hard HEAD~3
```
