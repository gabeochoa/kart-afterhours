
#pragma once

#include "rl.h"

using namespace afterhours;

struct IntroScreens : System<window_manager::ProvidesCurrentResolution> {

  enum struct State {
    None,
    Us,
    Raylib,
    Complete,
  } state = State::None;

  window_manager::Resolution resolution;

  raylib::Font raylibFont;
  float timeInState = 0.f;

  IntroScreens() { this->raylibFont = raylib::GetFontDefault(); }

  virtual bool should_run(float dt) {
    timeInState += dt;
    if (state == State::Complete) {
      return timeInState < 0.2f;
    }
    return true;
  }

  raylib::Color get_white_alpha(float start = 0.f, float length = 1.f) {
    int alpha = (int)std::min(
        255.f, std::lerp(0.f, 255.f, ((timeInState - start) / length)));
    return {255, 255, 255, (unsigned char)alpha};
  }

  State render_raylib() {

    float anim_duration = 1.20f;

    int font_size = (int)(resolution.height / 15);

    vec2 start_position{resolution.width * 0.4f, font_size * 4.f};
    float font_size_f = static_cast<float>(font_size);

    float thicc = 5.f;
    vec2 box_top_left = start_position + vec2{0, font_size_f * 1.5f};

    std::string powered_by_str = "POWERED BY";
    float powered_width =
        (float)raylib::MeasureText(powered_by_str.c_str(), font_size);
    raylib::DrawTextEx(this->raylibFont, powered_by_str.c_str(),
                       start_position - vec2{font_size_f / 4.f, 0}, font_size,
                       1.f, get_white_alpha(0.f, anim_duration));

    float width = powered_width * 0.80f;
    // first two lines
    if (timeInState > anim_duration) {
      float pct_complete =
          std::max(0.f,
                   std::min(anim_duration, (timeInState - anim_duration))) /
          anim_duration;
      // top line
      raylib::DrawLineEx(box_top_left,
                         box_top_left + vec2{width * pct_complete, 0}, thicc,
                         get_white_alpha(anim_duration, anim_duration));
      // left line
      raylib::DrawLineEx(box_top_left,
                         box_top_left + vec2{0, width * pct_complete}, thicc,
                         get_white_alpha(anim_duration, anim_duration));
    }

    if (timeInState > (anim_duration * 2.f)) {
      float pct_complete =
          std::max(0.f, std::min(anim_duration,
                                 (timeInState - (anim_duration * 2.f)))) /
          anim_duration;
      // right line
      raylib::DrawLineEx(box_top_left + vec2{width, 0},
                         box_top_left + vec2{width, 0} +
                             vec2{0, width * pct_complete},
                         thicc, get_white_alpha(anim_duration, anim_duration));
      // bottom line
      raylib::DrawLineEx(box_top_left + vec2{0, width},
                         box_top_left + vec2{0, width} +
                             vec2{width * pct_complete, 0.f},
                         thicc, get_white_alpha(anim_duration, anim_duration));
    }

    vec2 box_bottom_right = box_top_left + vec2{width, width};
    if (timeInState > (anim_duration * 3.f)) {
      std::string raylib_str = "raylib";
      float raylib_width =
          (float)raylib::MeasureText(raylib_str.c_str(), font_size);

      raylib::DrawTextEx(
          this->raylibFont, raylib_str.c_str(),
          box_bottom_right - vec2{raylib_width, font_size_f}, font_size, 1.f,
          get_white_alpha((anim_duration * 3.f), anim_duration * 3.f));
    }

    if (timeInState > (anim_duration * 4.5f)) {
      return State::Complete;
    }

    return State::Raylib;
  }

  virtual void
  for_each_with(Entity &,
                window_manager::ProvidesCurrentResolution &pCurrentResolution,
                float dt) override {

    resolution = pCurrentResolution.current_resolution;

    auto before = state;
    switch (before) {
    case State::None: {
      state = timeInState < 0.15f ? State::None : State::Us;
    } break;
    case State::Us: {
      // TODO
      state = State::Raylib;
    } break;
    case State::Raylib: {
      state = render_raylib();
    } break;
    case State::Complete: {
      running = false;
    } break;
    }

    if (before != state) {
      timeInState = 0.f;
    }
  }
};

void intro() {
  SystemManager systems;

  window_manager::register_update_systems(systems);
  systems.register_update_system(std::make_unique<IntroScreens>());

  while (running && !raylib::WindowShouldClose()) {
    raylib::BeginDrawing();
    { systems.run(raylib::GetFrameTime()); }
    raylib::EndDrawing();
  }
  running = true;
}
