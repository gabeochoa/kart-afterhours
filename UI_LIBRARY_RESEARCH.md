# UI Library Research: Clay UI & Immediate Mode UI Insights

## Overview
This document summarizes research into Clay UI library and immediate mode UI libraries, comparing their approaches to the current builder factory style UI system.

## Key Findings

### Clay UI Library

**Core Philosophy:**
- **Immediate Mode UI**: Recalculates entire layout from scratch every frame
- **Flexbox-like Layout Model**: Uses familiar CSS Flexbox concepts
- **Declarative Syntax**: React-like nested component structure
- **Renderer Agnostic**: Outputs sorted list of render primitives

**Key Features:**
1. **Microsecond Performance**: Static arena-based memory, no dynamic allocation
2. **Single Header File**: ~4,000 lines, zero dependencies
3. **Deterministic**: No malloc/free, predictable performance
4. **WebAssembly Support**: Can compile to 15KB .wasm file

**API Pattern:**
```c
Clay_BeginLayout(width, height);
  CLAY(column, {
    CLAY(button, { .label = "Click Me" });
    CLAY(text, { .content = "Hello" });
  });
Clay_EndLayout();
```

**Layout Algorithm:**
1. Set layout dimensions
2. Update pointer state
3. Handle scrolling
4. Begin layout declaration
5. Declare UI hierarchy (nested)
6. End layout
7. Render using outputted render commands

**Key Insight**: Layout is computed **every frame** from scratch, which:
- Simplifies state management (no persistent UI state)
- Enables fluid animations naturally
- Makes layout behavior predictable
- Eliminates "stale layout" issues

---

### Immediate Mode UI Libraries (Dear ImGui, Nuklear, etc.)

**Core Characteristics:**

1. **Statelessness**
   - UI elements defined each frame
   - Application manages state, not UI library
   - No retained widget objects

2. **Simple Event Handling**
   - Input handled during frame construction
   - Return values indicate interactions
   - No complex event systems needed

3. **Flexible Integration**
   - Easy to integrate into existing render pipelines
   - No special initialization required
   - Works with any graphics backend

**API Pattern (Dear ImGui style):**
```cpp
ImGui::Begin("Window");
ImGui::Text("Hello");
if (ImGui::Button("Click")) {
    // handle click
}
ImGui::End();
```

**Key Insight**: Elements are **rebuilt every frame**, which means:
- Layout is always fresh and correct
- No "layout cache" issues
- Natural animation support
- Simpler mental model

---

## Comparison: Current System vs. Clay/Immediate Mode

### Current System (Builder Factory Style)

**Strengths:**
- ✅ Fluent builder API (`ComponentConfig{}.with_label().with_size()`)
- ✅ Type-safe configuration
- ✅ Component reuse via entity management
- ✅ Flexible styling system

**Weaknesses (from your feedback):**
- ❌ Autolayout doesn't work as expected
- ❌ Hard to make UI that actually looks nice
- ❌ Layout behavior is unpredictable
- ❌ Complex state management (entities persist between frames)

**Current Flow:**
1. Create entities with `mk(parent, id)`
2. Configure with `ComponentConfig` builder
3. Entities persist between frames
4. Autolayout runs separately (may not compute correctly)
5. Rendering happens based on computed values

**Issues Identified:**
- Autolayout may not run for absolutely positioned elements
- Computed sizes remain -1.0 (uncomputed)
- Negative sizes cause invisible elements
- Timing issues (layout may not be ready when needed)

---

### Clay/Immediate Mode Approach

**Strengths:**
- ✅ Layout computed every frame (always fresh)
- ✅ Predictable behavior
- ✅ No stale layout issues
- ✅ Natural animation support
- ✅ Simpler mental model

**Potential Weaknesses:**
- ⚠️ Recomputes everything each frame (but Clay is optimized for this)
- ⚠️ Less "object-oriented" (no persistent entities)
- ⚠️ Different programming model

---

## Key Insights & Recommendations

### 1. **Layout Recalculation Every Frame**

**Clay's Approach:**
- Recalculates entire layout tree every frame
- No caching of layout results
- Layout is always correct and up-to-date

**Your System:**
- Entities persist between frames
- Autolayout runs separately
- May have timing issues or stale computed values

**Recommendation:**
Consider making autolayout run **every frame** and **always** recalculate, even for elements that haven't changed. This would:
- Eliminate stale layout issues
- Make behavior more predictable
- Fix the "computed_x: -1.0" problem

### 2. **Simpler Layout Model**

**Clay's Approach:**
- Uses Flexbox model (familiar to web developers)
- Clear parent-child relationships
- Explicit size constraints

**Your System:**
- Complex size system (`Dim::Pixels`, `Dim::Percent`, `Dim::Children`, etc.)
- Multiple layout passes (standalone, with parents, with children, violations)
- May have edge cases in constraint solving

**Recommendation:**
Consider simplifying the size system or making it more explicit:
- Clearer rules for when sizes are computed
- Better error messages when layout fails
- Validation that sizes are computed before rendering

### 3. **Declarative Nested Structure**

**Clay's Approach:**
```c
CLAY(column, {
  CLAY(button, { .label = "OK" });
  CLAY(button, { .label = "Cancel" });
});
```

**Your System:**
```cpp
auto container = div(context, mk(entity, 0), config);
div(context, mk(container.ent(), 0), child_config);
div(context, mk(container.ent(), 1), child_config);
```

**Recommendation:**
Your builder pattern is actually quite good! The issue isn't the API style, but the layout computation. Consider:
- Keeping the builder pattern (it's ergonomic)
- But ensuring layout is always fresh
- Maybe add a "force layout recalculation" option

### 4. **Explicit Size Requirements**

**Clay's Approach:**
- Layout always has explicit dimensions
- No "auto" sizes that might not compute
- Clear error when constraints can't be satisfied

**Your System:**
- `Dim::Children` can be ambiguous
- `-1.0` values indicate uncomputed
- No clear error when layout fails

**Recommendation:**
- Add validation: if `computed_x == -1.0` after layout, log an error
- Make size requirements more explicit
- Consider requiring explicit sizes for root elements

### 5. **Animation & State Management**

**Clay's Approach:**
- Layout changes naturally animate (recomputed each frame)
- State managed by application, not UI library
- No persistent UI state to sync

**Your System:**
- Entities persist, may have stale state
- Need to manually update positions for animations
- Complex state synchronization

**Recommendation:**
- Consider making layout truly immediate-mode (recompute every frame)
- Or add explicit "invalidate layout" mechanism
- Clear separation: UI describes layout, application manages state

---

## Specific Recommendations for Your System

### Short-term Fixes

1. **Force Layout Recalculation**
   - Always run autolayout every frame
   - Don't skip elements that "haven't changed"
   - This will fix the `-1.0` computed values

2. **Add Layout Validation**
   - Check if `computed_x == -1.0` after layout
   - Log warnings/errors when layout fails
   - Don't render elements with invalid sizes

3. **Fix Absolutely Positioned Elements**
   - Ensure autolayout computes sizes even for absolute elements
   - Size computation should happen before position computation
   - Absolute positioning should use computed size, not desired size

4. **Better Error Messages**
   - When layout fails, explain why
   - Show which constraints couldn't be satisfied
   - Indicate which elements have invalid sizes

### Long-term Improvements

1. **Consider Immediate-Mode Layout**
   - Recalculate layout every frame
   - Don't cache layout results
   - This matches Clay's approach and fixes predictability

2. **Simplify Size System**
   - Reduce number of size types
   - Make constraints more explicit
   - Better documentation of when each type is used

3. **Add Layout Debugging**
   - Visual debugger showing layout tree
   - Show computed vs desired sizes
   - Highlight elements with layout issues

4. **Improve Builder API**
   - Keep the fluent builder (it's good!)
   - But add helpers for common patterns
   - Maybe add layout presets (centered, fullscreen, etc.)

---

## What Makes Clay's Layout "Work"

1. **Always Fresh**: Layout computed every frame, no stale data
2. **Explicit Constraints**: Clear size requirements, no ambiguity
3. **Simple Model**: Flexbox is well-understood and predictable
4. **Fast**: Optimized for recomputation (arena allocation, SIMD)
5. **Deterministic**: No dynamic allocation, predictable performance

**The key insight**: Clay's layout works because it's **simple and always correct**. Your system might be trying to be too clever with caching and optimization, which leads to the unpredictable behavior you're experiencing.

---

## Conclusion

Your builder factory style is actually quite good from an API perspective. The issue is likely in the **layout computation**, not the API design. 

**Key Takeaways:**
- Consider making layout truly immediate-mode (recompute every frame)
- Add validation and error messages for layout failures
- Simplify the constraint solving system
- Keep the builder pattern, but ensure layout is always fresh

The builder pattern + immediate-mode layout computation could give you the best of both worlds: a nice API and predictable layout behavior.

---

## References

- [Clay UI GitHub](https://github.com/nicbarker/clay)
- [Clay Layout Algorithm Video](https://www.youtube.com/watch?v=by9lQvpvMIc)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)

---

## Deep Dive: Your Autolayout System Analysis

### ✅ **YES, It Already Runs Every Frame!**

After reviewing `vendor/afterhours`, I found that:

1. **`RunAutoLayout` is registered as an update system** (line 125 in `utilities.h`)
2. **It runs every frame** via `systems.run(dt)` in the game loop
3. **It DOES reset values** - `autolayout()` calls `reset_computed_values()` which sets `computed[Axis::X] = -1` and `computed[Axis::Y] = -1`

**So the system IS already "immediate-mode" in the sense that it runs every frame and resets!**

### ❌ **But the Multi-Pass Algorithm Has a Fundamental Problem**

The issue is NOT that it doesn't run every frame. The issue is that the **multi-pass layout algorithm has dependency chain problems** that can't be solved in a single pass, even when run every frame.

After examining your autolayout code, I found the root cause of the `-1.0` computed values:

### Your Layout Passes (in order):

1. **`calculate_standalone`** - Computes sizes that don't need parent/children:
   - `Dim::Pixels` ✅
   - `Dim::ScreenPercent` ✅
   - `Dim::Text` ✅
   - `Dim::Percent` → returns `-1.0` (needs parent) ❌
   - `Dim::Children` → returns `-1.0` (needs children) ❌

2. **`calculate_those_with_parents`** - Computes sizes that need parent:
   - `Dim::Percent` ✅ (if parent is computed)
   - Returns `-1.0` if parent not computed yet ❌

3. **`calculate_those_with_children`** - Computes sizes that need children:
   - `Dim::Children` ✅ (if all children computed)
   - Returns `-1.0` if children not computed yet ❌

### The Problem:

**Line 813-820 in autolayout.h:**
```cpp
if (parent.computed[axis] == -1) {
    // ... error log ...
    return no_change;  // no_change is -1.0!
}
```

**Line 1010-1015:**
```cpp
if (cs == -1) {
    log_error("expect that all children have been solved by now");
}
```

**The issue**: If a widget uses `Dim::Percent` but its parent hasn't been computed yet (maybe parent uses `Dim::Children` and children aren't ready), it returns `-1.0` and never retries.

### Why Multi-Pass is Actually Necessary

**Important Insight**: Multi-pass layout is **required** for auto-resizing features like:
- **Size-by-children**: Parent size depends on children's sizes
- **Size-by-content**: Element size depends on text/image content
- **Flexible layouts**: Elements that grow/shrink based on available space

**The Real Problem**: It's not that multi-pass is wrong - it's that your multi-pass implementation has **dependency chain failures** that leave values at `-1.0`.

### How Multi-Pass Should Work

**Correct Multi-Pass Approach**:

1. **Pass 1: Standalone Sizes** (top-down)
   - Compute sizes that don't depend on others (Pixels, ScreenPercent, Text)
   - These can always be computed immediately

2. **Pass 2: Parent-Dependent Sizes** (top-down)
   - Compute sizes that depend on parent (Percent)
   - Use **estimates** if parent not ready yet (don't return `-1.0`)

3. **Pass 3: Children-Dependent Sizes** (bottom-up)
   - Compute sizes that depend on children (Children)
   - Must traverse bottom-up: children first, then parent
   - This is where auto-resizing happens

4. **Pass 4: Constraint Solving** (if needed)
   - Fix violations (children don't fit in parent)
   - Adjust flexible elements

5. **Pass 5: Positioning**
   - Compute relative positions
   - Compute screen positions

**Key Fix**: Use **estimates** instead of `-1.0` when dependencies aren't ready. This allows:
- Layout to always produce valid values
- Multi-pass to work correctly
- Auto-resizing to function properly

### Why Clay Can Do Single-Pass

Clay uses a **different constraint model**:
- It doesn't support "size by children" in the same way
- It uses flexbox-style constraints that can be solved in one pass
- It has simpler size specifications

**Your system needs multi-pass** because you support:
- `Dim::Children` - size based on children (requires bottom-up pass)
- `Dim::Percent` - size based on parent (requires top-down pass)
- Complex constraint solving (violations, flexible elements)

**The solution isn't to eliminate multi-pass** - it's to **fix the multi-pass algorithm** to handle dependencies correctly.

### Fix Strategy:

Since you already run every frame, the fixes are:

1. **Multiple Iterations Per Frame**: Run the layout passes multiple times until convergence (within the same frame)
2. **Better Dependency Tracking**: Track which widgets depend on what, compute in correct order
3. **Default Values**: Instead of `-1.0`, use reasonable defaults (e.g., 0 or desired size)
4. **Validation After Layout**: Check for `-1.0` values and log/error appropriately
5. **Single-Pass Algorithm** (like Clay): Redesign to compute everything in one pass instead of multiple dependent passes

---

## Implementation Strategies

Here are concrete approaches to fix the multi-pass dependency problem:

### Strategy 1: Multiple Iterations Until Convergence (Easiest)

**Approach**: Run the existing multi-pass algorithm multiple times per frame until all values are computed.

**Implementation**:

```cpp
static void autolayout(UIComponent &widget,
                       const window_manager::Resolution resolution,
                       const std::map<EntityID, RefEntity> &map) {
  AutoLayout al(resolution, map);
  
  const int MAX_ITERATIONS = 10;
  int iteration = 0;
  bool has_uncomputed = true;
  
  while (has_uncomputed && iteration < MAX_ITERATIONS) {
    al.reset_computed_values(widget);
    
    al.calculate_standalone(widget);
    al.calculate_those_with_parents(widget);
    al.calculate_those_with_children(widget);
    al.solve_violations(widget);
    al.compute_relative_positions(widget);
    al.compute_rect_bounds(widget);
    
    // Check if any values are still uncomputed
    has_uncomputed = check_for_uncomputed_values(widget);
    iteration++;
  }
  
  if (has_uncomputed) {
    log_warn("Autolayout did not converge after {} iterations", MAX_ITERATIONS);
    // Fallback: use defaults or previous frame's values
  }
}
```

**Pros**:
- Minimal changes to existing code
- Preserves current algorithm structure
- Easy to implement and test

**Cons**:
- May be slower (multiple passes per frame)
- Still has dependency chain issues (just retries)
- Could hit iteration limit

**Where to implement**: Modify `AutoLayout::autolayout()` in `vendor/afterhours/src/plugins/autolayout.h` around line 1447

---

### Strategy 2: Better Dependency Tracking with Topological Sort

**Approach**: Build a dependency graph and compute widgets in the correct order.

**Implementation**:

```cpp
struct LayoutDependency {
  EntityID widget_id;
  std::vector<EntityID> depends_on;  // Parents or children it needs
  bool computed = false;
};

static void autolayout_with_dependencies(
    UIComponent &widget,
    const window_manager::Resolution resolution,
    const std::map<EntityID, RefEntity> &map) {
  
  AutoLayout al(resolution, map);
  
  // Build dependency graph
  std::map<EntityID, LayoutDependency> deps;
  build_dependency_graph(widget, deps);
  
  // Topological sort to get computation order
  std::vector<EntityID> computation_order;
  topological_sort(deps, computation_order);
  
  // Reset all values
  al.reset_computed_values(widget);
  
  // Compute in dependency order
  for (EntityID id : computation_order) {
    UIComponent &cmp = al.to_cmp(id);
    
    // Try to compute based on what's available
    if (can_compute_standalone(cmp)) {
      al.calculate_standalone(cmp);
    }
    if (can_compute_with_parent(cmp, al)) {
      al.calculate_those_with_parents(cmp);
    }
    if (can_compute_with_children(cmp, al)) {
      al.calculate_those_with_children(cmp);
    }
  }
  
  // Final passes
  al.solve_violations(widget);
  al.compute_relative_positions(widget);
  al.compute_rect_bounds(widget);
}
```

**Pros**:
- Computes in correct order
- More predictable
- Handles complex dependency chains

**Cons**:
- More complex to implement
- Requires dependency graph building
- Still multi-pass, just better ordered

**Where to implement**: New function in `autolayout.h`, or refactor existing `autolayout()`

---

### Strategy 3: Default Values Instead of -1.0

**Approach**: Use reasonable defaults when computation can't happen, instead of `-1.0`.

**Implementation**:

```cpp
float compute_size_for_parent_expectation(const UIComponent &widget, Axis axis) {
  const Size &exp = widget.desired[axis];
  float no_change = widget.computed[axis];
  
  // If already computed, return it
  if (no_change >= 0) {
    return no_change;
  }
  
  if (widget.parent == -1) {
    if (exp.dim == Dim::Percent) {
      // Use screen size as fallback for root
      return fetch_screen_value_(axis) * exp.value;
    }
    return no_change;
  }
  
  UIComponent &parent = this->to_cmp(widget.parent);
  if (parent.computed[axis] == -1) {
    // Parent not ready - use desired size or screen percentage as fallback
    if (exp.dim == Dim::Percent) {
      // Estimate based on screen size
      float screen_size = fetch_screen_value_(axis);
      return screen_size * exp.value * 0.5f;  // Conservative estimate
    }
    // Return desired value if available, or 0
    return exp.value > 0 ? exp.value : 0.0f;
  }
  
  // Normal computation
  float parent_size = (parent.computed[axis] - parent.computed_margin[axis]);
  switch (exp.dim) {
    case Dim::Percent:
      return exp.value * parent_size;
    // ... other cases
  }
  return no_change;
}
```

**Pros**:
- Elements always have valid sizes
- No `-1.0` values to check
- UI can render even with incomplete layout

**Cons**:
- May show incorrect sizes temporarily
- Masks the real problem
- Could cause visual glitches

**Where to implement**: Modify computation functions in `autolayout.h`:
- `compute_size_for_parent_expectation()` (line 793)
- `compute_size_for_child_expectation()` (line 1047)
- `compute_size_for_standalone_exp()` (line 753)

---

### Strategy 4: Fixed Multi-Pass with Estimates - Best for Auto-Resizing

**Approach**: Keep multi-pass (needed for auto-resizing), but fix dependency handling with estimates.

**Implementation Concept**:

```cpp
struct LayoutContext {
  window_manager::Resolution resolution;
  std::map<EntityID, RefEntity> entities;
  
  // Cache for computed values during single pass
  std::map<EntityID, std::pair<float, float>> size_cache;  // width, height
  std::map<EntityID, Vector2Type> position_cache;
};

static void autolayout_single_pass(
    UIComponent &widget,
    LayoutContext &ctx,
    float available_width,
    float available_height) {
  
  // Compute this widget's size based on constraints
  float width = compute_width_single_pass(widget, ctx, available_width);
  float height = compute_height_single_pass(widget, ctx, available_height);
  
  // Cache the result
  ctx.size_cache[widget.id] = {width, height};
  widget.computed[Axis::X] = width;
  widget.computed[Axis::Y] = height;
  
  // Compute padding and margin
  compute_spacing_single_pass(widget, ctx);
  
  // Compute available space for children
  float child_available_w = width - widget.computed_padd[Axis::X] - widget.computed_margin[Axis::X];
  float child_available_h = height - widget.computed_padd[Axis::Y] - widget.computed_margin[Axis::Y];
  
  // Layout children
  float current_x = 0, current_y = 0;
  for (EntityID child_id : widget.children) {
    UIComponent &child = ctx.to_cmp(child_id);
    
    if (child.absolute) {
      // Absolute positioning - doesn't affect layout
      continue;
    }
    
    // Recursive call for child
    autolayout_single_pass(child, ctx, child_available_w, child_available_h);
    
    // Position child based on flex direction
    if (widget.flex_direction == FlexDirection::Row) {
      child.computed_rel[Axis::X] = current_x;
      child.computed_rel[Axis::Y] = 0;
      current_x += child.computed[Axis::X] + child.computed_margin[Axis::X];
    } else {  // Column
      child.computed_rel[Axis::X] = 0;
      child.computed_rel[Axis::Y] = current_y;
      current_y += child.computed[Axis::Y] + child.computed_margin[Axis::Y];
    }
  }
  
  // If widget size depends on children, recompute
  if (widget.desired[Axis::X].dim == Dim::Children) {
    float children_width = compute_children_width(widget, ctx);
    widget.computed[Axis::X] = children_width;
  }
  if (widget.desired[Axis::Y].dim == Dim::Children) {
    float children_height = compute_children_height(widget, ctx);
    widget.computed[Axis::Y] = children_height;
  }
}

float compute_width_single_pass(
    const UIComponent &widget,
    LayoutContext &ctx,
    float available_width) {
  
  const Size &exp = widget.desired[Axis::X];
  
  switch (exp.dim) {
    case Dim::Pixels:
      return exp.value;
      
    case Dim::ScreenPercent:
      return exp.value * ctx.resolution.width;
      
    case Dim::Text:
      return get_text_size_for_axis(widget, Axis::X);
      
    case Dim::Percent:
      // Use available_width (passed from parent)
      return exp.value * available_width;
      
    case Dim::Children:
      // Will be computed after children are laid out
      // Return estimate for now, will be corrected
      return 0.0f;  // Or estimate based on children
      
    case Dim::None:
      return widget.computed[Axis::X] >= 0 ? widget.computed[Axis::X] : 0.0f;
  }
  return 0.0f;
}
```

**Pros**:
- Supports auto-resizing (size-by-children)
- Handles dependencies correctly
- Always computes valid values (no `-1.0`)
- Works with existing multi-pass structure
- Estimates allow rendering even with incomplete layout

**Cons**:
- Still multi-pass (but that's necessary for auto-resizing)
- Estimates might be slightly off until refined
- Need to handle circular dependencies

**Where to implement**: Modify existing `autolayout()` in `autolayout.h`, update computation functions to use estimates

---

### Strategy 5: Hybrid Approach - Iterations + Defaults

**Approach**: Combine multiple iterations with default values as fallback.

**Implementation**:

```cpp
static void autolayout(UIComponent &widget,
                       const window_manager::Resolution resolution,
                       const std::map<EntityID, RefEntity> &map) {
  AutoLayout al(resolution, map);
  
  // Try multiple iterations
  const int MAX_ITERATIONS = 5;
  for (int i = 0; i < MAX_ITERATIONS; i++) {
    al.reset_computed_values(widget);
    al.calculate_standalone(widget);
    al.calculate_those_with_parents(widget);
    al.calculate_those_with_children(widget);
    al.solve_violations(widget);
    al.compute_relative_positions(widget);
    al.compute_rect_bounds(widget);
    
    if (!has_uncomputed_values(widget)) {
      return;  // Success!
    }
  }
  
  // Fallback: fill in defaults for any remaining -1.0 values
  fill_default_values(widget, al);
}

void fill_default_values(UIComponent &widget, AutoLayout &al) {
  // For any uncomputed values, use reasonable defaults
  if (widget.computed[Axis::X] < 0) {
    if (widget.desired[Axis::X].dim == Dim::Percent) {
      // Estimate based on screen
      widget.computed[Axis::X] = widget.desired[Axis::X].value * al.fetch_screen_value_(Axis::X) * 0.5f;
    } else {
      widget.computed[Axis::X] = widget.desired[Axis::X].value > 0 ? widget.desired[Axis::X].value : 100.0f;
    }
  }
  
  if (widget.computed[Axis::Y] < 0) {
    if (widget.desired[Axis::Y].dim == Dim::Percent) {
      widget.computed[Axis::Y] = widget.desired[Axis::Y].value * al.fetch_screen_value_(Axis::Y) * 0.5f;
    } else {
      widget.computed[Axis::Y] = widget.desired[Axis::Y].value > 0 ? widget.desired[Axis::Y].value : 50.0f;
    }
  }
  
  // Recursively fix children
  for (EntityID child_id : widget.children) {
    fill_default_values(al.to_cmp(child_id), al);
  }
}
```

**Pros**:
- Best of both worlds
- Handles most cases with iterations
- Fallback ensures no `-1.0` values
- Easier than full redesign

**Cons**:
- Still has dependency issues (just handles them better)
- Defaults might be wrong
- More code to maintain

**Where to implement**: Modify `AutoLayout::autolayout()` and add `fill_default_values()` helper

---

## Recommended Approach

**Start with Strategy 5 (Hybrid)** because:
1. Minimal changes to existing code
2. Fixes the immediate `-1.0` problem
3. Can be implemented incrementally
4. Provides fallback safety

**Then move to Strategy 4 (Fixed Multi-Pass with Estimates)** for long-term:
1. Supports auto-resizing (which you need)
2. Handles dependencies correctly
3. Always produces valid values
4. Works with your existing multi-pass structure
5. Better than single-pass for your use case (supports size-by-children)

**Note**: Single-pass (Strategy 4 old version) won't work for your system because you need size-by-children, which requires bottom-up computation. Multi-pass is the right approach - it just needs to be fixed to handle dependencies correctly.

---

## Implementation Steps

### Phase 1: Quick Fix (Strategy 5)
1. Add iteration loop to `autolayout()`
2. Add `has_uncomputed_values()` check
3. Add `fill_default_values()` fallback
4. Test with existing UI screens
5. Verify no more `-1.0` values

### Phase 2: Validation
1. Add logging for uncomputed values
2. Add warnings when iterations hit limit
3. Add metrics to track iteration counts
4. Monitor performance impact

### Phase 3: Long-term (Strategy 4)
1. Design single-pass algorithm
 elemntes 
4. Gradually migrate
5. Remove old multi-pass code

---

## Code Locations

**Main function to modify**:
- `vendor/afterhours/src/plugins/autolayout.h` line 1447: `AutoLayout::autolayout()`

**Helper functions to add**:
- `has_uncomputed_values()` - check for `-1.0` values
- `fill_default_values()` - provide fallback values
- `check_for_uncomputed_values()` - validation helper

**Computation functions that might need changes**:
- `compute_size_for_parent_expectation()` (line 793)
- `compute_size_for_child_expectation()` (line 1047)
- `compute_size_for_standalone_exp()` (line 753)

---

## Research: How to Do Layout Updates

Based on research into Clay UI, immediate-mode UI libraries, and constraint solving algorithms, here's how layout updates should work:

### Current System Analysis

**Your Current Layout Update Flow** (from `autolayout.h`):

```cpp
1. reset_computed_values()     // Set all to -1.0
2. calculate_standalone()       // Compute Pixels, ScreenPercent, Text
3. calculate_those_with_parents() // Compute Percent (needs parent)
4. calculate_those_with_children() // Compute Children (needs children)
5. solve_violations()           // Fix constraint violations
6. compute_relative_positions() // Position children
7. compute_rect_bounds()        // Final screen positions
```

**Problems**:
- Steps 2-4 have dependencies that can fail
- If step 3 needs parent but parent uses step 4, it fails
- No retry mechanism within the frame
- `-1.0` values propagate and never get fixed

### How Immediate-Mode Layout Should Work

**Single-Pass Approach** (Clay-style):

```cpp
void layout_single_pass(Widget &widget, LayoutContext &ctx) {
  // 1. Compute size based on constraints (all in one go)
  float width = resolve_width(widget, ctx);
  float height = resolve_height(widget, ctx);
  
  // 2. Cache results immediately
  widget.computed_width = width;
  widget.computed_height = height;
  
  // 3. Compute available space for children
  float child_w = width - padding_x - margin_x;
  float child_h = height - padding_y - margin_y;
  
  // 4. Layout children (recursive)
  float current_x = 0, current_y = 0;
  for (auto &child : widget.children) {
    layout_single_pass(child, ctx.with_available(child_w, child_h));
    
    // Position child
    child.x = current_x;
    child.y = current_y;
    
    // Advance position
    if (widget.flex_direction == Row) {
      current_x += child.computed_width;
    } else {
      current_y += child.computed_height;
    }
  }
  
  // 5. If size depends on children, recompute
  if (widget.size_mode == SizeByChildren) {
    widget.computed_width = compute_from_children(widget);
    widget.computed_height = compute_from_children(widget);
  }
}
```

**Key Differences**:
- Everything computed in one traversal
- No separate passes for different constraint types
- Results cached as computed
- Children laid out with available space passed down
- Size-by-children handled after children are laid out

### Constraint Resolution Strategies

#### Strategy 1: Top-Down with Available Space

**Concept**: Pass available space down the tree, compute sizes top-down.

```cpp
struct LayoutContext {
  float available_width;
  float available_height;
  float parent_x;
  float parent_y;
};

float resolve_size(const Size &size_spec, LayoutContext &ctx, Axis axis) {
  switch (size_spec.dim) {
    case Dim::Pixels:
      return size_spec.value;
      
    case Dim::ScreenPercent:
      return size_spec.value * (axis == X ? screen_width : screen_height);
      
    case Dim::Percent:
      return size_spec.value * (axis == X ? ctx.available_width : ctx.available_height);
      
    case Dim::Text:
      return measure_text_size(size_spec);
      
    case Dim::Children:
      // Will be computed after children
      return 0; // Placeholder
  }
}

void layout_widget(Widget &w, LayoutContext &ctx) {
  // Resolve size
  w.computed_width = resolve_size(w.desired_width, ctx, X);
  w.computed_height = resolve_size(w.desired_height, ctx, Y);
  
  // Compute padding/margin
  w.computed_padding = resolve_padding(w.desired_padding, ctx);
  w.computed_margin = resolve_margin(w.desired_margin, ctx);
  
  // Compute available space for children
  LayoutContext child_ctx = ctx;
  child_ctx.available_width = w.computed_width - w.computed_padding.x - w.computed_margin.x;
  child_ctx.available_height = w.computed_height - w.computed_padding.y - w.computed_margin.y;
  child_ctx.parent_x = ctx.parent_x + w.computed_margin.left;
  child_ctx.parent_y = ctx.parent_y + w.computed_margin.top;
  
  // Layout children
  float current_x = 0, current_y = 0;
  for (auto &child : w.children) {
    if (child.absolute) {
      // Absolute positioning - doesn't affect layout
      continue;
    }
    
    layout_widget(child, child_ctx);
    
    // Position child
    child.computed_x = current_x;
    child.computed_y = current_y;
    
    // Advance
    if (w.flex_direction == Row) {
      current_x += child.computed_width + child.computed_margin.x;
    } else {
      current_y += child.computed_height + child.computed_margin.y;
    }
  }
  
  // If size depends on children, recompute
  if (w.desired_width.dim == Dim::Children) {
    w.computed_width = compute_children_width(w);
  }
  if (w.desired_height.dim == Dim::Children) {
    w.computed_height = compute_children_height(w);
  }
}
```

**Pros**:
- Single pass
- No dependency chains
- Always computes something
- Predictable

**Cons**:
- Size-by-children requires second pass (but only for those widgets)
- Need to handle circular dependencies

#### Strategy 2: Iterative Constraint Solving

**Concept**: Use iterative algorithm to solve constraints until convergence.

```cpp
bool layout_iterative(Widget &root, int max_iterations = 10) {
  for (int iter = 0; iter < max_iterations; iter++) {
    reset_all_computed(root);
    
    // Try to compute everything
    bool all_computed = true;
    compute_pass(root, all_computed);
    
    if (all_computed) {
      return true; // Success!
    }
    
    // If not all computed, try again with what we have
    // Some values might be estimates now
  }
  
  return false; // Didn't converge
}

void compute_pass(Widget &w, bool &all_computed) {
  // Try standalone first
  if (w.desired_width.dim == Dim::Pixels || w.desired_width.dim == Dim::ScreenPercent) {
    w.computed_width = compute_standalone(w, X);
  }
  
  // Try with parent if available
  if (w.desired_width.dim == Dim::Percent) {
    if (w.parent && w.parent->computed_width >= 0) {
      w.computed_width = w.desired_width.value * w.parent->computed_width;
    } else {
      all_computed = false;
      // Use estimate
      w.computed_width = estimate_size(w, X);
    }
  }
  
  // Try with children
  if (w.desired_width.dim == Dim::Children) {
    float children_width = 0;
    bool children_ready = true;
    for (auto &child : w.children) {
      compute_pass(child, all_computed);
      if (child.computed_width < 0) {
        children_ready = false;
        break;
      }
      children_width += child.computed_width;
    }
    if (children_ready) {
      w.computed_width = children_width;
    } else {
      all_computed = false;
      w.computed_width = estimate_size(w, X);
    }
  }
  
  // Same for height...
}
```

**Pros**:
- Can handle complex dependencies
- Converges to solution
- Works with existing multi-pass structure

**Cons**:
- May take multiple iterations
- Slower than single-pass
- Still has dependency issues (just retries)

#### Strategy 3: Constraint Graph with Topological Sort

**Concept**: Build dependency graph, solve in correct order.

```cpp
struct ConstraintNode {
  Widget *widget;
  std::vector<ConstraintNode*> depends_on;
  bool computed = false;
};

void layout_with_dependencies(Widget &root) {
  // Build dependency graph
  std::vector<ConstraintNode> nodes;
  build_dependency_graph(root, nodes);
  
  // Topological sort
  std::vector<ConstraintNode*> sorted;
  topological_sort(nodes, sorted);
  
  // Compute in order
  for (auto *node : sorted) {
    compute_widget_size(*node->widget);
    node->computed = true;
  }
  
  // Position children
  compute_positions(root);
}

void build_dependency_graph(Widget &w, std::vector<ConstraintNode> &nodes) {
  ConstraintNode node;
  node.widget = &w;
  
  // Find dependencies
  if (w.desired_width.dim == Dim::Percent) {
    // Depends on parent
    if (w.parent) {
      node.depends_on.push_back(find_node(nodes, w.parent));
    }
  }
  
  if (w.desired_width.dim == Dim::Children) {
    // Depends on children
    for (auto &child : w.children) {
      build_dependency_graph(child, nodes);
      node.depends_on.push_back(find_node(nodes, &child));
    }
  }
  
  nodes.push_back(node);
}
```

**Pros**:
- Computes in correct order
- Handles complex dependencies
- Predictable

**Cons**:
- More complex to implement
- Requires graph building
- Still multi-pass conceptually

### Best Practices from Research

1. **Pass Available Space Down**
   - Instead of computing from parent size, pass available space to children
   - Eliminates parent dependency for percent-based sizing
   - Example: `layout(child, available_width, available_height)`

2. **Compute Bottom-Up for Size-by-Children**
   - First pass: compute all standalone sizes
   - Second pass: compute size-by-children from bottom up
   - Only need two passes, not three

3. **Use Estimates for Unavailable Values**
   - If parent not ready, use screen percentage as estimate
   - If children not ready, use desired size or 0
   - Better than `-1.0` which breaks rendering

4. **Separate Size Computation from Positioning**
   - First: compute all sizes
   - Second: compute all positions
   - Clearer separation of concerns

5. **Validate After Each Pass**
   - Check for uncomputed values
   - Log warnings for estimates used
   - Fail fast on impossible constraints

### Recommended Implementation Approach

**Hybrid: Two-Pass with Estimates**

```cpp
void autolayout_improved(UIComponent &widget,
                        const window_manager::Resolution resolution,
                        const std::map<EntityID, RefEntity> &map) {
  AutoLayout al(resolution, map);
  
  // PASS 1: Compute sizes (with estimates for unavailable values)
  al.reset_computed_values(widget);
  compute_sizes_with_estimates(widget, al);
  
  // PASS 2: Refine size-by-children (bottom-up)
  refine_children_sizes(widget, al);
  
  // PASS 3: Position children
  al.compute_relative_positions(widget);
  
  // PASS 4: Final screen positions
  al.compute_rect_bounds(widget);
  
  // Validate
  validate_layout(widget, al);
}

void compute_sizes_with_estimates(UIComponent &widget, AutoLayout &al) {
  // Standalone sizes (always work)
  if (widget.desired[Axis::X].dim == Dim::Pixels ||
      widget.desired[Axis::X].dim == Dim::ScreenPercent ||
      widget.desired[Axis::X].dim == Dim::Text) {
    widget.computed[Axis::X] = al.compute_size_for_standalone_exp(widget, Axis::X);
  }
  
  // Percent sizes (use estimate if parent not ready)
  if (widget.desired[Axis::X].dim == Dim::Percent) {
    if (widget.parent != -1) {
      UIComponent &parent = al.to_cmp(widget.parent);
      if (parent.computed[Axis::X] >= 0) {
        widget.computed[Axis::X] = al.compute_size_for_parent_expectation(widget, Axis::X);
      } else {
        // Estimate: use screen percentage
        float screen_w = al.fetch_screen_value_(Axis::X);
        widget.computed[Axis::X] = widget.desired[Axis::X].value * screen_w * 0.5f; // Conservative
        log_trace("Using estimate for widget {} width (parent not ready)", widget.id);
      }
    } else {
      // Root widget - use screen
      float screen_w = al.fetch_screen_value_(Axis::X);
      widget.computed[Axis::X] = widget.desired[Axis::X].value * screen_w;
    }
  }
  
  // Children sizes (defer to pass 2)
  if (widget.desired[Axis::X].dim == Dim::Children) {
    widget.computed[Axis::X] = 0; // Placeholder, will be computed in pass 2
  }
  
  // Same for Y axis...
  
  // Recursively compute children
  for (EntityID child_id : widget.children) {
    compute_sizes_with_estimates(al.to_cmp(child_id), al);
  }
}

void refine_children_sizes(UIComponent &widget, AutoLayout &al) {
  // First refine children
  for (EntityID child_id : widget.children) {
    refine_children_sizes(al.to_cmp(child_id), al);
  }
  
  // Then compute size-by-children if needed
  if (widget.desired[Axis::X].dim == Dim::Children) {
    widget.computed[Axis::X] = al.compute_size_for_child_expectation(widget, Axis::X);
  }
  if (widget.desired[Axis::Y].dim == Dim::Children) {
    widget.computed[Axis::Y] = al.compute_size_for_child_expectation(widget, Axis::Y);
  }
}
```

**Benefits**:
- Always computes valid values (no `-1.0`)
- Only two passes needed for sizes
- Estimates allow rendering even with incomplete layout
- Clear separation of concerns
- Minimal changes to existing code

### Performance Considerations

**Frame Budget**: 16.67ms for 60 FPS
- Layout should take < 5ms ideally
- Single-pass is fastest
- Multiple iterations add overhead
- Cache computed values when possible

**Optimization Strategies**:
1. Early exit if nothing changed
2. Only recompute dirty widgets
3. Batch constraint solving
4. Use SIMD for parallel computation (like Clay)
5. Arena allocation for temporary data

### References

- [Clay UI Library](https://github.com/nicbarker/clay) - Single-pass, high-performance layout
- [Immediate Mode UI](https://en.wikipedia.org/wiki/Immediate_mode_(computer_graphics)) - Wikipedia
- [Constraint Solving for UI Layout](https://arxiv.org/abs/1309.7001) - Kaczmarz algorithm with soft constraints
- [CSS Flexbox Algorithm](https://www.w3.org/TR/css-flexbox-1/#layout-algorithm) - W3C specification

### Absolutely Positioned Elements Issue:

Looking at line 797-798:
```cpp
if (widget.absolute && exp.dim == Dim::Percent) {
    VALIDATE(false, "Absolute widgets should not use Percent");
}
```

But absolutely positioned elements still need their sizes computed! They just don't participate in parent layout. The size computation should still happen in `calculate_standalone` or `calculate_those_with_parents`.

**The fix**: Absolutely positioned elements should compute their sizes in `calculate_standalone` if they use `Dim::Pixels` or `Dim::ScreenPercent`, but they're probably using `Dim::Percent` which requires a parent, and that's failing.

---

## Ryan Fleury's UI Library Blog Series

**Source**: [Digital Grove by Ryan Fleury](https://www.rfleury.com)

Ryan Fleury has written a comprehensive series of blog posts about building a UI library from first principles. The series covers architecture, implementation, and best practices. Here's what we can learn from each part:

### Part 1: The Interaction Medium
**Source**: [UI, Part 1: The Interaction Medium](https://www.rfleury.com/p/ui-part-1-the-interaction-medium)

**Key Concepts:**

1. **The User/Programmer Information Cycle**
   - UIs serve as the communication bridge between users and programmers
   - Both parties need a **shared agreement** about the interface structure
   - When a user clicks a button, they communicate information; when the screen changes, the program communicates back
   - This mutual understanding is essential for effective interaction

2. **The Cost of Transmitting Information**
   - Sending/receiving information is not free
   - Goal: Achieve the smallest possible **bits sent** to **useful bits received** ratio
   - Minimize time spent by user AND program
   - Example: Typing "Hello, Mr. Program. I would like you to toggle the checkbox, please." (68 bytes) vs clicking (1 bit) - the click is massively more efficient

3. **The Common Language of User Interfaces**
   - Standard widgets (buttons, checkboxes, sliders, text fields, scroll bars) form a "common language"
   - Users can reuse existing knowledge from other programs
   - Reduces learning time and cognitive load

4. **The Importance of Appearance and Animation**
   - **Signifiers**: Hints/clues that communicate how objects should be interacted with
   - Visual feedback (embossed buttons, hover states, press states)
   - Animation smoothly represents state changes at human-interpretable rates
   - Dynamic appearances that respond to user action

5. **Minimizing Code Size and Complexity**
   - UI code often spirals out of control
   - Goal: Spend as little time and code as possible building/maintaining UIs
   - Keep code simple and robust-to-change
   - Interfaces rarely stay the same - need low-friction changes

6. **Core vs Builder Code**
   - **Core**: Implements common codepaths and helpers
   - **Builder Code**: Uses the core to arrange widget instances
   - Builder code is larger and changes more frequently
   - **Critical**: Make builder code as small and easily-controllable as possible
   - Less disaster to have complexity in core (but still avoid unnecessary complexity)

7. **Escape Hatches**
   - Core cannot assume it knows all widget designs
   - Must provide "escape hatches" - ways for builder code to bypass common codepaths
   - Allows specialized widgets when standard ones aren't sufficient
   - Builder can compose with smaller subset of API-provided codepaths

**Relevance to Your System:**
- Your autolayout unpredictability breaks the "shared understanding" - users see one thing, code expects another
- The `-1.0` computed values mean the system isn't communicating its state clearly
- Your builder pattern (`ComponentConfig`) aligns with Fleury's "builder code" concept
- The complexity in your autolayout system violates the "minimize complexity" principle
- Your system needs better "escape hatches" for edge cases (like absolutely positioned elements)

### Part 2: Build It Every Frame (Immediate Mode)
**Source**: [UI, Part 2: Build It Every Frame](https://www.rfleury.com/p/ui-part-2-build-it-every-frame-immediate)

**Key Concepts:**

1. **Immediate Mode User Interfaces (IMGUI)**
   - UI is constructed **anew each frame** from scratch
   - No retained state between frames
   - Entire widget hierarchy is rebuilt every frame
   - Contrasts with retained-mode APIs that maintain persistent UI state

2. **Benefits of Immediate Mode**
   - **Localized code**: Each widget's definition and behavior is in one place
   - **Simplified state management**: No need to sync UI state with application state
   - **More straightforward codebase**: Less complex than retained-mode
   - **Enhanced flexibility**: Easy to make dynamic changes
   - **Rapid iteration**: Changes are immediately reflected

3. **How It Works**
   - Each frame, the UI code runs and describes what the UI should look like
   - The system computes layout, handles input, and renders
   - Next frame, start over - no memory of previous frame's UI structure
   - Application state drives what UI is described, not stored UI state

4. **State Management**
   - Application manages state, not UI library
   - UI is a pure function of application state
   - No need to track which widgets exist, their properties, etc.
   - Eliminates state synchronization bugs

**Relevance to Your System:**
- **This directly addresses your autolayout issues!**
- Your system uses retained entities with multi-pass layout - this creates dependency chains
- Immediate-mode layout would recalculate everything every frame, eliminating stale `-1.0` values
- Clay uses this approach and doesn't have your dependency problems
- Your builder pattern could work with immediate-mode layout computation
- You already run every frame, but the multi-pass algorithm structure is the problem

**Key Insight**: The builder factory style you like can coexist with immediate-mode layout - they're orthogonal concerns. Builder describes what you want, immediate-mode computes it fresh.

### Part 3: The Widget Building Language
**Source**: [UI, Part 3: The Widget Building Language](https://www.rfleury.com/p/ui-part-3-the-widget-building-language)

**Key Concepts:**

1. **Core Architecture**
   - Balance between **core code** (common functionality) and **builder code** (specific instances)
   - Core provides essential functionalities
   - Builder code utilizes core to create specific UI components
   - Modular design enhances maintainability and scalability

2. **Widget Features**
   - Various features that widgets might possess:
     - **Clickability**: Responding to mouse clicks
     - **Keyboard focus**: Tab navigation, focus management
     - **Scrolling**: Scrollable containers
     - **Rendering effects**: Visual styling, animations
     - **Layout**: Sizing, positioning, flex behavior
   - Features can be combined in various ways
   - System must handle numerous feature combinations without excessive complexity

3. **Flexible Feature System**
   - Core provides building blocks
   - Builder code composes features as needed
   - Avoid hardcoding specific widget types
   - Allow feature combinations that weren't anticipated

4. **Builder Code Goals**
   - **Minimal**: As small as possible
   - **Adaptable**: Easy to change and extend
   - **Clear**: Obvious what each piece does
   - **Composable**: Features can be mixed and matched

5. **Core Code Goals**
   - Provide common codepaths
   - Handle feature interactions correctly
   - Fastpaths for common widget designs (buttons, checkboxes, etc.)
   - Escape hatches for specialized needs

**Relevance to Your System:**
- Your `ComponentConfig` builder aligns with this approach
- The issue isn't the builder API itself, but how it interacts with layout computation
- Fleury emphasizes "minimal" - your builder might be accumulating too many concerns
- Your system separates concerns well (ComponentConfig for spec, autolayout for computation)
- The problem is the layout computation algorithm, not the builder pattern
- Your feature system (padding, margin, colors, fonts, etc.) matches Fleury's "widget features" concept

**Key Insight**: Your builder pattern is good! The problem is the layout computation system that the builder feeds into. Keep the builder, fix the layout algorithm.

### Part 4: The Widget Is A Lie (Node Composition)
**Key Concepts:**
- Challenges traditional widget-based hierarchies
- Proposes **node composition** approach instead of explicit widgets
- UI elements constructed from nodes dynamically
- More flexible and maintainable
- Promotes reusability and modularity
- Dynamic hierarchies easier to create and modify

**Relevance to Your System:**
- Your system uses explicit widgets (Button, Div, etc.) with `ComponentConfig`
- Node composition might simplify the relationship between components and layout
- Could reduce the complexity of your autolayout passes
- More dynamic composition might help with the dependency chain issues

### Part 5: Visual Content Specification
**Key Concepts:**
- Specifying visual information (styling and spacing) within builder codepath
- Encoding spacing and styling effectively
- Methods for defining appearance and layout programmatically
- Visual content specification as part of the builder flow

**Relevance to Your System:**
- Your `ComponentConfig` handles this with `.with_padding()`, `.with_margin()`, `.with_custom_color()`
- This is working well in your system
- The issue is how these specifications translate to computed layout values
- Suggests the builder → layout translation might need improvement

### Part 6: Rendering
**Key Concepts:**
- Leveraging GPU to render sophisticated UIs efficiently
- High-performance rendering with minimal code
- Efficient rendering practices for responsive interfaces
- GPU-based techniques for better performance

**Relevance to Your System:**
- Less directly relevant to your autolayout issues
- But performance matters - if layout is slow, immediate-mode becomes harder
- Your rendering system seems separate from layout (good separation of concerns)

### Part 7: State Management and Input Handling
**Key Concepts:**
- Managing state mutations effectively
- Addressing issues like jank
- Implementing hotkeys
- State management to prevent UI lag
- Ensuring UI responds promptly to interactions

**Relevance to Your System:**
- Your autolayout timing issues might relate to state management
- The `-1.0` values suggest state isn't being managed correctly across layout passes
- Jank could be related to layout not completing before rendering
- State management patterns might help with the dependency chain problem

### Part 8: Performance Considerations
**Key Concepts:**
- Strategies to optimize UI performance
- Reducing latency
- Ensuring smooth user interactions
- Performance optimization techniques

**Relevance to Your System:**
- Your multi-pass layout might be causing performance issues
- Multiple iterations through the tree could be optimized
- Immediate-mode can be fast if done right (see Clay's microsecond performance)
- Performance and correctness are both important

### Part 9: Keyboard and Gamepad Navigation
**Key Focus**: "Techniques for supporting keyboard and gamepad navigation without overburdening builder code"

**Key Concepts:**
- Adding navigation features without complicating builder API
- Techniques to keep builder code clean while supporting complex input handling
- Balancing functionality with API simplicity
- Integrating alternative input methods seamlessly

**Relevance to Your System:**
- **Directly addresses your concern**: "builder factory style is good but hard to use sometimes"
- Suggests there are patterns for adding features without breaking ergonomics
- Your builder might be accumulating too many concerns (layout + navigation + styling)
- Could inform how to refactor without losing the builder pattern you like

---

## Key Takeaways from Ryan Fleury's Series

### 1. **Immediate-Mode Layout Solves Your Problem**
Part 2's emphasis on immediate-mode directly addresses your autolayout unpredictability. Your multi-pass system creates dependency chains that fail. Immediate-mode recalculates everything every frame, eliminating these issues.

### 2. **Builder Pattern + Immediate-Mode = Best of Both Worlds**
Parts 3 and 9 show that:
- Builder patterns are good for ergonomics (you're right to like yours)
- But layout computation should be immediate-mode (recalculate every frame)
- These are separate concerns that can coexist

### 3. **Node Composition Might Simplify Things**
Part 4's node composition approach could reduce the complexity of your autolayout passes. Instead of explicit widgets with complex relationships, nodes compose more dynamically.

### 4. **Keep Builders Minimal**
Parts 3 and 9 emphasize keeping builder code minimal. Your `ComponentConfig` might be accumulating too many concerns. Consider separating:
- Visual specification (colors, fonts) ✅ You have this
- Layout specification (sizes, padding) ✅ You have this
- Layout computation ❌ This should be separate and immediate-mode

### 5. **State Management Matters**
Part 7's focus on state management relates to your `-1.0` problem. The layout system needs to manage state correctly across passes, or better yet, eliminate the need for multi-pass state.

---

## Recommendations Based on Ryan Fleury's Series

1. **Adopt Immediate-Mode Layout Computation**
   - Recalculate layout every frame
   - Don't cache computed values between frames
   - This fixes the `-1.0` dependency chain problem

2. **Keep Your Builder Pattern**
   - It's ergonomic and you like it
   - But separate layout specification from layout computation
   - Builder specifies what you want, immediate-mode computes it fresh

3. **Consider Node Composition**
   - Might simplify your autolayout passes
   - More dynamic composition could reduce dependency issues

4. **Minimize Builder Concerns**
   - Your builder is doing too much
   - Separate visual spec, layout spec, and layout computation
   - Keep builder focused on specification, not computation

5. **Fix State Management**
   - The `-1.0` values suggest state isn't managed correctly
   - Immediate-mode eliminates most state management issues
   - If keeping multi-pass, need better state tracking

---

## Connection to Clay UI

Both Clay and Ryan Fleury's series advocate for:
- **Immediate-mode layout computation** (Clay does it, Fleury recommends it)
- **Simple, predictable systems** (Clay's single-pass, Fleury's minimal builders)
- **Separation of concerns** (Clay's renderer-agnostic design, Fleury's builder vs computation)

Your system has the right idea with the builder pattern, but the layout computation is the problem. Both sources point to immediate-mode as the solution.

---

## Making Automatic Layouts Beautiful

Based on research into magazine design, website layouts, and professional design systems, here are principles for making automatic layouts look good:

### Core Design Principles for Automatic Layouts

#### 1. **Grid Systems**
**Concept**: Use structured grids to organize content automatically.

**Implementation**:
- **8pt Grid System**: All spacing values are multiples of 8px (8, 16, 24, 32, 40, 48, etc.)
- **Column Grids**: Divide screen into columns (12-column, 16-column common)
- **Baseline Grid**: Align text to vertical rhythm (typically 4px or 8px increments)

**Benefits**:
- Automatic alignment
- Visual consistency
- Easy to implement programmatically
- Professional appearance

**Code Example**:
```cpp
// 8pt grid spacing system
inline float grid_8pt(int units) {
  return units * 8.0f;
}

// Automatic spacing based on grid
ComponentConfig{}
  .with_padding(Padding{
    .top = grid_8pt(2),    // 16px
    .right = grid_8pt(2),  // 16px
    .bottom = grid_8pt(2), // 16px
    .left = grid_8pt(2)    // 16px
  })
  .with_margin(Margin{
    .top = grid_8pt(3),    // 24px
    .bottom = grid_8pt(3)  // 24px
  })
```

#### 2. **White Space (Negative Space)**
**Concept**: Generous spacing makes layouts breathe and look professional.

**Key Rules**:
- **More space is usually better** than less
- **Consistent spacing** creates rhythm
- **Group related elements** with less space between them
- **Separate unrelated elements** with more space

**Automatic Spacing Rules**:
```cpp
// Automatic spacing based on element relationships
struct AutoSpacing {
  // Between siblings
  static float sibling_gap() { return grid_8pt(2); } // 16px
  
  // Between sections
  static float section_gap() { return grid_8pt(4); } // 32px
  
  // Around containers
  static float container_padding() { return grid_8pt(3); } // 24px
  
  // Between text and elements
  static float text_margin() { return grid_8pt(2); } // 16px
};
```

#### 3. **Visual Hierarchy**
**Concept**: Size, spacing, and color create natural reading flow.

**Automatic Hierarchy Rules**:
- **Larger elements** = more important
- **More spacing around** = more emphasis
- **Darker/bolder** = more important
- **Top-left** = primary focus (F-pattern reading)

**Implementation**:
```cpp
// Automatic hierarchy based on element type
ComponentConfig auto_hierarchy(ElementType type) {
  switch (type) {
    case ElementType::Heading:
      return ComponentConfig{}
        .with_margin(Margin{.bottom = grid_8pt(2)})
        .with_font_size(24.0f);
        
    case ElementType::Body:
      return ComponentConfig{}
        .with_margin(Margin{.bottom = grid_8pt(3)})
        .with_font_size(16.0f);
        
    case ElementType::Caption:
      return ComponentConfig{}
        .with_margin(Margin{.top = grid_8pt(1), .bottom = grid_8pt(2)})
        .with_font_size(12.0f);
  }
}
```

#### 4. **Typography Scale**
**Concept**: Use mathematical scale for font sizes (creates harmony).

**Common Scales**:
- **Major Third**: 1.25x ratio (1.0, 1.25, 1.563, 1.953, 2.441)
- **Perfect Fourth**: 1.333x ratio (1.0, 1.333, 1.777, 2.37, 3.16)
- **Golden Ratio**: 1.618x ratio (1.0, 1.618, 2.618, 4.236)

**Implementation**:
```cpp
// Typographic scale
struct TypographyScale {
  static constexpr float BASE_SIZE = 16.0f;
  static constexpr float RATIO = 1.25f; // Major Third
  
  static float size(int level) {
    return BASE_SIZE * std::pow(RATIO, level);
  }
  
  // Level 0: 16px (body)
  // Level 1: 20px (subheading)
  // Level 2: 25px (heading)
  // Level 3: 31px (title)
  // Level 4: 39px (display)
};
```

#### 5. **Consistent Spacing Rhythm**
**Concept**: Vertical rhythm creates visual flow.

**Baseline Grid**:
- All text aligns to vertical grid
- Line height = font size × 1.5 (or 1.618 for golden ratio)
- Spacing between elements = multiple of line height

**Implementation**:
```cpp
float compute_line_height(float font_size) {
  return font_size * 1.5f; // Or 1.618 for golden ratio
}

float compute_vertical_rhythm(float font_size, int lines) {
  return compute_line_height(font_size) * lines;
}

// Automatic spacing that respects rhythm
ComponentConfig{}
  .with_font_size(16.0f)
  .with_margin(Margin{
    .top = compute_vertical_rhythm(16.0f, 1),  // 24px (1 line)
    .bottom = compute_vertical_rhythm(16.0f, 1) // 24px (1 line)
  })
```

#### 6. **Proportional Spacing**
**Concept**: Use ratios for spacing relationships.

**Golden Ratio Spacing**:
- If element A has spacing X
- Related element B should have spacing X × 1.618 or X / 1.618
- Creates natural, pleasing relationships

**Container Padding Rule**:
- Padding should be ~1/8 to 1/12 of container width
- For 800px container: 66-100px padding
- For 1200px container: 100-150px padding

**Implementation**:
```cpp
float golden_ratio_spacing(float base) {
  return base * 1.618f;
}

float proportional_padding(float container_width) {
  return container_width / 10.0f; // ~10% padding
}
```

#### 7. **Alignment Rules**
**Concept**: Consistent alignment creates order.

**Automatic Alignment**:
- **Text**: Left-align (or center for headings)
- **Numbers**: Right-align
- **Images**: Center or align to grid
- **Buttons**: Align to content or grid

**Grid Alignment**:
```cpp
// Snap to 8pt grid
float snap_to_grid(float value) {
  return std::round(value / 8.0f) * 8.0f;
}

// Align to container
float align_in_container(float element_size, float container_size, Alignment align) {
  switch (align) {
    case Alignment::Left:
      return 0;
    case Alignment::Center:
      return (container_size - element_size) / 2.0f;
    case Alignment::Right:
      return container_size - element_size;
  }
}
```

### Automatic Layout Presets

#### Magazine-Style Layout
```cpp
ComponentConfig magazine_style() {
  return ComponentConfig{}
    .with_padding(Padding{
      .top = grid_8pt(6),    // 48px
      .right = grid_8pt(4),   // 32px
      .bottom = grid_8pt(6),  // 48px
      .left = grid_8pt(4)     // 32px
    })
    .with_font_size(TypographyScale::size(0)) // 16px
    .with_flex_direction(FlexDirection::Column)
    .with_margin(Margin{.bottom = grid_8pt(4)}); // 32px between sections
}
```

#### Card Layout
```cpp
ComponentConfig card_style() {
  return ComponentConfig{}
    .with_padding(Padding{grid_8pt(3)}) // 24px all around
    .with_margin(Margin{grid_8pt(2)})    // 16px all around
    .with_rounded_corners(RoundedCorners::All)
    .with_custom_color(card_background_color);
}
```

#### Form Layout
```cpp
ComponentConfig form_field_style() {
  return ComponentConfig{}
    .with_margin(Margin{.bottom = grid_8pt(3)}) // 24px between fields
    .with_padding(Padding{
      .top = grid_8pt(2),
      .right = grid_8pt(2),
      .bottom = grid_8pt(2),
      .left = grid_8pt(2)
    });
}
```

### Default Spacing System

**Recommended Defaults**:
```cpp
struct DefaultSpacing {
  // Tiny spacing (for tight groups)
  static float tiny() { return grid_8pt(1); }  // 8px
  
  // Small spacing (between related elements)
  static float small() { return grid_8pt(2); } // 16px
  
  // Medium spacing (between sections)
  static float medium() { return grid_8pt(3); } // 24px
  
  // Large spacing (major separations)
  static float large() { return grid_8pt(4); }  // 32px
  
  // Extra large (page sections)
  static float xlarge() { return grid_8pt(6); } // 48px
  
  // Container padding
  static float container() { return grid_8pt(4); } // 32px
};
```

### Automatic Spacing Based on Context

**Smart Spacing Rules**:
```cpp
ComponentConfig auto_spacing(ElementType type, ElementType previous_type) {
  ComponentConfig config;
  
  // More space after headings
  if (previous_type == ElementType::Heading) {
    config.with_margin(Margin{.top = DefaultSpacing::medium()});
  }
  
  // Less space between related elements
  if (type == previous_type) {
    config.with_margin(Margin{.top = DefaultSpacing::small()});
  }
  
  // More space around containers
  if (type == ElementType::Container) {
    config.with_padding(Padding{DefaultSpacing::container()});
  }
  
  return config;
}
```

### Typography Best Practices

**Line Height**:
- Body text: 1.5x font size (16px font = 24px line height)
- Headings: 1.2x font size (24px font = 29px line height)
- Tight spacing: 1.25x font size

**Letter Spacing**:
- Body text: 0 (default)
- Headings: -0.5px to -1px (tighter)
- Small text: +0.5px (more readable)

**Paragraph Spacing**:
- 1.5x to 2x line height between paragraphs
- For 16px font with 24px line height: 36-48px between paragraphs

### Color and Contrast

**Automatic Color Rules**:
- **Text on light background**: Dark gray (#333 or darker)
- **Text on dark background**: Light gray (#CCC or lighter)
- **Minimum contrast ratio**: 4.5:1 for body text, 3:1 for large text
- **Accent colors**: Use sparingly (10% of layout max)

### Responsive Spacing

**Scale spacing with screen size**:
```cpp
float responsive_spacing(float base_spacing, float screen_width) {
  // Scale spacing proportionally to screen size
  // Base: 1280px screen
  float scale = screen_width / 1280.0f;
  return base_spacing * scale;
  
  // Or use breakpoints
  if (screen_width < 768) {
    return base_spacing * 0.75f; // Smaller on mobile
  }
  return base_spacing;
}
```

### Implementation in Your System

**Add to ComponentConfig**:
```cpp
// Automatic beautiful defaults
ComponentConfig &with_auto_spacing() {
  padding = Padding{DefaultSpacing::small()};
  margin = Margin{DefaultSpacing::medium()};
  return *this;
}

ComponentConfig &with_magazine_style() {
  return magazine_style();
}

ComponentConfig &with_card_style() {
  return card_style();
}
```

**Update Autolayout**:
- Snap all computed positions to 8pt grid
- Ensure spacing respects vertical rhythm
- Align text to baseline grid

### Key Takeaways

1. **Use 8pt Grid**: All spacing multiples of 8px
2. **Generous White Space**: More space usually looks better
3. **Consistent Rhythm**: Vertical spacing follows typographic scale
4. **Visual Hierarchy**: Size and spacing create importance
5. **Proportional Relationships**: Use ratios (golden ratio, etc.)
6. **Alignment**: Snap to grid, consistent alignment
7. **Typography Scale**: Mathematical font size relationships
8. **Context-Aware Spacing**: Different spacing for different relationships

### References

- [Adobe InDesign Magazine Layout Guide](https://www.adobe.com/learn/indesign/web/design-magazine-layout)
- [8pt Grid System](https://spec.fm/specifics/8-pt-grid) - Design system spacing
- [Typography Scale](https://type-scale.com/) - Visual typography calculator
- [Material Design Spacing](https://material.io/design/layout/spacing-methods.html) - Google's spacing system
- [Baseline Grid](https://www.smashingmagazine.com/2012/12/css-baseline-the-good-the-bad-and-the-ugly/) - Typography rhythm

---

## Unit Testing the Autolayout System

Based on your existing test infrastructure, here's how to write unit tests for autolayout fixes:

### Current Test Infrastructure

**You have**:
- ✅ Catch2 framework available (`vendor/afterhours/vendor/catch2/catch.hpp`)
- ✅ Example unit tests in `vendor/afterhours/example/tests/main.cpp`
- ✅ Integration tests in `src/testing/tests/`

**You need**:
- Unit tests for autolayout computation functions
- Tests for the multi-pass algorithm fixes
- Tests for estimate fallbacks
- Tests for dependency chain handling

### Setting Up Unit Tests

**Create test file**: `src/testing/tests/AutolayoutTest.h`

```cpp
#pragma once

#define CATCH_CONFIG_MAIN
#include "../../../vendor/afterhours/vendor/catch2/catch.hpp"
#include <afterhours/ah.h>
#include <map>

using namespace afterhours;
using namespace afterhours::ui;

// Helper to create entity mapping for tests
std::map<EntityID, RefEntity> create_test_mapping(UIComponent &widget) {
  std::map<EntityID, RefEntity> map;
  // Add widget and children to mapping
  // Implementation depends on your Entity system
  return map;
}
```

### Test Categories

#### 1. Standalone Size Computation

**Test**: Sizes that don't depend on parent/children should always compute.

```cpp
TEST_CASE("Standalone sizes always compute", "[Autolayout]") {
  window_manager::Resolution resolution{1280, 720};
  std::map<EntityID, RefEntity> empty_map;
  AutoLayout al(resolution, empty_map);
  
  UIComponent widget(1);
  widget.desired[Axis::X] = pixels(100.0f);
  widget.desired[Axis::Y] = pixels(200.0f);
  
  al.calculate_standalone(widget);
  
  REQUIRE(widget.computed[Axis::X] == 100.0f);
  REQUIRE(widget.computed[Axis::Y] == 200.0f);
}

TEST_CASE("ScreenPercent computes correctly", "[Autolayout]") {
  window_manager::Resolution resolution{1280, 720};
  std::map<EntityID, RefEntity> empty_map;
  AutoLayout al(resolution, empty_map);
  
  UIComponent widget(1);
  widget.desired[Axis::X] = screen_pct(0.5f);  // 50% of screen width
  widget.desired[Axis::Y] = screen_pct(0.25f); // 25% of screen height
  
  al.calculate_standalone(widget);
  
  REQUIRE(widget.computed[Axis::X] == 640.0f);  // 1280 * 0.5
  REQUIRE(widget.computed[Axis::Y] == 180.0f); // 720 * 0.25
}

TEST_CASE("Standalone never returns -1.0", "[Autolayout]") {
  window_manager::Resolution resolution{1280, 720};
  std::map<EntityID, RefEntity> empty_map;
  AutoLayout al(resolution, empty_map);
  
  UIComponent widget(1);
  widget.desired[Axis::X] = pixels(100.0f);
  widget.desired[Axis::Y] = pixels(200.0f);
  
  al.calculate_standalone(widget);
  
  REQUIRE(widget.computed[Axis::X] >= 0.0f);
  REQUIRE(widget.computed[Axis::Y] >= 0.0f);
  REQUIRE(widget.computed[Axis::X] != -1.0f);
  REQUIRE(widget.computed[Axis::Y] != -1.0f);
}
```

#### 2. Parent-Dependent Sizes with Estimates

**Test**: When parent isn't ready, use estimates instead of `-1.0`.

```cpp
TEST_CASE("Parent-dependent uses estimate when parent not ready", "[Autolayout]") {
  window_manager::Resolution resolution{1280, 720};
  std::map<EntityID, RefEntity> map;
  AutoLayout al(resolution, map);
  
  UIComponent parent(1);
  parent.desired[Axis::X] = children(); // Parent depends on children
  parent.computed[Axis::X] = -1.0f;     // Not computed yet
  
  UIComponent child(2);
  child.parent = parent.id;
  child.desired[Axis::X] = percent(0.5f); // 50% of parent
  
  // Before fix: would return -1.0
  // After fix: should use estimate
  al.calculate_those_with_parents(child);
  
  // Should have an estimate, not -1.0
  REQUIRE(child.computed[Axis::X] >= 0.0f);
  REQUIRE(child.computed[Axis::X] != -1.0f);
  
  // Estimate should be reasonable (e.g., based on screen size)
  REQUIRE(child.computed[Axis::X] > 0.0f);
  REQUIRE(child.computed[Axis::X] <= 1280.0f);
}

TEST_CASE("Parent-dependent computes correctly when parent ready", "[Autolayout]") {
  window_manager::Resolution resolution{1280, 720};
  std::map<EntityID, RefEntity> map;
  AutoLayout al(resolution, map);
  
  UIComponent parent(1);
  parent.computed[Axis::X] = 1000.0f; // Already computed
  parent.computed_margin[Axis::X] = 0.0f;
  
  UIComponent child(2);
  child.parent = parent.id;
  child.desired[Axis::X] = percent(0.5f); // 50% of parent
  
  al.calculate_those_with_parents(child);
  
  REQUIRE(child.computed[Axis::X] == 500.0f); // 1000 * 0.5
}
```

#### 3. Children-Dependent Sizes (Auto-Resizing)

**Test**: Size-by-children should compute after children are computed.

```cpp
TEST_CASE("Children-dependent computes after children ready", "[Autolayout]") {
  window_manager::Resolution resolution{1280, 720};
  std::map<EntityID, RefEntity> map;
  AutoLayout al(resolution, map);
  
  UIComponent parent(1);
  parent.desired[Axis::X] = children(); // Size by children
  parent.flex_direction = FlexDirection::Row;
  
  UIComponent child1(2);
  child1.parent = parent.id;
  child1.desired[Axis::X] = pixels(100.0f);
  child1.computed[Axis::X] = 100.0f; // Already computed
  
  UIComponent child2(3);
  child2.parent = parent.id;
  child2.desired[Axis::X] = pixels(200.0f);
  child2.computed[Axis::X] = 200.0f; // Already computed
  
  parent.children.push_back(child1.id);
  parent.children.push_back(child2.id);
  
  al.calculate_those_with_children(parent);
  
  // Parent should be sum of children (for Row layout)
  REQUIRE(parent.computed[Axis::X] == 300.0f); // 100 + 200
}

TEST_CASE("Children-dependent handles empty children", "[Autolayout]") {
  window_manager::Resolution resolution{1280, 720};
  std::map<EntityID, RefEntity> map;
  AutoLayout al(resolution, map);
  
  UIComponent parent(1);
  parent.desired[Axis::X] = children();
  parent.children.clear(); // No children
  
  al.calculate_those_with_children(parent);
  
  // Should return 0 or desired value, not -1.0
  REQUIRE(parent.computed[Axis::X] >= 0.0f);
  REQUIRE(parent.computed[Axis::X] != -1.0f);
}
```

#### 4. Multi-Pass Algorithm

**Test**: Full multi-pass should never leave `-1.0` values.

```cpp
TEST_CASE("Multi-pass never leaves -1.0 values", "[Autolayout]") {
  window_manager::Resolution resolution{1280, 720};
  std::map<EntityID, RefEntity> map;
  
  // Create complex hierarchy with dependencies
  UIComponent root(1);
  root.desired[Axis::X] = screen_pct(1.0f);
  root.desired[Axis::Y] = screen_pct(1.0f);
  
  UIComponent container(2);
  container.parent = root.id;
  container.desired[Axis::X] = children(); // Depends on children
  container.desired[Axis::Y] = percent(0.5f); // Depends on parent
  
  UIComponent child(3);
  child.parent = container.id;
  child.desired[Axis::X] = percent(0.5f); // Depends on parent
  child.desired[Axis::Y] = pixels(100.0f);
  
  container.children.push_back(child.id);
  root.children.push_back(container.id);
  
  // Run full autolayout
  AutoLayout::autolayout(root, resolution, map);
  
  // Check that nothing is -1.0
  auto check_no_neg_one = [](UIComponent &w) {
    REQUIRE(w.computed[Axis::X] != -1.0f);
    REQUIRE(w.computed[Axis::Y] != -1.0f);
    REQUIRE(w.computed[Axis::X] >= 0.0f);
    REQUIRE(w.computed[Axis::Y] >= 0.0f);
  };
  
  check_no_neg_one(root);
  check_no_neg_one(container);
  check_no_neg_one(child);
}

TEST_CASE("Multi-pass handles circular dependencies gracefully", "[Autolayout]") {
  // Test that estimates prevent infinite loops
  window_manager::Resolution resolution{1280, 720};
  std::map<EntityID, RefEntity> map;
  
  UIComponent parent(1);
  parent.desired[Axis::X] = children();
  
  UIComponent child(2);
  child.parent = parent.id;
  child.desired[Axis::X] = percent(1.0f); // 100% of parent
  
  parent.children.push_back(child.id);
  
  // Should complete without infinite loop
  AutoLayout::autolayout(parent, resolution, map);
  
  // Both should have valid values (estimates if needed)
  REQUIRE(parent.computed[Axis::X] >= 0.0f);
  REQUIRE(child.computed[Axis::X] >= 0.0f);
}
```

#### 5. Estimate Quality

**Test**: Estimates should be reasonable and usable.

```cpp
TEST_CASE("Estimates are reasonable for percent sizes", "[Autolayout]") {
  window_manager::Resolution resolution{1280, 720};
  std::map<EntityID, RefEntity> map;
  AutoLayout al(resolution, map);
  
  UIComponent widget(1);
  widget.parent = -1; // No parent
  widget.desired[Axis::X] = percent(0.5f); // 50%
  
  // When parent not available, estimate should be based on screen
  al.calculate_those_with_parents(widget);
  
  // Estimate should be reasonable (e.g., 50% of screen or less)
  REQUIRE(widget.computed[Axis::X] > 0.0f);
  REQUIRE(widget.computed[Axis::X] <= 1280.0f);
  
  // Should be in the ballpark of 50% of screen
  float expected_range_min = 200.0f;  // Conservative lower bound
  float expected_range_max = 800.0f;  // Conservative upper bound
  REQUIRE(widget.computed[Axis::X] >= expected_range_min);
  REQUIRE(widget.computed[Axis::X] <= expected_range_max);
}
```

#### 6. Absolutely Positioned Elements

**Test**: Absolutely positioned elements should still get sizes computed.

```cpp
TEST_CASE("Absolutely positioned elements get sizes computed", "[Autolayout]") {
  window_manager::Resolution resolution{1280, 720};
  std::map<EntityID, RefEntity> map;
  AutoLayout al(resolution, map);
  
  UIComponent widget(1);
  widget.absolute = true;
  widget.desired[Axis::X] = pixels(200.0f);
  widget.desired[Axis::Y] = pixels(100.0f);
  
  al.calculate_standalone(widget);
  
  // Absolutely positioned should still have computed sizes
  REQUIRE(widget.computed[Axis::X] == 200.0f);
  REQUIRE(widget.computed[Axis::Y] == 100.0f);
  REQUIRE(widget.computed[Axis::X] != -1.0f);
  REQUIRE(widget.computed[Axis::Y] != -1.0f);
}
```

#### 7. Constraint Solving

**Test**: Violation solving should work correctly.

```cpp
TEST_CASE("Constraint solving handles overflow", "[Autolayout]") {
  window_manager::Resolution resolution{1280, 720};
  std::map<EntityID, RefEntity> map;
  AutoLayout al(resolution, map);
  
  UIComponent parent(1);
  parent.computed[Axis::X] = 100.0f; // Small parent
  
  UIComponent child1(2);
  child1.parent = parent.id;
  child1.desired[Axis::X] = pixels(60.0f);
  child1.computed[Axis::X] = 60.0f;
  child1.desired[Axis::X].strictness = 1.0f; // Strict
  
  UIComponent child2(3);
  child2.parent = parent.id;
  child2.desired[Axis::X] = pixels(60.0f);
  child2.computed[Axis::X] = 60.0f;
  child2.desired[Axis::X].strictness = 0.0f; // Flexible
  
  parent.children.push_back(child1.id);
  parent.children.push_back(child2.id);
  
  // Children total 120, parent is 100 - violation!
  al.solve_violations(parent);
  
  // Flexible child should be reduced
  REQUIRE(child2.computed[Axis::X] < 60.0f);
  REQUIRE(child2.computed[Axis::X] >= 0.0f);
  
  // Strict child should remain
  REQUIRE(child1.computed[Axis::X] == 60.0f);
}
```

### Test Structure Template

```cpp
#pragma once

#define CATCH_CONFIG_MAIN
#include "../../../vendor/afterhours/vendor/catch2/catch.hpp"
#include <afterhours/ah.h>

using namespace afterhours;
using namespace afterhours::ui;

TEST_CASE("Test name", "[Category]") {
  // Setup
  window_manager::Resolution resolution{1280, 720};
  std::map<EntityID, RefEntity> map;
  AutoLayout al(resolution, map);
  
  // Create test widgets
  UIComponent widget(1);
  // ... configure widget ...
  
  // Execute
  al.calculate_standalone(widget);
  // or: AutoLayout::autolayout(widget, resolution, map);
  
  // Assert
  REQUIRE(widget.computed[Axis::X] == expected_value);
  REQUIRE(widget.computed[Axis::X] != -1.0f);
}
```

### Running Unit Tests

**Option 1: Separate test executable**

Create `src/testing/tests/autolayout_test_main.cpp`:

```cpp
#define CATCH_CONFIG_MAIN
#include "../../../vendor/afterhours/vendor/catch2/catch.hpp"
#include "AutolayoutTest.h"
```

Build and run:
```bash
make autolayout_tests
./output/autolayout_tests
```

**Option 2: Add to existing test runner**

Add to `src/main.cpp`:
```cpp
if (cmdl["--run-unit-tests"]) {
  // Run Catch2 unit tests
  return Catch::Session().run(argc, argv);
}
```

### Test Coverage Goals

**Critical tests to add**:
1. ✅ Standalone sizes never return `-1.0`
2. ✅ Parent-dependent sizes use estimates when parent not ready
3. ✅ Children-dependent sizes compute after children ready
4. ✅ Multi-pass never leaves `-1.0` values
5. ✅ Absolutely positioned elements get sizes
6. ✅ Estimates are reasonable
7. ✅ Constraint solving works
8. ✅ Circular dependencies handled gracefully

### Integration with CI

Add to your build system:
```makefile
test-autolayout: autolayout_tests
	./output/autolayout_tests --success

.PHONY: test-autolayout
```

### Best Practices

1. **Test in isolation**: Each test should be independent
2. **Test edge cases**: Empty children, no parent, circular dependencies
3. **Test estimates**: Ensure they're reasonable, not just "not -1.0"
4. **Test the fix**: Specifically test that `-1.0` values don't appear
5. **Test performance**: Ensure multi-pass doesn't take too long
6. **Test correctness**: After fixing, values should be correct, not just valid

### Example: Testing the Fix

**Before fix test** (should fail):
```cpp
TEST_CASE("Parent-dependent returns -1.0 when parent not ready", "[Autolayout][Bug]") {
  // This test documents the bug
  // Should fail before fix, pass after fix
  UIComponent child(1);
  child.parent = 999; // Non-existent parent
  child.desired[Axis::X] = percent(0.5f);
  
  AutoLayout al;
  al.calculate_those_with_parents(child);
  
  // Before fix: would be -1.0
  // After fix: should be estimate
  REQUIRE(child.computed[Axis::X] != -1.0f);
}
```

**After fix test** (should pass):
```cpp
TEST_CASE("Parent-dependent uses estimate when parent not ready", "[Autolayout][Fixed]") {
  // This test verifies the fix
  UIComponent child(1);
  child.parent = 999; // Non-existent parent
  child.desired[Axis::X] = percent(0.5f);
  
  window_manager::Resolution resolution{1280, 720};
  AutoLayout al(resolution, {});
  al.calculate_those_with_parents(child);
  
  // Should have estimate, not -1.0
  REQUIRE(child.computed[Axis::X] >= 0.0f);
  REQUIRE(child.computed[Axis::X] != -1.0f);
  REQUIRE(child.computed[Axis::X] > 0.0f); // Should be reasonable
}
```

### References

- [Catch2 Documentation](https://github.com/catchorg/Catch2) - Your test framework
- Existing tests: `vendor/afterhours/example/tests/main.cpp`
- Your integration tests: `src/testing/tests/`

