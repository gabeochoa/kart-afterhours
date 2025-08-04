
#pragma once

#include "rl.h"

#include "config.h"
#include "preload.h"

using namespace afterhours;

struct IntroScreens
    : System<window_manager::ProvidesCurrentResolution, ui::FontManager> {

  enum struct State {
    None,
    Us,
    Raylib,
    Delay,
    Complete,
  } state = State::None;

  // Animation constants
  static constexpr float INITIAL_DELAY = 0.15f;
  static constexpr float US_ANIMATION_DURATION = 0.80f;
  static constexpr float RAYLIB_ANIMATION_DURATION = 0.90f;
  static constexpr float DELAY_DURATION = 0.5f;
  static constexpr float COMPLETION_DELAY = 0.2f;
  static constexpr float BOX_LINE_THICKNESS = 5.f;
  static constexpr float FONT_SIZE_DIVISOR = 15.f;
  static constexpr float TITLE_FONT_SIZE_DIVISOR = 3.5f;

  // Text constants
  static constexpr const char *POWERED_BY_TEXT = "POWERED BY";
  static constexpr const char *RAYLIB_TEXT = "raylib";
  static constexpr const char *TITLE_TEXT = "Cart Chaos";

  window_manager::Resolution resolution;
  float timeInState = 0.f;

  IntroScreens() {}

  virtual bool should_run(float dt) override {
    timeInState += dt;
    return state != State::Complete || timeInState < COMPLETION_DELAY;
  }

  raylib::Color get_white_alpha(float start = 0.f, float length = 1.f) {
    int alpha = (int)std::min(
        255.f, std::lerp(0.f, 255.f, ((timeInState - start) / length)));
    return {255, 255, 255, (unsigned char)alpha};
  }

  float get_animation_progress(float start_time, float duration) {
    return std::max(0.f, std::min(duration, (timeInState - start_time))) /
           duration;
  }

  bool is_animation_complete(float start_time, float duration) {
    return timeInState > (start_time + duration);
  }

  State render_raylib(ui::FontManager &fm) {
    raylib::Font raylibFont = fm.get_font(get_font_name(FontID::raylibFont));
    int font_size = (int)(resolution.height / FONT_SIZE_DIVISOR);
    float font_size_f = static_cast<float>(font_size);

    vec2 start_position{(float)resolution.width * 0.4f, font_size_f * 4.f};
    vec2 box_top_left = start_position + vec2{0.f, font_size_f * 1.5f};

    float fade_start_time = RAYLIB_ANIMATION_DURATION * 4.0f;
    float fade_duration = RAYLIB_ANIMATION_DURATION * 0.8f;

    render_powered_by_text(raylibFont, start_position, font_size_f,
                           fade_start_time, fade_duration);
    render_animation_box(box_top_left, font_size_f, fade_start_time,
                         fade_duration);
    render_raylib_text(raylibFont, box_top_left, font_size_f, fade_start_time,
                       fade_duration);

    if (timeInState > (RAYLIB_ANIMATION_DURATION * 4.5f)) {
      return State::Delay;
    }
    return State::Raylib;
  }

  void render_powered_by_text(raylib::Font &font, vec2 position,
                              float font_size, float fade_start_time = 0.f,
                              float fade_duration = 0.f) {
    raylib::Color color = get_white_alpha(0.f, RAYLIB_ANIMATION_DURATION);

    if (fade_duration > 0.f && timeInState > fade_start_time) {
      float fade_progress =
          get_animation_progress(fade_start_time, fade_duration);
      color.a = (unsigned char)(color.a * (1.f - fade_progress));
    }

    raylib::DrawTextEx(font, POWERED_BY_TEXT,
                       position - vec2{font_size / 4.f, 0}, font_size, 1.f,
                       color);
  }

  void render_animation_box(vec2 box_top_left, float font_size,
                            float fade_start_time = 0.f,
                            float fade_duration = 0.f) {
    float powered_width =
        (float)raylib::MeasureText(POWERED_BY_TEXT, (int)font_size);
    float width = powered_width * 0.80f;

    render_box_lines(box_top_left, width, fade_start_time, fade_duration);
  }

  void render_box_lines(vec2 box_top_left, float width,
                        float fade_start_time = 0.f,
                        float fade_duration = 0.f) {
    if (timeInState > RAYLIB_ANIMATION_DURATION) {
      float pct_complete = get_animation_progress(RAYLIB_ANIMATION_DURATION,
                                                  RAYLIB_ANIMATION_DURATION);

      raylib::Color line_color =
          get_white_alpha(RAYLIB_ANIMATION_DURATION, RAYLIB_ANIMATION_DURATION);
      if (fade_duration > 0.f && timeInState > fade_start_time) {
        float fade_progress =
            get_animation_progress(fade_start_time, fade_duration);
        line_color.a = (unsigned char)(line_color.a * (1.f - fade_progress));
      }

      // Top and left lines
      raylib::DrawLineEx(box_top_left,
                         box_top_left + vec2{width * pct_complete, 0},
                         BOX_LINE_THICKNESS, line_color);
      raylib::DrawLineEx(box_top_left,
                         box_top_left + vec2{0, width * pct_complete},
                         BOX_LINE_THICKNESS, line_color);
    }

    if (timeInState > (RAYLIB_ANIMATION_DURATION * 2.f)) {
      float pct_complete = get_animation_progress(
          RAYLIB_ANIMATION_DURATION * 2.f, RAYLIB_ANIMATION_DURATION);

      raylib::Color line_color =
          get_white_alpha(RAYLIB_ANIMATION_DURATION, RAYLIB_ANIMATION_DURATION);
      if (fade_duration > 0.f && timeInState > fade_start_time) {
        float fade_progress =
            get_animation_progress(fade_start_time, fade_duration);
        line_color.a = (unsigned char)(line_color.a * (1.f - fade_progress));
      }

      // Right and bottom lines
      raylib::DrawLineEx(box_top_left + vec2{width, 0},
                         box_top_left + vec2{width, 0} +
                             vec2{0, width * pct_complete},
                         BOX_LINE_THICKNESS, line_color);
      raylib::DrawLineEx(box_top_left + vec2{0, width},
                         box_top_left + vec2{0, width} +
                             vec2{width * pct_complete, 0.f},
                         BOX_LINE_THICKNESS, line_color);
    }
  }

  void render_raylib_text(raylib::Font &font, vec2 box_top_left,
                          float font_size, float fade_start_time = 0.f,
                          float fade_duration = 0.f) {
    float powered_width =
        (float)raylib::MeasureText(POWERED_BY_TEXT, (int)font_size);
    float width = powered_width * 0.80f;
    vec2 box_bottom_right = box_top_left + vec2{width, width};

    if (timeInState > (RAYLIB_ANIMATION_DURATION * 3.f)) {
      float raylib_width =
          (float)raylib::MeasureText(RAYLIB_TEXT, (int)font_size);

      raylib::Color text_color = get_white_alpha(
          (RAYLIB_ANIMATION_DURATION * 3.f), RAYLIB_ANIMATION_DURATION * 3.f);
      if (fade_duration > 0.f && timeInState > fade_start_time) {
        float fade_progress =
            get_animation_progress(fade_start_time, fade_duration);
        text_color.a = (unsigned char)(text_color.a * (1.f - fade_progress));
      }

      raylib::DrawTextEx(font, RAYLIB_TEXT,
                         box_bottom_right - vec2{raylib_width, font_size},
                         font_size, 1.f, text_color);
    }
  }

  State render_us(ui::FontManager &fm) {
    raylib::Font font = fm.get_font(get_font_name(FontID::EQPro));

    float font_size = ((float)resolution.height / TITLE_FONT_SIZE_DIVISOR);
    float width = (float)raylib::MeasureText(TITLE_TEXT, (int)font_size);

    vec2 position = {((float)resolution.width - width) * 2.f,
                     ((float)resolution.height / 2.f) - (font_size / 2.f)};

    float fade_start_time = US_ANIMATION_DURATION * 1.5f;
    float fade_duration = US_ANIMATION_DURATION * 0.5f;

    unsigned char alpha =
        calculate_title_alpha_with_fade(fade_start_time, fade_duration);
    raylib::DrawTextEx(font, TITLE_TEXT, position, font_size, 1.f,
                       {255, 255, 255, alpha});

    if (timeInState > (US_ANIMATION_DURATION * 2.0f)) {
      return State::Raylib;
    }
    return State::Us;
  }

  unsigned char calculate_title_alpha() {
    if (timeInState < (US_ANIMATION_DURATION * 1.0f)) {
      return 255;
    } else if (timeInState < (US_ANIMATION_DURATION * 2.0f)) {
      return 255 - (unsigned char)(255 * (timeInState /
                                          (US_ANIMATION_DURATION * 2.f)));
    } else {
      return 0;
    }
  }

  unsigned char calculate_title_alpha_with_fade(float fade_start_time,
                                                float fade_duration) {
    unsigned char base_alpha = calculate_title_alpha();

    if (fade_duration > 0.f && timeInState > fade_start_time) {
      float fade_progress =
          get_animation_progress(fade_start_time, fade_duration);
      return (unsigned char)(base_alpha * (1.f - fade_progress));
    }

    return base_alpha;
  }

  virtual void
  for_each_with(Entity &,
                window_manager::ProvidesCurrentResolution &pCurrentResolution,
                ui::FontManager &fm, float) override {

    raylib::ClearBackground(raylib::BLACK);
    resolution = pCurrentResolution.current_resolution;

    // Check for any key or mouse click to skip intro
    if (raylib::GetKeyPressed() != 0 ||
        raylib::IsMouseButtonPressed(raylib::MOUSE_LEFT_BUTTON)) {
      state = State::Complete;
      running = false;
      return;
    }

    auto previous_state = state;
    state = determine_next_state(fm);

    if (previous_state != state) {
      timeInState = 0.f;
    }
  }

  State determine_next_state(ui::FontManager &fm) {
    switch (state) {
    case State::None:
      return timeInState < INITIAL_DELAY ? State::None : State::Us;
    case State::Us:
      return render_us(fm);
    case State::Raylib:
      return render_raylib(fm);
    case State::Delay:
      return timeInState > DELAY_DURATION ? State::Complete : State::Delay;
    case State::Complete:
      running = false;
      return State::Complete;
    default:
      return State::None;
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

  // turn back on running for the next screen
  running = true;
}
