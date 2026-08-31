#pragma once

#include <afterhours/ah.h>
#include <afterhours/src/graphics.h>
#include <cmath>

#include "../e2e_integration.h"
#include "../game_state_manager.h"

// Memphis-'93 backdrop: a fixed set of geometric shapes drifting and slowly
// spinning behind the menu UI.
//
// Drawn at the *end* of the world pass, over the karts and the arena, and it
// repaints the ground first. The world entities exist whatever screen you are
// on -- karts outlive rounds -- so a menu used to have the last round's kart
// parked in the middle of it. Painting over them is one system; gating the
// eight systems of the world pass individually is eight.
//
// Menu state only. Pause is its own state precisely so the arena stays
// visible under the scrim, which is what the mock asks for.
//
// Determinism: layout comes from a per-shape integer seed (no rand(), no
// GetTime()) and the animation clock is a locally accumulated dt that never
// advances while e2e is enabled, so every screenshot is byte-identical.
struct RenderMenuBackdrop
    : afterhours::System<
          afterhours::window_manager::ProvidesCurrentResolution> {

  static constexpr int SHAPE_COUNT = 18;
  static constexpr raylib::Color PALETTE[4] = {
      {224, 107, 221, 255}, // orchid
      {91, 168, 240, 255},  // sky
      {79, 214, 166, 255},  // mint
      {240, 232, 92, 255},  // butter
  };

  mutable float clock = 0.f;

  // Deterministic hash -> [0, 1). Same value every run, every platform.
  static float seeded(unsigned seed) {
    seed = seed * 1664525u + 1013904223u;
    seed ^= seed >> 16;
    seed *= 2246822519u;
    seed ^= seed >> 13;
    return static_cast<float>(seed & 0xFFFFFFu) / 16777216.f;
  }

  static float wrap01(float v) {
    v = std::fmod(v, 1.f);
    return v < 0.f ? v + 1.f : v;
  }

  static raylib::Vector2 on_circle(raylib::Vector2 center, float radius, float degrees) {
    const float r = degrees * DEG2RAD;
    return raylib::Vector2{center.x + radius * std::cos(r),
                center.y + radius * std::sin(r)};
  }

  void draw_shape(int index, float t, float width, float height) const {
    const unsigned s = static_cast<unsigned>(index) * 9781u + 7u;

    // Everything is a fraction of the current resolution, so it scales.
    const float margin = height * 0.2f;
    const float span_x = width + 2.f * margin;
    const float span_y = height + 2.f * margin;
    const float vx = (seeded(s + 3) - 0.5f) * 0.06f;  // screens per second
    const float vy = (seeded(s + 4) - 0.5f) * 0.06f;
    const raylib::Vector2 center{wrap01(seeded(s + 1) + vx * t) * span_x - margin,
                      wrap01(seeded(s + 2) + vy * t) * span_y - margin};

    const float radius = height * (0.025f + seeded(s + 5) * 0.055f);
    const float angle = seeded(s + 6) * 360.f + (seeded(s + 7) - 0.5f) * 30.f * t;
    const raylib::Color color =
        PALETTE[static_cast<int>(seeded(s + 8) * 4.f) & 3];
    const float thickness = std::max(2.f, height * 0.004f);

    switch (index % 5) {
    case 0: { // filled triangle (clockwise in math coords == CCW on screen)
      raylib::DrawTriangle(on_circle(center, radius, angle),
                           on_circle(center, radius, angle - 120.f),
                           on_circle(center, radius, angle - 240.f), color);
      break;
    }
    case 1: { // outlined triangle
      const raylib::Vector2 a = on_circle(center, radius, angle);
      const raylib::Vector2 b = on_circle(center, radius, angle - 120.f);
      const raylib::Vector2 c = on_circle(center, radius, angle - 240.f);
      raylib::DrawLineEx(a, b, thickness, color);
      raylib::DrawLineEx(b, c, thickness, color);
      raylib::DrawLineEx(c, a, thickness, color);
      break;
    }
    case 2: { // ring
      raylib::DrawRing(center, radius - thickness, radius, 0.f, 360.f, 48,
                       color);
      break;
    }
    case 3: { // outlined square
      raylib::Vector2 prev = on_circle(center, radius, angle + 270.f);
      for (int k = 0; k < 4; k++) {
        const raylib::Vector2 next = on_circle(center, radius, angle + k * 90.f);
        raylib::DrawLineEx(prev, next, thickness, color);
        prev = next;
      }
      break;
    }
    default: { // small filled dot
      raylib::DrawCircleV(center, radius * 0.3f, color);
      break;
    }
    }
  }

  virtual void for_each_with(
      const afterhours::Entity &,
      const afterhours::window_manager::ProvidesCurrentResolution &resolution,
      float dt) const override {
    if (!GameStateManager::get().is_menu_active())
      return;

    // Frozen at t=0 under e2e so screenshots are reproducible byte-for-byte.
    if (!e2e_integration::is_enabled())
      clock += dt;

    const float width = static_cast<float>(resolution.width());
    const float height = static_cast<float>(resolution.height());
    // Same colour BeginWorldRender clears to; this is that clear again, after
    // the karts and the arena have drawn into the pass.
    raylib::DrawRectangle(0, 0, static_cast<int>(width),
                          static_cast<int>(height),
                          raylib::Color{46, 27, 105, 255});
    for (int i = 0; i < SHAPE_COUNT; i++)
      draw_shape(i, clock, width, height);
  }
};
