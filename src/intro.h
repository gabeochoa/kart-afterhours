
#pragma once

#include "rl.h"

#include "components.h"
#include "config.h"
#include "preload.h"
#include "shader_library.h"
#include "sound_library.h"

using namespace afterhours;

struct IntroScreens
    : System<window_manager::ProvidesCurrentResolution, ui::FontManager> {

  enum struct State {
    None,
    Chase,
    Raylib,
    Delay,
    Complete,
  } state = State::None;

  // Animation timing constants
  static constexpr float INITIAL_DELAY = 1.0f;
  static constexpr float CHASE_STATE_FULL_TIME = 20.0f;
  static constexpr float RAYLIB_ANIMATION_DURATION = 0.90f;
  static constexpr float DELAY_DURATION = 0.5f;
  static constexpr float COMPLETION_DELAY = 0.2f;
  static constexpr float PASSBY_FADE_TOTAL =
      RAYLIB_ANIMATION_DURATION * 4.5f + DELAY_DURATION;
  static constexpr float PASSBY_SKIP_FADE_TOTAL = 0.15f;

  // Car animation configuration
  static constexpr float CHASE_CAR_SIZE = 120.f;
  static constexpr float CHASE_SPEED = 800.f;
  static constexpr float CHASE_SINE_AMPLITUDE = 60.f;
  static constexpr float CHASE_SINE_FREQUENCY = 2.0f;
  static constexpr float CHASE_CAR_SPACING = 0.6f;
  static constexpr float CHASE_CAR_START_DELAY = 0.2f;

  // Text timing configuration
  static constexpr float TEXT_FADE_IN_DURATION_DIVISOR = 20.f;
  static constexpr float TEXT_FADE_OUT_START_DIVISOR = 9.f;
  static constexpr float TEXT_FADE_OUT_DURATION_DIVISOR = 10.f;

  // UI constants
  static constexpr float BOX_LINE_THICKNESS = 5.f;
  static constexpr float FONT_SIZE_DIVISOR = 15.f;

  // Text constants
  static constexpr const char *POWERED_BY_TEXT = "POWERED BY";
  static constexpr const char *RAYLIB_TEXT = "raylib";
  static constexpr const char *TITLE_TEXT = "kart chaos";

  window_manager::Resolution resolution;
  float timeInState = 0.f;
  bool passbyPlayed[3] = {false, false, false};
  bool passbyStarted = false;
  bool passbyFadeActive = false;
  float passbyFadeElapsed = 0.f;
  bool skipRequested = false;
  float passbyFadeTotal = PASSBY_FADE_TOTAL;

  void set_passby_volume(float v) {
    auto &s0 = SoundLibrary::get().get("IntroPassBy_0");
    auto &s1 = SoundLibrary::get().get("IntroPassBy_1");
    auto &s2 = SoundLibrary::get().get("IntroPassBy_2");
    raylib::SetSoundVolume(s0, v);
    raylib::SetSoundVolume(s1, v);
    raylib::SetSoundVolume(s2, v);
  }

  void stop_passby() {
    auto &s0 = SoundLibrary::get().get("IntroPassBy_0");
    auto &s1 = SoundLibrary::get().get("IntroPassBy_1");
    auto &s2 = SoundLibrary::get().get("IntroPassBy_2");
    raylib::StopSound(s0);
    raylib::StopSound(s1);
    raylib::StopSound(s2);
  }

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

  void render_title_text(float alpha = 255.f) {
    raylib::Font title_font =
        EntityHelper::get_singleton_cmp<ui::FontManager>()->get_font(
            get_font_name(FontID::EQPro));
    float title_font_size = ((float)resolution.height / 3.f);
    float title_width =
        (float)raylib::MeasureText(TITLE_TEXT, (int)title_font_size);

    vec2 title_position = {(float)resolution.width / 2.f - title_width / 2.5f,
                           (float)resolution.height / 2.f -
                               title_font_size / 2.f};

    raylib::DrawTextEx(title_font, TITLE_TEXT, title_position, title_font_size,
                       1.f, {255, 255, 255, (unsigned char)alpha});
  }

  vec2 calculate_car_position(float car_offset, int car_index) {
    float total_distance = (float)resolution.width + CHASE_CAR_SIZE;
    float x_pos = car_offset * total_distance - CHASE_CAR_SIZE;

    // Add randomization to Y position
    float base_y = (float)resolution.height / 2.f;
    float sine_offset =
        std::sin(car_offset * CHASE_SINE_FREQUENCY * 2.f * M_PI) *
        CHASE_SINE_AMPLITUDE;

    if (car_index == 1) {
      sine_offset = 0.f;
    }

    if (car_index == 2) {
      sine_offset = -(sine_offset / 4.f);
    }

    float random_offset =
        std::sin((car_offset + car_index * 1.5f) * 3.14f) * 20.f;
    // 0.5f = phase offset per car, 3.14f = frequency, 20.f = amplitude
    float y_pos = base_y + sine_offset + random_offset;

    return {x_pos, y_pos};
  }

  State render_chase() {
    // Get the spritesheet texture
    auto *spritesheet_component = EntityHelper::get_singleton_cmp<
        afterhours::texture_manager::HasSpritesheet>();
    if (!spritesheet_component) {
      return State::Raylib;
    }

    raylib::Texture2D sheet = spritesheet_component->texture;
    Rectangle source_frame =
        afterhours::texture_manager::idx_to_sprite_frame(0, 1);

    // Calculate car movement using CHASE_SPEED
    float car_distance = (timeInState - CHASE_CAR_START_DELAY) * CHASE_SPEED;
    float total_distance = (float)resolution.width + CHASE_CAR_SIZE;
    float car_progress = car_distance / total_distance;

    // Calculate text fade progress
    float text_fade_in_duration =
        CHASE_STATE_FULL_TIME / TEXT_FADE_IN_DURATION_DIVISOR;
    float text_fade_out_start =
        CHASE_STATE_FULL_TIME / TEXT_FADE_OUT_START_DIVISOR;
    float text_fade_out_duration =
        CHASE_STATE_FULL_TIME / TEXT_FADE_OUT_DURATION_DIVISOR;

    float text_alpha = 255.f;
    if (timeInState < text_fade_in_duration) {
      // Fade in
      text_alpha = (timeInState / text_fade_in_duration) * 255.f;
    } else if (timeInState > text_fade_out_start) {
      // Fade out
      float fade_out_progress =
          (timeInState - text_fade_out_start) / text_fade_out_duration;
      text_alpha = (1.f - std::min(1.f, fade_out_progress)) * 255.f;
    }

    // Create mask texture with text
    static raylib::RenderTexture2D textMaskTexture;
    static bool maskTextureCreated = false;
    if (!maskTextureCreated) {
      textMaskTexture =
          raylib::LoadRenderTexture(resolution.width, resolution.height);
      maskTextureCreated = true;
    }

    // Render text to mask texture
    raylib::BeginTextureMode(textMaskTexture);
    raylib::ClearBackground({0, 0, 0, 0});
    render_title_text(text_alpha);
    raylib::EndTextureMode();

    // Render text to screen
    render_title_text(text_alpha);

    // Render cars to separate texture with masking
    if (car_progress > 0.f) {
      static raylib::RenderTexture2D carTexture;
      static bool carTextureCreated = false;
      if (!carTextureCreated) {
        carTexture =
            raylib::LoadRenderTexture(resolution.width, resolution.height);
        carTextureCreated = true;
      }

      raylib::BeginTextureMode(carTexture);
      raylib::ClearBackground({0, 0, 0, 0});

      raylib::Color car_colors[] = {raylib::RED, raylib::BLUE, raylib::GREEN};

      for (int i = 0; i < 3; i++) {
        float car_offset = car_progress - (i * CHASE_CAR_SPACING);
        if (car_offset < 0.f || car_offset > 1.f)
          continue;

        vec2 car_pos = calculate_car_position(car_offset, i);
        vec2 next_car_pos = calculate_car_position(car_offset + 0.01f, i);
        float angle =
            std::atan2(next_car_pos.y - car_pos.y, next_car_pos.x - car_pos.x) *
                180.f / M_PI +
            90.f;

        if (!passbyPlayed[i] && car_pos.x >= (float)resolution.width * 0.1f) {
          auto enqueue = [](const char *name) {
            auto opt = EntityQuery({.force_merge = true})
                           .whereHasComponent<SoundEmitter>()
                           .gen_first();
            if (opt.valid()) {
              auto &ent = opt.asE();
              auto &req = ent.addComponentIfMissing<PlaySoundRequest>();
              req.policy = PlaySoundRequest::Policy::Name;
              req.name = name;
              req.prefer_alias =
                  false; // these pass-bys are long; no aliasing needed
            }
          };
          switch (i) {
          case 0:
            enqueue("IntroPassBy_0");
            break;
          case 1:
            enqueue("IntroPassBy_1");
            break;
          case 2:
            enqueue("IntroPassBy_2");
            break;
          }
          passbyPlayed[i] = true;
        }

        raylib::DrawTexturePro(
            sheet, source_frame,
            Rectangle{car_pos.x, car_pos.y, CHASE_CAR_SIZE, CHASE_CAR_SIZE},
            vec2{CHASE_CAR_SIZE / 2.f, CHASE_CAR_SIZE / 2.f}, angle,
            car_colors[i]);
      }

      raylib::EndTextureMode();

      // Apply mask shader
      raylib::Shader maskShader =
          ShaderLibrary::get().get(ShaderType::text_mask);
      int maskTextureLoc = raylib::GetShaderLocation(maskShader, "maskTexture");
      int timeLoc = raylib::GetShaderLocation(maskShader, "time");
      int resolutionLoc = raylib::GetShaderLocation(maskShader, "resolution");

      float timeValue = static_cast<float>(raylib::GetTime());
      vec2 resolutionValue = {static_cast<float>(resolution.width),
                              static_cast<float>(resolution.height)};

      if (timeLoc >= 0) {
        raylib::SetShaderValue(maskShader, timeLoc, &timeValue,
                               raylib::SHADER_UNIFORM_FLOAT);
      }
      if (resolutionLoc >= 0) {
        raylib::SetShaderValue(maskShader, resolutionLoc, &resolutionValue,
                               raylib::SHADER_UNIFORM_VEC2);
      }

      raylib::BeginShaderMode(maskShader);
      raylib::SetShaderValueTexture(maskShader, maskTextureLoc,
                                    textMaskTexture.texture);

      raylib::DrawTextureRec(
          carTexture.texture,
          Rectangle{0, 0, (float)resolution.width, -(float)resolution.height},
          vec2{0, 0}, raylib::WHITE);

      raylib::EndShaderMode();
    }

    // End chase state when the 3rd car (green car) has completed its journey
    float third_car_progress = car_progress - (2 * CHASE_CAR_SPACING);
    if (third_car_progress >= 1.0f) {
      passbyPlayed[0] = passbyPlayed[1] = passbyPlayed[2] = false;
      passbyStarted = false;
      return State::Raylib;
    }
    return State::Chase;
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

  virtual void
  for_each_with(Entity &,
                window_manager::ProvidesCurrentResolution &pCurrentResolution,
                ui::FontManager &fm, float dt) override {

    raylib::ClearBackground(raylib::BLACK);
    resolution = pCurrentResolution.current_resolution;

    // Check for any key or mouse click to skip intro
    if (raylib::GetKeyPressed() != 0 ||
        raylib::IsMouseButtonPressed(raylib::MOUSE_LEFT_BUTTON)) {
      skipRequested = true;
      if (!passbyFadeActive) {
        passbyFadeActive = true;
        passbyFadeElapsed = 0.f;
      }
      passbyFadeTotal = PASSBY_SKIP_FADE_TOTAL;
    }

    auto previous_state = state;
    state = determine_next_state(fm);

    if (previous_state != state) {
      timeInState = 0.f;
    }

    if (!passbyStarted && previous_state != State::Chase &&
        state == State::Chase) {
      set_passby_volume(1.f);
      auto opt = EntityQuery({.force_merge = true})
                     .whereHasComponent<SoundEmitter>()
                     .gen_first();
      if (opt.valid()) {
        auto &ent = opt.asE();
        auto &r0 = ent.addComponentIfMissing<PlaySoundRequest>();
        r0.policy = PlaySoundRequest::Policy::Name;
        r0.name = "IntroPassBy_0";
        r0.prefer_alias = false;
        auto &r1 = ent.addComponentIfMissing<PlaySoundRequest>();
        r1.policy = PlaySoundRequest::Policy::Name;
        r1.name = "IntroPassBy_1";
        r1.prefer_alias = false;
        auto &r2 = ent.addComponentIfMissing<PlaySoundRequest>();
        r2.policy = PlaySoundRequest::Policy::Name;
        r2.name = "IntroPassBy_2";
        r2.prefer_alias = false;
      }
      passbyStarted = true;
    }

    if (!passbyFadeActive && previous_state != State::Raylib &&
        state == State::Raylib) {
      passbyFadeActive = true;
      passbyFadeElapsed = 0.f;
    }

    if (passbyFadeActive) {
      passbyFadeElapsed += dt;
      float t = std::min(1.f, passbyFadeElapsed / passbyFadeTotal);
      set_passby_volume(1.f - t);
    }

    if (skipRequested && passbyFadeActive &&
        passbyFadeElapsed >= passbyFadeTotal) {
      set_passby_volume(0.f);
      stop_passby();
      passbyFadeActive = false;
      running = false;
      state = State::Complete;
      return;
    }

    if (state == State::Complete) {
      set_passby_volume(0.f);
      stop_passby();
      passbyFadeActive = false;
      skipRequested = false;
      passbyFadeTotal = PASSBY_FADE_TOTAL;
    }
  }

  State determine_next_state(ui::FontManager &fm) {
    switch (state) {
    case State::None:
      return timeInState < INITIAL_DELAY ? State::None : State::Chase;
    case State::Chase:
      return render_chase();
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

inline void intro() {
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
