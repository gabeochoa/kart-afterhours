# Afterhours Engine Upgrade Review

**Date:** 2026-02-25
**From:** `8c36067` (current)
**To:** `51ad3d9` (latest)
**Commits:** ~130

---

## High Impact — New UI Primitives

The game currently builds all layouts with raw `div()` calls. These new primitives would significantly simplify `ui_systems.cpp`.

### `hstack()` / `vstack()`
Horizontal and vertical layout containers with sensible defaults (`percent(1.0) x children()`). Replaces the pattern of `div().with_size(...)` + manual flex direction.

```cpp
// before
imm::div(ctx, "row", [&](auto &e) {
    e.get<UIComponent>().set_size(percent(1.0), children());
    e.get<UIComponent>().set_flex_direction(FlexDirection::Row);
});

// after
imm::hstack(ctx, "row", [&](auto &e) { ... });
```

### `spacer()`
Fills available space. Replaces empty divs used for pushing elements apart.

### `tray()`
Single-tab-stop container for arrow-key navigation within a group. Main menu button lists, settings rows, etc. would benefit. Dropdown and radio_group already use it internally.

### `stepper()`
Built-in stepper with neighboring item display via `num_visible` parameter. The round settings screens likely reinvent this for value selection.

### `toggle_button` primitive
New `imm::primitive::toggle_button` — toggle_switch was refactored to use it internally.

---

## High Impact — Layout Engine

### `Dim::Expand`
Flex-grow equivalent. Fills remaining available space without hardcoding sizes. Huge for responsive layouts.

### Flex `gap`
CSS `gap` equivalent — spacing between children without manual spacer elements or margin hacks.

### Min/Max constraints
`min_size` and `max_size` now properly cap or floor computed sizes.

### Percent sizing fix
Children now correctly use parent's content area (after padding). Wrap height correctly reflects all wrapped rows.

---

## High Impact — Styling and Theming

### Button variants
`ButtonVariant::Filled`, `ButtonVariant::Outline`, `ButtonVariant::Ghost` + icon+text button support. Could differentiate primary actions (Play) from secondary (Settings, About).

### Theme refactor (`theme.h`)
Better defaults, more customization. The game already sets up a synthwave theme — this gives more control.

### Decorators
Bracket-style and other decorators for UI components. Visual flourishes without manual drawing.

### Per-component hover backgrounds
`with_hover_background()` on any element.

### Per-side borders
`with_border_left()`, `with_border_top()`, etc. instead of all-or-nothing borders.

### Cursor on hover
Automatic cursor change when hovering interactive elements.

### Letter spacing
`with_letter_spacing()` for text rendering — useful for titles and headers.

### Default transparent backgrounds
All elements now default to transparent instead of needing explicit `with_transparent_bg()`.

---

## Medium Impact — Animation

### Declarative animation system
Rotation and scale animations for UI components, with text rotation support in batched renderer.

```cpp
// Rotate, scale, or combine animations declaratively
entity.get<UIComponent>().with_rotation(angle);
entity.get<UIComponent>().with_scale(scale);
```

Could add juice to menu transitions, button hover effects, title screen, etc.

---

## Medium Impact — Input

### KeyChord system
Support for key combinations (e.g., Ctrl+A, Shift+Tab). Already used for text_input select-all.

### `pressed_exact` input method
Distinguishes between "A pressed" and "Shift+A pressed". Prevents modifier combos from triggering base key actions.

### Input exclusivity
Clicking anywhere outside a dropdown closes it. Previously had to click a specific close target.

### `consume_press` fix
Fix for key delivered to wrong action when multiple actions share a key.

### Automatic gamepad mapping loading
Controllers work out of the box via `try_load_gamepad_mappings()`.

### Clipboard support
Text input now supports copy/paste/select-all (Ctrl/Cmd+C/V/A).

---

## Medium Impact — Rendering

### Sokol/Metal backend
Full Metal backend via Sokol, including render textures, rounded rects, better text rendering. Not immediately relevant if staying on raylib, but good for future portability.

### Render texture helpers
`add_render_texture()`, `begin_render_texture()`, `end_render_texture()` across all backends.

### In-memory screenshot capture
All backends can capture screenshots to memory (not just files).

### 3D drawing helpers
Backend-agnostic 3D drawing with raylib, sokol, and none backends. Could be useful if the game ever wants 3D elements.

---

## Low Impact — Engine Internals

### ECS performance
- `should_iterate` override and `constexpr tags_ok` optimization
- Component mapping cached once per frame instead of per system
- Backward-compatible query refactor

### Entity ID fix
`ENTITY_ID_GEN` changed from `static` to `inline` for globally unique IDs across translation units.

### SINGLETON_FWD fix
Changed from `static` to `inline` for shared_ptr — fixes duplicate singleton instances.

### UI entities separated
UI entities now live in a dedicated `EntityCollection`, separate from game entities.

### Render systems no longer const
Render system `for_each_with` methods can now mutate state.

### Render layer inheritance
Children automatically inherit `render_layer` from parent.

### Auto-derived debug names
Debug name auto-derived from label text when not explicitly set.

---

## Breaking Changes

These may require migration in existing code:

| Old API | New API | Impact |
|---------|---------|--------|
| `scroll_view()` | `div().with_overflow()` | Check if used in UI |
| `drag_group()` | `div().with_draggable_children()` | Check if used in UI |
| `checkbox_no_label` | Use `spacer()` | Config references this |
| `ToggleSwitchStyle::Circle` | Removed | Check if used |

---

## Convenience APIs Added

Small helpers that reduce boilerplate:

- `with_720p_size(w, h)` — resolution-aware sizing
- `with_transparent_bg()` — explicit transparent background
- `with_absolute_position(x, y)` — position overloads
- `Margin::all(v)`, `Padding::horizontal(v)` — static factory methods
- `sprite()` overload defaulting to full-texture source rect
- `with_font_size(FontSize)` — separate font size from font family
- `FontSize` alias type
- `get_final_rect()` — get computed rect after layout

---

## Recommended Adoption Order

1. **Fix breaking changes** — migrate any uses of old APIs
2. **Adopt `hstack`/`vstack`/`spacer`** — immediate code simplification
3. **Add `gap`** — remove manual spacing divs
4. **Use `Dim::Expand`** — replace hardcoded fill sizes
5. **Try `tray`** — improve keyboard nav in menus
6. **Use `stepper`** — simplify settings value selectors
7. **Button variants** — visual hierarchy in menus
8. **Animations** — polish and juice
9. **Gamepad auto-mapping** — controller support improvement
