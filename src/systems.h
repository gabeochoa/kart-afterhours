#pragma once

#include "car_affectors.h"
#include "components.h"
#include "game_state_manager.h"
#include "input_mapping.h"
#include "makers.h"
#include "map_system.h"
#include "query.h"
#include "round_settings.h"
#include "shader_library.h"
#include "sound_library.h"
#include <fmt/format.h>
#include <afterhours/ah.h>

// Hippo game constants
constexpr float HIPPO_SPAWN_INTERVAL = 0.8f;
constexpr int MAX_HIPPO_ITEMS_ON_SCREEN = 20;

// Sprite rendering constants
constexpr float SPRITE_OFFSET_X = 9.f;
constexpr float SPRITE_OFFSET_Y = 4.f;

template <typename... Components>
struct PausableSystem : afterhours::System<Components...> {
  virtual bool should_run(float) override {
    return !GameStateManager::get().is_paused();
  }
};

#include "systems_roundtypes.h"

struct UpdateSpriteTransform
    : System<Transform, afterhours::texture_manager::HasSprite> {

  virtual void for_each_with(Entity &entity, Transform &transform,
                             afterhours::texture_manager::HasSprite &hasSprite,
                             float) override {
    hasSprite.update_transform(transform.position, transform.size,
                               transform.angle);

    if (entity.has_child_of<HasColor>()) {
      hasSprite.update_color(entity.get_with_child<HasColor>().color());
    }
  }
};

struct UpdateAnimationTransform
    : System<Transform, afterhours::texture_manager::HasAnimation> {

  virtual void
  for_each_with(Entity &, Transform &transform,
                afterhours::texture_manager::HasAnimation &hasAnimation,
                float) override {
    hasAnimation.update_transform(transform.position, transform.size,
                                  transform.angle);
  }
};

// System to add SkipTextureManagerRendering to entities with shaders
struct MarkEntitiesWithShaders : System<HasShader> {
  virtual void for_each_with(Entity &entity, HasShader &, float) override {
    if (!entity.has<SkipTextureManagerRendering>()) {
      entity.addComponent<SkipTextureManagerRendering>();
      log_info("Marked entity {} to skip texture_manager rendering", entity.id);
    }
  }
};

// System to render sprites with per-entity shaders
struct RenderSpritesWithShaders
    : System<Transform, afterhours::texture_manager::HasSprite, HasShader,
             HasColor> {
  virtual void
  for_each_with(const Entity &entity, const Transform &transform,
                const afterhours::texture_manager::HasSprite &hasSprite,
                const HasShader &hasShader, const HasColor &hasColor,
                float) const override {
// TODO shaders have issues on Windows - disable for now
#ifdef _WIN32
    // On Windows, render without shader for now
    auto *spritesheet_component = EntityHelper::get_singleton_cmp<
        afterhours::texture_manager::HasSpritesheet>();
    if (!spritesheet_component) {
      log_warn(
          "No spritesheet component found! Drawing BLUE rectangle as fallback");
      raylib::DrawRectanglePro(
          Rectangle{transform.center().x, transform.center().y,
                    transform.size.x, transform.size.y},
          vec2{transform.size.x / 2.f, transform.size.y / 2.f}, transform.angle,
          raylib::BLUE);
      return;
    }

    raylib::Texture2D sheet = spritesheet_component->texture;
    Rectangle source_frame =
        afterhours::texture_manager::idx_to_sprite_frame(0, 1);

    // Use the sprite's scale for rendering size
    float dest_width = source_frame.width * hasSprite.scale;
    float dest_height = source_frame.height * hasSprite.scale;

    // Fine-tuned offset for perfect sprite alignment
    float offset_x = SPRITE_OFFSET_X;
    float offset_y = SPRITE_OFFSET_Y;

    float rotated_x = offset_x * cosf(transform.angle * M_PI / 180.f) -
                      offset_y * sinf(transform.angle * M_PI / 180.f);
    float rotated_y = offset_x * sinf(transform.angle * M_PI / 180.f) +
                      offset_y * cosf(transform.angle * M_PI / 180.f);

    raylib::DrawTexturePro(
        sheet, source_frame,
        Rectangle{
            transform.position.x + transform.size.x / 2.f + rotated_x,
            transform.position.y + transform.size.y / 2.f + rotated_y,
            dest_width,
            dest_height,
        },
        vec2{dest_width / 2.f, dest_height / 2.f}, transform.angle,
        hasColor.color());
#else
    // Original shader rendering for non-Windows platforms
    if (!ShaderLibrary::get().contains(hasShader.shader_name)) {
      log_warn("Shader not found: {}", hasShader.shader_name);
      return;
    }

    // Get the spritesheet texture from singleton
    auto *spritesheet_component = EntityHelper::get_singleton_cmp<
        afterhours::texture_manager::HasSpritesheet>();
    if (!spritesheet_component) {
      log_warn(
          "No spritesheet component found! Drawing BLUE rectangle as fallback");
      raylib::DrawRectanglePro(
          Rectangle{transform.center().x, transform.center().y,
                    transform.size.x, transform.size.y},
          vec2{transform.size.x / 2.f, transform.size.y / 2.f}, transform.angle,
          raylib::BLUE);
      return;
    }

    raylib::Texture2D sheet = spritesheet_component->texture;
    Rectangle source_frame =
        afterhours::texture_manager::idx_to_sprite_frame(0, 1);

    // Use the sprite's scale for rendering size
    float dest_width = source_frame.width * hasSprite.scale;
    float dest_height = source_frame.height * hasSprite.scale;

    // Apply shader and render
    const auto &shader = ShaderLibrary::get().get(hasShader.shader_name);
    raylib::BeginShaderMode(shader);

    // winnerRainbow toggle: on for car_winner, off otherwise
    int rainbowLoc = raylib::GetShaderLocation(shader, "winnerRainbow");
    if (rainbowLoc != -1) {
      float rainbowOn =
          (hasShader.shader_name == std::string("car_winner")) ? 1.0f : 0.0f;
      raylib::SetShaderValue(shader, rainbowLoc, &rainbowOn,
                             raylib::SHADER_UNIFORM_FLOAT);
    }

    // Pass the entity's color to the shader
    raylib::Color entityColor = hasColor.color();
    float entityColorF[4] = {
        entityColor.r / 255.0f,
        entityColor.g / 255.0f,
        entityColor.b / 255.0f,
        entityColor.a / 255.0f,
    };
    raylib::SetShaderValue(shader,
                           raylib::GetShaderLocation(shader, "entityColor"),
                           entityColorF, raylib::SHADER_UNIFORM_VEC4);

    // Update common uniforms if present
    float shaderTime = static_cast<float>(raylib::GetTime());
    int timeLoc = raylib::GetShaderLocation(shader, "time");
    if (timeLoc != -1) {
      raylib::SetShaderValue(shader, timeLoc, &shaderTime,
                             raylib::SHADER_UNIFORM_FLOAT);
    }
    auto rezCmp = EntityHelper::get_singleton_cmp<
        window_manager::ProvidesCurrentResolution>();
    if (rezCmp) {
      vec2 rez = {static_cast<float>(rezCmp->current_resolution.width),
                  static_cast<float>(rezCmp->current_resolution.height)};
      int rezLoc = raylib::GetShaderLocation(shader, "resolution");
      if (rezLoc != -1) {
        raylib::SetShaderValue(shader, rezLoc, &rez,
                               raylib::SHADER_UNIFORM_VEC2);
      }
    }

    // Provide source frame UV bounds so shaders can clamp sampling
    float uvMin[2] = {source_frame.x / static_cast<float>(sheet.width),
                      source_frame.y / static_cast<float>(sheet.height)};
    float uvMax[2] = {(source_frame.x + source_frame.width) /
                          static_cast<float>(sheet.width),
                      (source_frame.y + source_frame.height) /
                          static_cast<float>(sheet.height)};
    int uvMinLoc = raylib::GetShaderLocation(shader, "uvMin");
    if (uvMinLoc != -1) {
      raylib::SetShaderValue(shader, uvMinLoc, uvMin,
                             raylib::SHADER_UNIFORM_VEC2);
    }
    int uvMaxLoc = raylib::GetShaderLocation(shader, "uvMax");
    if (uvMaxLoc != -1) {
      raylib::SetShaderValue(shader, uvMaxLoc, uvMax,
                             raylib::SHADER_UNIFORM_VEC2);
    }

    // Pass speed percentage uniform for car shader
    if (hasShader.shader_name == "car") {
      float speedPercent = transform.speed() / Config::get().max_speed.data;
      int speedLocation = raylib::GetShaderLocation(shader, "speed");
      if (speedLocation != -1) {
        raylib::SetShaderValue(shader, speedLocation, &speedPercent,
                               raylib::SHADER_UNIFORM_FLOAT);
      }
    }

    // Fine-tuned offset for perfect sprite alignment
    float offset_x = SPRITE_OFFSET_X;
    float offset_y = SPRITE_OFFSET_Y;

    float rotated_x = offset_x * cosf(transform.angle * M_PI / 180.f) -
                      offset_y * sinf(transform.angle * M_PI / 180.f);
    float rotated_y = offset_x * sinf(transform.angle * M_PI / 180.f) +
                      offset_y * cosf(transform.angle * M_PI / 180.f);

    raylib::DrawTexturePro(
        sheet, source_frame,
        Rectangle{
            transform.position.x + transform.size.x / 2.f + rotated_x,
            transform.position.y + transform.size.y / 2.f + rotated_y,
            dest_width,
            dest_height,
        },
        vec2{dest_width / 2.f, dest_height / 2.f}, transform.angle,
        raylib::WHITE);

    raylib::EndShaderMode();
#endif
  }
};

// System to render animations with per-entity shaders
struct RenderAnimationsWithShaders
    : System<Transform, afterhours::texture_manager::HasAnimation, HasShader,
             HonkState> {
  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             const afterhours::texture_manager::HasAnimation &,
                             const HasShader &hasShader, const HonkState &honk,
                             float) const override {
    if (!ShaderLibrary::get().contains(hasShader.shader_name)) {
      log_warn("Shader not found: {}", hasShader.shader_name);
      return;
    }

    // Apply shader
    const auto &shader = ShaderLibrary::get().get(hasShader.shader_name);
    raylib::BeginShaderMode(shader);

    // Render animation entities as SKYBLUE for visual distinction
    raylib::DrawRectanglePro(
        Rectangle{
            transform.center().x,
            transform.center().y,
            transform.size.x,
            transform.size.y,
        },
        vec2{transform.size.x / 2.f, transform.size.y / 2.f}, transform.angle,
        raylib::SKYBLUE);

    // End shader
    raylib::EndShaderMode();
  }
};

struct RenderFPS : System<window_manager::ProvidesCurrentResolution> {
  virtual ~RenderFPS() {}
  virtual void for_each_with(
      const Entity &,
      const window_manager::ProvidesCurrentResolution &pCurrentResolution,
      float) const override {
    raylib::DrawFPS((int)(pCurrentResolution.width() - 80), 0);
  }
};

struct RenderDebugWindowInfo
    : System<window_manager::ProvidesCurrentResolution> {
  input::PossibleInputCollector<InputAction> inpc;

  virtual void once(float) override {
    inpc = input::get_input_collector<InputAction>();
  }

  virtual bool should_run(float) override {
    return true; // @nocommit just for  testing ui scale
    inpc = input::get_input_collector<InputAction>();
    if (!inpc.has_value())
      return false;
    bool toggle_pressed =
        std::ranges::any_of(inpc.inputs_pressed(), [](const auto &a) {
          return a.action == InputAction::ToggleUILayoutDebug;
        });
    static bool enabled = false;
    if (toggle_pressed)
      enabled = !enabled;
    return enabled;
  }

  virtual void for_each_with(
      const Entity &,
      const window_manager::ProvidesCurrentResolution &pCurrentResolution,
      float) const override {
    const int window_w = raylib::GetScreenWidth();
    const int window_h = raylib::GetScreenHeight();
    const auto rez = pCurrentResolution.current_resolution;

    const int x = pCurrentResolution.width() - 160;
    const int y0 = 18;
    const int y1 = 36;
    const int font = 14;
    const raylib::Color col = raylib::WHITE;

    raylib::DrawText(fmt::format("win {}x{}", window_w, window_h).c_str(), x,
                     y0, font, col);
    raylib::DrawText(fmt::format("game {}x{}", rez.width, rez.height).c_str(),
                     x, y1, font, col);
  }
};

struct UpdateRenderTexture : System<> {

  window_manager::Resolution resolution;

  virtual ~UpdateRenderTexture() {}

  void once(float) {
    const window_manager::ProvidesCurrentResolution *pcr =
        EntityHelper::get_singleton_cmp<
            window_manager::ProvidesCurrentResolution>();
    if (pcr->current_resolution != resolution) {
      resolution = pcr->current_resolution;
      raylib::UnloadRenderTexture(mainRT);
      mainRT = raylib::LoadRenderTexture(resolution.width, resolution.height);
      // keep the second render texture in sync
      raylib::UnloadRenderTexture(screenRT);
      screenRT = raylib::LoadRenderTexture(resolution.width, resolution.height);
    }
  }
};

struct LetterboxLayout {
  int bar_left = 0;
  int bar_right = 0;
  int bar_top = 0;
  int bar_bottom = 0;
  raylib::Rectangle dst{0.f, 0.f, 0.f, 0.f};
};

static inline LetterboxLayout
compute_letterbox_layout(const int window_width, const int window_height,
                         const int content_width, const int content_height) {
  LetterboxLayout layout;
  int dest_w = window_width;
  int dest_h = static_cast<int>(
      std::round(static_cast<double>(dest_w) * content_height / content_width));
  if (dest_h > window_height) {
    dest_h = window_height;
    dest_w = static_cast<int>(std::round(static_cast<double>(dest_h) *
                                         content_width / content_height));
  }

  const int bar_w_total = window_width - dest_w;
  const int bar_h_total = window_height - dest_h;
  layout.bar_left = bar_w_total / 2;
  layout.bar_right = bar_w_total - layout.bar_left;
  layout.bar_top = bar_h_total / 2;
  layout.bar_bottom = bar_h_total - layout.bar_top;
  layout.dst = raylib::Rectangle{
      static_cast<float>(layout.bar_left), static_cast<float>(layout.bar_top),
      static_cast<float>(dest_w), static_cast<float>(dest_h)};
  return layout;
}

struct RenderRenderTexture : System<window_manager::ProvidesCurrentResolution> {
  virtual ~RenderRenderTexture() {}
  virtual void for_each_with(
      const Entity &entity,
      const window_manager::ProvidesCurrentResolution &pCurrentResolution,
      float) const override {
    const int window_w = raylib::GetScreenWidth();
    const int window_h = raylib::GetScreenHeight();
    const int content_w = mainRT.texture.width;
    const int content_h = mainRT.texture.height;
    const LetterboxLayout layout =
        compute_letterbox_layout(window_w, window_h, content_w, content_h);

    const raylib::Rectangle src{0.0f, 0.0f, (float)mainRT.texture.width,
                                -(float)mainRT.texture.height};
    raylib::DrawTexturePro(mainRT.texture, src, layout.dst, {0.0f, 0.0f}, 0.0f,
                           raylib::WHITE);
  }
};

struct BeginPostProcessingShader : System<> {
  virtual void once(float) override {
    const bool hasBase = ShaderLibrary::get().contains("post_processing");
    const bool hasTag = ShaderLibrary::get().contains("post_processing_tag");
    auto &rm = RoundManager::get();
    bool useTagShader = false;
    if (rm.active_round_type == RoundType::TagAndGo) {
      auto &settings = rm.get_active_settings();
      useTagShader = (settings.state == RoundSettings::GameState::Countdown);
    }
    const char *name =
        (useTagShader && hasTag) ? "post_processing_tag" : "post_processing";
    if (!ShaderLibrary::get().contains(name)) {
      return;
    }
    const auto &shader = ShaderLibrary::get().get(name);
    raylib::BeginShaderMode(shader);
    // Update common uniforms
    float t = static_cast<float>(raylib::GetTime());
    int timeLoc = raylib::GetShaderLocation(shader, "time");
    if (timeLoc != -1) {
      raylib::SetShaderValue(shader, timeLoc, &t, raylib::SHADER_UNIFORM_FLOAT);
    }
    auto *rez = EntityHelper::get_singleton_cmp<
        window_manager::ProvidesCurrentResolution>();
    if (rez) {
      vec2 r = {static_cast<float>(rez->current_resolution.width),
                static_cast<float>(rez->current_resolution.height)};
      int rezLoc = raylib::GetShaderLocation(shader, "resolution");
      if (rezLoc != -1) {
        raylib::SetShaderValue(shader, rezLoc, &r, raylib::SHADER_UNIFORM_VEC2);
      }
    }
  }
};

struct ConfigureTaggerSpotlight : System<> {
  virtual void once(float) override {
    if (!ShaderLibrary::get().contains("post_processing_tag")) {
      return;
    }
    // Always update common uniforms
    set_common_uniforms();
    auto &settings = RoundManager::get().get_active_settings();
    if (RoundManager::get().active_round_type != RoundType::TagAndGo) {
      set_enabled(false);
      return;
    }
    if (settings.state != RoundSettings::GameState::Countdown) {
      set_enabled(false);
      return;
    }
    OptEntity tagger = EntityQuery()
                           .whereHasComponent<Transform>()
                           .whereHasComponent<HasTagAndGoTracking>()
                           .whereLambda([](const Entity &e) {
                             return e.get<HasTagAndGoTracking>().is_tagger;
                           })
                           .gen_first();
    if (!tagger.valid()) {
      set_enabled(false);
      return;
    }
    auto *rez = EntityHelper::get_singleton_cmp<
        window_manager::ProvidesCurrentResolution>();
    if (!rez) {
      set_enabled(false);
      return;
    }
    vec2 screen = {static_cast<float>(rez->current_resolution.width),
                   static_cast<float>(rez->current_resolution.height)};
    vec2 center = tagger->get<Transform>().center();
    // Account for sprite fine-tune offsets used in rendering so spotlight
    // centers correctly
    const float offset_x = SPRITE_OFFSET_X;
    const float offset_y = SPRITE_OFFSET_Y;
    vec2 adjusted = {center.x + offset_x, center.y + offset_y};
    float ux = adjusted.x / std::max(1.0f, screen.x);
    float uy = adjusted.y / std::max(1.0f, screen.y);
    // Flip Y to match render texture sampling (DrawTexturePro uses negative
    // src.height)
    vec2 uv = {ux, 1.0f - uy};
    float t = static_cast<float>(raylib::GetTime());
    float radius = 0.22f + 0.02f * std::sin(t * 3.0f);
    float softness = 0.18f;
    float dim = 0.82f;
    float desat = 0.85f;
    set_values(true, uv, radius, softness, dim, desat);
  }

  void set_enabled(bool on) const {
    const auto &shader = ShaderLibrary::get().get("post_processing_tag");
    float enabled = on ? 1.0f : 0.0f;
    int loc = raylib::GetShaderLocation(shader, "spotlightEnabled");
    if (loc != -1) {
      raylib::SetShaderValue(shader, loc, &enabled,
                             raylib::SHADER_UNIFORM_FLOAT);
    }
  }

  void set_values(bool on, vec2 pos, float radius, float softness, float dim,
                  float desat) const {
    const auto &shader = ShaderLibrary::get().get("post_processing_tag");
    float enabled = on ? 1.0f : 0.0f;
    int locEnabled = raylib::GetShaderLocation(shader, "spotlightEnabled");
    if (locEnabled != -1) {
      raylib::SetShaderValue(shader, locEnabled, &enabled,
                             raylib::SHADER_UNIFORM_FLOAT);
    }
    int locPos = raylib::GetShaderLocation(shader, "spotlightPos");
    if (locPos != -1) {
      raylib::SetShaderValue(shader, locPos, &pos, raylib::SHADER_UNIFORM_VEC2);
    }
    int locRad = raylib::GetShaderLocation(shader, "spotlightRadius");
    if (locRad != -1) {
      raylib::SetShaderValue(shader, locRad, &radius,
                             raylib::SHADER_UNIFORM_FLOAT);
    }
    int locSoft = raylib::GetShaderLocation(shader, "spotlightSoftness");
    if (locSoft != -1) {
      raylib::SetShaderValue(shader, locSoft, &softness,
                             raylib::SHADER_UNIFORM_FLOAT);
    }
    int locDim = raylib::GetShaderLocation(shader, "dimAmount");
    if (locDim != -1) {
      raylib::SetShaderValue(shader, locDim, &dim,
                             raylib::SHADER_UNIFORM_FLOAT);
    }
    int locDesat = raylib::GetShaderLocation(shader, "desaturateAmount");
    if (locDesat != -1) {
      raylib::SetShaderValue(shader, locDesat, &desat,
                             raylib::SHADER_UNIFORM_FLOAT);
    }
  }

  void set_common_uniforms() const {
    if (ShaderLibrary::get().contains("post_processing")) {
      const auto &shader = ShaderLibrary::get().get("post_processing");
      float t = static_cast<float>(raylib::GetTime());
      int timeLoc = raylib::GetShaderLocation(shader, "time");
      if (timeLoc != -1) {
        raylib::SetShaderValue(shader, timeLoc, &t,
                               raylib::SHADER_UNIFORM_FLOAT);
      }
      auto *rez = EntityHelper::get_singleton_cmp<
          window_manager::ProvidesCurrentResolution>();
      if (rez) {
        vec2 r = {static_cast<float>(rez->current_resolution.width),
                  static_cast<float>(rez->current_resolution.height)};
        int rezLoc = raylib::GetShaderLocation(shader, "resolution");
        if (rezLoc != -1) {
          raylib::SetShaderValue(shader, rezLoc, &r,
                                 raylib::SHADER_UNIFORM_VEC2);
        }
      }
    }
    if (ShaderLibrary::get().contains("post_processing_tag")) {
      const auto &shader = ShaderLibrary::get().get("post_processing_tag");
      float t = static_cast<float>(raylib::GetTime());
      int timeLoc = raylib::GetShaderLocation(shader, "time");
      if (timeLoc != -1) {
        raylib::SetShaderValue(shader, timeLoc, &t,
                               raylib::SHADER_UNIFORM_FLOAT);
      }
      auto *rez = EntityHelper::get_singleton_cmp<
          window_manager::ProvidesCurrentResolution>();
      if (rez) {
        vec2 r = {static_cast<float>(rez->current_resolution.width),
                  static_cast<float>(rez->current_resolution.height)};
        int rezLoc = raylib::GetShaderLocation(shader, "resolution");
        if (rezLoc != -1) {
          raylib::SetShaderValue(shader, rezLoc, &r,
                                 raylib::SHADER_UNIFORM_VEC2);
        }
      }
    }
  }
};

struct RenderLetterboxBars : System<window_manager::ProvidesCurrentResolution> {
  virtual ~RenderLetterboxBars() {}
  virtual void for_each_with(
      const Entity &entity,
      const window_manager::ProvidesCurrentResolution &pCurrentResolution,
      float) const override {
    const int window_w = raylib::GetScreenWidth();
    const int window_h = raylib::GetScreenHeight();
    const int content_w = mainRT.texture.width;
    const int content_h = mainRT.texture.height;
    const LetterboxLayout layout =
        compute_letterbox_layout(window_w, window_h, content_w, content_h);

    if (layout.bar_left > 0) {
      raylib::DrawRectangle(0, 0, layout.bar_left, window_h, raylib::BLACK);
      raylib::DrawRectangle(window_w - layout.bar_right, 0, layout.bar_right,
                            window_h, raylib::BLACK);
    }
    if (layout.bar_top > 0) {
      raylib::DrawRectangle(0, 0, window_w, layout.bar_top, raylib::BLACK);
      raylib::DrawRectangle(0, window_h - layout.bar_bottom, window_w,
                            layout.bar_bottom, raylib::BLACK);
    }
  }
};

struct RenderDebugGridOverlay
    : System<window_manager::ProvidesCurrentResolution> {
  input::PossibleInputCollector<InputAction> inpc;

  virtual void once(float) override {
    inpc = input::get_input_collector<InputAction>();
  }

  virtual bool should_run(float) override {
    inpc = input::get_input_collector<InputAction>();
    if (!inpc.has_value())
      return false;
    bool toggle_pressed =
        std::ranges::any_of(inpc.inputs_pressed(), [](const auto &a) {
          return a.action == InputAction::ToggleUIDebug;
        });
    static bool enabled = false;
    if (toggle_pressed)
      enabled = !enabled;
    return enabled;
  }

  virtual void for_each_with(
      const Entity &,
      const window_manager::ProvidesCurrentResolution &pCurrentResolution,
      float) const override {
    auto resolution = pCurrentResolution.current_resolution;
    const int step = 80;
    const raylib::Color line_color{255, 255, 255, 60};
    const raylib::Color text_color{255, 255, 0, 200};

    for (int x = 0; x <= resolution.width; x += step) {
      raylib::DrawLine(x, 0, x, resolution.height, line_color);
      std::string label = fmt::format("{}", x);
      raylib::DrawText(label.c_str(), x + 4, 6, 12, text_color);
    }

    for (int y = 0; y <= resolution.height; y += step) {
      raylib::DrawLine(0, y, resolution.width, y, line_color);
      std::string label = fmt::format("{}", y);
      raylib::DrawText(label.c_str(), 6, y + 4, 12, text_color);
    }
  }
};

struct RenderEntities : System<Transform> {

  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             float) const override {
    if (entity.has<afterhours::texture_manager::HasSpritesheet>())
      return;
    if (entity.has<afterhours::texture_manager::HasAnimation>())
      return;

    // Check if entity has a shader and apply it
    bool has_shader = entity.has<HasShader>();
    raylib::Color render_color;

    if (has_shader) {
      const auto &shader_component = entity.get<HasShader>();

      if (ShaderLibrary::get().contains(shader_component.shader_name)) {

        const auto &shader =
            ShaderLibrary::get().get(shader_component.shader_name);
        raylib::BeginShaderMode(shader);
        render_color = raylib::MAGENTA;
      } else {
        log_warn("Shader not found in library: {}",
                 shader_component.shader_name);
        has_shader = false;
        render_color = entity.has_child_of<HasColor>()
                           ? entity.get_with_child<HasColor>().color()
                           : raylib::RAYWHITE;
      }
    } else {
      render_color = entity.has_child_of<HasColor>()
                         ? entity.get_with_child<HasColor>().color()
                         : raylib::RAYWHITE;
    }

    raylib::DrawRectanglePro(
        Rectangle{
            transform.center().x,
            transform.center().y,
            transform.size.x,
            transform.size.y,
        },
        vec2{transform.size.x / 2.f, transform.size.y / 2.f}, transform.angle,
        render_color);

    // End shader mode if we started it
    if (has_shader) {
      raylib::EndShaderMode();
    }
  }
};

struct UpdateColorBasedOnEntityID : System<HasEntityIDBasedColor> {

  virtual void for_each_with(Entity &,
                             HasEntityIDBasedColor &hasEntityIDBasedColor,
                             float) override {

    const auto parent_is_alive = EQ() //
                                     .whereID(hasEntityIDBasedColor.id)
                                     .has_values();
    if (parent_is_alive)
      return;
    hasEntityIDBasedColor.set(hasEntityIDBasedColor.default_);
  }
};

struct MatchKartsToPlayers : System<input::ProvidesMaxGamepadID> {

  virtual void for_each_with(Entity &,
                             input::ProvidesMaxGamepadID &maxGamepadID,
                             float) override {

    auto existing_players = EQ().whereHasComponent<PlayerID>().gen();

    // we are good
    if (existing_players.size() + 1 == maxGamepadID.count())
      return;

    if (existing_players.size() > maxGamepadID.count() + 1) {
      // remove the player that left
      for (Entity &player : existing_players) {
        if (input::is_gamepad_available(player.get<PlayerID>().id))
          continue;
        player.cleanup = true;
      }
      return;
    }
    // TODO add +1 here to auto gen extra players
    for (int i = 0; i < (int)maxGamepadID.count(); i++) {
      bool found = false;
      for (Entity &player : existing_players) {
        if (i == player.get<PlayerID>().id) {
          found = true;
          break;
        }
      }
      if (!found) {
        make_player(i);
      }
    }
  }
};

struct RenderWeaponCooldown : System<Transform, CanShoot> {

  virtual void for_each_with(const Entity &, const Transform &transform,
                             const CanShoot &canShoot, float) const override {

    for (auto it = canShoot.weapons.begin(); it != canShoot.weapons.end();
         ++it) {
      const std::unique_ptr<Weapon> &weapon = it->second;

      vec2 center = transform.center();
      Rectangle body = transform.rect();

      float nw = body.width / 2.f;
      float nh = body.height / 2.f;

      Rectangle arm = Rectangle{
          center.x, //
          center.y, //
          nw,
          nh * (weapon->cooldown / weapon->config.cooldownReset),
      };

      raylib::DrawRectanglePro(arm,
                               {nw / 2.f, nh / 2.f}, // rotate around center
                               transform.angle, raylib::RED);
    }
  }
};

struct WeaponCooldownSystem : PausableSystem<CanShoot> {
  virtual void for_each_with(Entity &, CanShoot &canShoot, float dt) override {
    magic_enum::enum_for_each<InputAction>([&](auto val) {
      constexpr InputAction action = val;
      canShoot.pass_time(action, dt);
    });
  }
};

struct WeaponFireSystem : PausableSystem<WantsWeaponFire, CanShoot, Transform> {
  virtual void for_each_with(Entity &entity, WantsWeaponFire &want,
                             CanShoot &canShoot, Transform &, float dt) override {
    if (!canShoot.weapons.contains(want.action)) {
      entity.removeComponent<WantsWeaponFire>();
      return;
    }
    auto &weapon = *canShoot.weapons[want.action];
    if (weapon.fire(dt)) {
      ProjectileConfig proj;
      proj.size = weapon.config.size;
      proj.speed = weapon.config.speed;
      proj.acceleration = weapon.config.acceleration;
      proj.life_time_seconds = weapon.config.life_time_seconds;
      proj.spread = weapon.config.spread;
      proj.can_wrap_around = weapon.config.can_wrap_around;
      proj.render_out_of_bounds = weapon.config.render_out_of_bounds;
      proj.base_damage = weapon.config.base_damage;
      proj.angle_offsets = {0.f};

      RecoilConfig rec{weapon.config.knockback_amt};

      entity.addComponent<WeaponFired>(
          WeaponFired{want.action, static_cast<int>(weapon.type),
                      static_cast<int>(weapon.firing_direction), std::move(proj), std::move(rec)});
    }
    entity.removeComponent<WantsWeaponFire>();
  }
};

struct ProjectileSpawnSystem : System<WeaponFired, Transform> {
  virtual void for_each_with(Entity &entity, WeaponFired &evt,
                             Transform &transform, float) override {
    (void)transform;
    // Adjust projectile pattern by weapon type
    ProjectileConfig cfg = std::move(evt.projectile);
    switch (static_cast<Weapon::Type>(evt.weapon_type)) {
    case Weapon::Type::Shotgun:
      cfg.angle_offsets = {-15.f, -5.f, 5.f, 15.f};
      break;
    default:
      break;
    }

    const float base_angle = entity.get<Transform>().angle;

    make_poof_anim(entity, static_cast<Weapon::FiringDirection>(evt.firing_direction),
                   base_angle, 0.f);

    for (float ao : cfg.angle_offsets) {
      make_bullet(entity, cfg,
                  static_cast<Weapon::FiringDirection>(evt.firing_direction),
                  base_angle, ao);
    }
  }
};

struct WeaponRecoilSystem : System<WeaponFired, Transform> {
  virtual void for_each_with(Entity &entity, WeaponFired &evt, Transform &t,
                             float) override {
    const float knockback_amt = evt.recoil.knockback_amt;
    vec2 recoil = {std::cos(t.as_rad()), std::sin(t.as_rad())};
    recoil = vec_norm(vec2{-recoil.y, recoil.x});
    t.velocity += (recoil * knockback_amt);
  }
};

struct WeaponSoundSystem : System<WeaponFired> {
  virtual void for_each_with(Entity &, WeaponFired &evt, float) override {
    switch (static_cast<Weapon::Type>(evt.weapon_type)) {
    case Weapon::Type::Cannon:
      SoundLibrary::get().play(SoundFile::Weapon_Canon_Shot);
      break;
    case Weapon::Type::Sniper:
      SoundLibrary::get().play(SoundFile::Weapon_Sniper_Shot);
      break;
    case Weapon::Type::Shotgun:
      SoundLibrary::get().play(SoundFile::Weapon_Shotgun_Shot);
      break;
    case Weapon::Type::MachineGun:
      SoundLibrary::get().play_random_match(
          "SPAS-12_-_FIRING_-_Pump_Action_-_Take_1_-_20m_In_Front_-_AB_-_MKH8020_");
      break;
    default:
      break;
    }
  }
};

struct WeaponFiredCleanupSystem : System<WeaponFired> {
  virtual void for_each_with(Entity &entity, WeaponFired &, float) override {
    entity.removeComponent<WeaponFired>();
  }
};

struct Shoot : PausableSystem<PlayerID, Transform, CanShoot> {
  input::PossibleInputCollector<InputAction> inpc;

  virtual void once(float) override {
    inpc = input::get_input_collector<InputAction>();
  }
  virtual void for_each_with(Entity &entity, PlayerID &playerID, Transform &,
                             CanShoot &, float) override {

    if (!inpc.has_value()) {
      return;
    }

    for (const auto &actions_done : inpc.inputs()) {
      if (actions_done.id != playerID.id)
        continue;

      if (actions_done.amount_pressed > 0.f) {
        entity.addComponentIfMissing<WantsWeaponFire>(actions_done.action);
      }
    }
  }
};

struct WrapAroundTransform : System<Transform, CanWrapAround> {

  window_manager::Resolution resolution;

  virtual void once(float) override {
    resolution =
        EQ().whereHasComponent<
                afterhours::window_manager::ProvidesCurrentResolution>()
            .gen_first_enforce()
            .get<afterhours::window_manager::ProvidesCurrentResolution>()
            .current_resolution;
  }

  virtual void for_each_with(Entity &entity, Transform &transform,
                             CanWrapAround &canWrap, float) override {

    float width = (float)resolution.width;
    float height = (float)resolution.height;

    float padding = canWrap.padding;

    raylib::Rectangle screenRect{0, 0, width, height};
    const auto overlaps =
        EQ::WhereOverlaps::overlaps(screenRect, transform.rect());
    if (overlaps) {
      // Early return, no wrapping checks need to be done further.
      return;
    }

    // Non-overlapping logic

    // If it's not overlapping the screen rect and it doesn't want to be
    // rendered out of bounds
    if (!transform.render_out_of_bounds || transform.cleanup_out_of_bounds) {
      entity.cleanup = transform.cleanup_out_of_bounds;
      return;
    }

    if (transform.rect().x > width + padding) {
      transform.position.x = -padding;
    }

    if (transform.rect().x < 0 - padding) {
      transform.position.x = width + padding;
    }

    if (transform.rect().y < 0 - padding) {
      transform.position.y = height + padding;
    }

    if (transform.rect().y > height + padding) {
      transform.position.y = -padding;
    }
  }
};

struct SkidMarks : System<Transform, TireMarkComponent> {
  virtual void for_each_with(Entity &entity, Transform &transform,
                             TireMarkComponent &tire, float dt) override {
    tire.pass_time(dt);
    if (!should_render_skid(transform)) {
      tire.added_last_frame = false;
      return;
    }
    const vec2 markPosition = transform.center();
    const bool useWinnerColors =
        entity.has<HasShader>() &&
        (entity.get<HasShader>().shader_name == std::string("car_winner"));
    const float markHue = useWinnerColors ? tire.rolling_hue : 0.0f;
    tire.add_mark(markPosition, !tire.added_last_frame, markHue);
    tire.added_last_frame = true;
    if (useWinnerColors) {
      tire.rolling_hue += 0.10f;
      if (tire.rolling_hue > 6.28318f) {
        tire.rolling_hue -= 6.28318f;
      }
    }
  }

private:
  bool should_render_skid(const Transform &transform) const {
    if (transform.accel_mult > 2.f) {
      return true;
    }
    if (transform.speed() == 0.f) {
      return false;
    }
    const vec2 velocityNormalized = transform.velocity / transform.speed();
    const float angleRadians = to_radians(transform.angle - 90.f);
    const vec2 carForward = {static_cast<float>(std::cos(angleRadians)),
                             static_cast<float>(std::sin(angleRadians))};
    const float velocityForwardDot = vec_dot(velocityNormalized, carForward);
    const bool movingSideways = std::fabs(velocityForwardDot) <
                                (Config::get().skid_threshold.data / 100.f);
    return movingSideways;
  }
};

struct RenderSkid : System<Transform, TireMarkComponent> {
  virtual void for_each_with(const Entity &entity, const Transform &,
                             const TireMarkComponent &tire,
                             float) const override {
    const bool useWinnerColors =
        entity.has<HasShader>() &&
        (entity.get<HasShader>().shader_name == std::string("car_winner"));
    const float offsetX = 7.f;
    const float offsetY = 4.f;
    render_single_tire(tire, vec2{offsetX, offsetY}, useWinnerColors);
    render_single_tire(tire, vec2{-offsetX, -offsetY}, useWinnerColors);
  }

  static raylib::Color rainbow_from_hue(float hue, unsigned char alpha) {
    const float red = sinf(hue + 0.0f) * 0.5f + 0.5f;
    const float green = sinf(hue + 2.094f) * 0.5f + 0.5f;
    const float blue = sinf(hue + 4.188f) * 0.5f + 0.5f;
    return raylib::Color(static_cast<unsigned char>(red * 255.0f),
                         static_cast<unsigned char>(green * 255.0f),
                         static_cast<unsigned char>(blue * 255.0f), alpha);
  }

  void render_single_tire(const TireMarkComponent &tire, vec2 offset,
                          bool useWinnerColors) const {
    for (size_t i = 1; i < tire.points.size(); i++) {
      auto mp0 = tire.points[i - 1];
      auto mp1 = tire.points[i];
      if (mp0.gap || mp1.gap) {
        continue;
      }
      if (distance_sq(mp0.position, mp1.position) > 100.f) {
        continue;
      }
      float fade = mp0.time / mp0.lifetime;
      fade = fmaxf(0.0f, fminf(1.0f, fade));
      if (fade <= 0.0f) {
        continue;
      }
      const unsigned char a = static_cast<unsigned char>(255.0f * fade);
      raylib::Color col = useWinnerColors ? rainbow_from_hue(mp0.hue, a)
                                          : raylib::Color(20, 20, 20, a);
      raylib::DrawSplineSegmentLinear(mp0.position + offset,
                                      mp1.position + offset, 5.f, col);
    }
  }
};

struct RenderOOB : System<Transform> {
  window_manager::Resolution resolution;
  Rectangle screen;

  virtual void once(float) override {
    resolution = EntityHelper::get_singleton_cmp<
                     afterhours::window_manager::ProvidesCurrentResolution>()
                     ->current_resolution;

    screen = Rectangle{0, 0, (float)resolution.width, (float)resolution.height};
  }

  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             float) const override {
    if (is_point_inside(transform.pos(), screen) ||
        !transform.render_out_of_bounds) {
      return;
    }

    const auto size =
        std::max(5.f, //
                 std::lerp(20.f, 5.f,
                           (distance_sq(transform.pos(), rect_center(screen)) /
                            (screen.width * screen.height))));

    raylib::DrawCircleV(calc(screen, transform.pos()), size,
                        entity.has<HasColor>() ? entity.get<HasColor>().color()
                                               : raylib::PINK);
  }
};

struct UpdateTrackingEntities : System<Transform, TracksEntity> {
  virtual void for_each_with(Entity &, Transform &transform,
                             TracksEntity &tracker, float) override {

    OptEntity opte = EQ().whereID(tracker.id).gen_first();
    if (!opte.valid())
      return;
    transform.position = (opte->get<Transform>().pos() + tracker.offset);
    transform.angle = opte->get<Transform>().angle;
  }
};

struct UpdateCollidingEntities : PausableSystem<Transform> {

  std::set<int> ids;

  virtual void once(float) override { ids.clear(); }

  void positional_correction(Transform &a, Transform &b,
                             const vec2 &collisionNormal,
                             float penetrationDepth) {
    float correctionMagnitude =
        std::max(penetrationDepth, 0.0f) /
        (1.0f / a.collision_config.mass + 1.0f / b.collision_config.mass);
    vec2 correction = collisionNormal * correctionMagnitude;

    a.position -= correction / a.collision_config.mass;
    b.position += correction / b.collision_config.mass;
  }

  void resolve_collision(Transform &a, Transform &b, const float dt) {
    vec2 collisionNormal = vec_norm(b.position - a.position);

    // Calculate normal impulse
    float impulse = calculate_impulse(a, b, collisionNormal);
    vec2 impulseVector =
        collisionNormal * impulse * Config::get().collision_scalar.data * dt;

    // Apply normal impulse
    if (a.collision_config.mass > 0.0f &&
        a.collision_config.mass != std::numeric_limits<float>::max()) {
      a.velocity -= impulseVector / a.collision_config.mass;
    }

    if (b.collision_config.mass > 0.0f &&
        b.collision_config.mass != std::numeric_limits<float>::max()) {
      b.velocity += impulseVector / b.collision_config.mass;
    }

    // Calculate and apply friction impulse
    vec2 relativeVelocity = b.velocity - a.velocity;
    vec2 tangent =
        vec_norm(relativeVelocity -
                 collisionNormal * vec_dot(relativeVelocity, collisionNormal));

    float frictionImpulseMagnitude =
        vec_dot(relativeVelocity, tangent) /
        (1.0f / a.collision_config.mass + 1.0f / b.collision_config.mass);
    float frictionCoefficient =
        std::sqrt(a.collision_config.friction * b.collision_config.friction);
    frictionImpulseMagnitude =
        std::clamp(frictionImpulseMagnitude, -impulse * frictionCoefficient,
                   impulse * frictionCoefficient);

    vec2 frictionImpulse = tangent * frictionImpulseMagnitude *
                           Config::get().collision_scalar.data * dt;

    if (a.collision_config.mass > 0.0f &&
        a.collision_config.mass != std::numeric_limits<float>::max()) {
      a.velocity -= frictionImpulse / a.collision_config.mass;
    }

    if (b.collision_config.mass > 0.0f &&
        b.collision_config.mass != std::numeric_limits<float>::max()) {
      b.velocity += frictionImpulse / b.collision_config.mass;
    }

    // Positional correction
    float penetrationDepth = calculate_penetration_depth(a.rect(), b.rect());
    positional_correction(a, b, collisionNormal, penetrationDepth);
  }

  float calculate_penetration_depth(const Rectangle &a, const Rectangle &b) {
    // Calculate the overlap along the X axis
    float overlapX =
        std::min(a.x + a.width, b.x + b.width) - std::max(a.x, b.x);

    // Calculate the overlap along the Y axis
    float overlapY =
        std::min(a.y + a.height, b.y + b.height) - std::max(a.y, b.y);

    // If there's no overlap, return 0
    if (overlapX <= 0.0f || overlapY <= 0.0f)
      return 0.0f;

    // Return the smaller overlap for the penetration depth
    return std::min(overlapX, overlapY);
  }

  float calculate_dynamic_restitution(const Transform &a, const Transform &b) {
    float baseRestitution = std::min(a.collision_config.restitution,
                                     b.collision_config.restitution);

    // Reduce restitution for high-speed collisions
    vec2 relativeVelocity = b.velocity - a.velocity;
    float speed = Vector2Length(relativeVelocity);

    if (speed >
        (Config::get().max_speed.data * .75f)) { // Adjust threshold as needed
      baseRestitution *= 0.5f; // Reduce bounce for high-speed collisions
    }

    return baseRestitution;
  }

  float calculate_impulse(const Transform &a, const Transform &b,
                          const vec2 &collisionNormal) {
    vec2 relativeVelocity = b.velocity - a.velocity;
    float velocityAlongNormal = vec_dot(relativeVelocity, collisionNormal);

    // Prevent objects from "sticking" or resolving collisions when moving apart
    if (velocityAlongNormal > 0.0f)
      return 0.0f;

    float restitution = calculate_dynamic_restitution(a, b);

    // Impulse calculation with restitution
    float impulse = -(1.0f + restitution) * velocityAlongNormal;
    impulse /=
        (1.0f / a.collision_config.mass + 1.0f / b.collision_config.mass);

    return impulse;
  }

  virtual void for_each_with(Entity &entity, Transform &transform,
                             float dt) override {

    // skip any already resolved
    if (ids.contains(entity.id)) {
      return;
    }

    const auto gets_absorbed = [](Entity &ent) {
      return ent.has<CollisionAbsorber>() &&
             ent.get<CollisionAbsorber>().absorber_type ==
                 CollisionAbsorber::AbsorberType::Absorbed;
    };

    if (gets_absorbed(entity)) {
      return;
    }

    if (entity.has<IsFloorOverlay>()) {
      return;
    }

    auto can_collide = EQ().whereHasComponent<Transform>()
                           .whereMissingComponent<IsFloorOverlay>()
                           .whereNotID(entity.id)
                           .whereOverlaps(transform.rect())
                           .gen();

    for (Entity &other : can_collide) {
      Transform &b = other.get<Transform>();

      // If the other transform gets absorbed, but this is its parent, ignore
      // collision.
      if (gets_absorbed(other)) {
        if (other.get<CollisionAbsorber>().parent_id.value_or(-1) ==
            entity.id) {
          ids.insert(other.id);
          continue;
        }
      }

      resolve_collision(transform, b, dt);
      ids.insert(other.id);
    }
  }
};

struct VelFromInput
    : PausableSystem<PlayerID, Transform, HonkState, HasShader> {
  virtual void for_each_with(Entity &entity, PlayerID &playerID,
                             Transform &transform, HonkState &honk,
                             HasShader &hasShader, float dt) override {
    input::PossibleInputCollector<InputAction> inpc =
        input::get_input_collector<InputAction>();
    if (!inpc.has_value()) {
      return;
    }

    transform.accel = 0.f;
    auto steer = 0.f;

    bool honk_down = false;
    for (const auto &actions_done : inpc.inputs()) {
      if (actions_done.id != playerID.id) {
        continue;
      }

      switch (actions_done.action) {
      case InputAction::Accel:
        transform.accel = transform.is_reversing()
                              ? -Config::get().breaking_acceleration.data
                              : Config::get().forward_acceleration.data;
        break;
      case InputAction::Brake:
        transform.accel = transform.is_reversing()
                              ? Config::get().reverse_acceleration.data
                              : -Config::get().breaking_acceleration.data;
        break;
      case InputAction::Left:
        steer = -actions_done.amount_pressed;
        break;
      case InputAction::Right:
        steer = actions_done.amount_pressed;
        break;
      case InputAction::Boost:
        break;
      case InputAction::Honk:
        honk_down = true;
        break;
      default:
        break;
      }
    }

    for (auto &actions_done : inpc.inputs_pressed()) {
      if (actions_done.id != playerID.id) {
        continue;
      }

      switch (actions_done.action) {
      case InputAction::Accel:
        break;
      case InputAction::Brake:
        break;
      case InputAction::Left:
        break;
      case InputAction::Right:
        break;
      case InputAction::Boost: {
        entity.addComponentIfMissing<WantsBoost>();
      } break;
      case InputAction::Honk: {
        if (auto opt = EntityQuery({.force_merge = true})
                           .whereHasComponent<SoundEmitter>()
                           .gen_first();
            opt.valid()) {
          auto &ent = opt.asE();
          auto &req = ent.addComponentIfMissing<PlaySoundRequest>();
          req.policy = honk.was_down ? PlaySoundRequest::Policy::PrefixIfNonePlaying
                                     : PlaySoundRequest::Policy::PrefixFirstAvailable;
          req.prefix =
              "VEHHorn_Renault_R4_GTL_Horn_Signal_01_Interior_JSE_RR4_Mono_";
        }
      } break;
      default:
        break;
      }
    }

    const char *horn_prefix =
        "VEHHorn_Renault_R4_GTL_Horn_Signal_01_Interior_JSE_RR4_Mono_";
    if (honk_down) {
      if (auto opt = EntityQuery({.force_merge = true})
                         .whereHasComponent<SoundEmitter>()
                         .gen_first();
          opt.valid()) {
        auto &ent = opt.asE();
        auto &req = ent.addComponentIfMissing<PlaySoundRequest>();
        req.policy = honk.was_down ? PlaySoundRequest::Policy::PrefixIfNonePlaying
                                   : PlaySoundRequest::Policy::PrefixFirstAvailable;
        req.prefix = horn_prefix;
      }
    }
    honk.was_down = honk_down;

    float steering_multiplier = affector_steering_multiplier(transform);
    float steering_sensitivity =
        Config::get().steering_sensitivity.data +
        affector_steering_sensitivity_additive(transform);

    if (transform.speed() > 0.01) {
      const auto minRadius = Config::get().minimum_steering_radius.data;
      const auto maxRadius = Config::get().maximum_steering_radius.data;
      const auto speed_percentage =
          transform.speed() / Config::get().max_speed.data;

      const auto rad = std::lerp(minRadius, maxRadius, speed_percentage);

      transform.angle +=
          steer * steering_sensitivity * dt * rad * steering_multiplier;
      transform.angle = std::fmod(transform.angle + 360.f, 360.f);
    }

    float accel_multiplier = affector_acceleration_multiplier(transform);

    float mvt{0.f};
    if (transform.accel != 0.f) {
      mvt = std::clamp(
          transform.speed() +
              (transform.accel * transform.accel_mult * accel_multiplier),
          -Config::get().max_speed.data, Config::get().max_speed.data);
    } else {
      mvt = std::clamp(transform.speed(), -Config::get().max_speed.data,
                       Config::get().max_speed.data);
    }

    // Apply speed multiplier for TagAndGo mode
    if (RoundManager::get().active_round_type == RoundType::TagAndGo) {
      auto &tag_settings =
          RoundManager::get().get_active_rt<RoundTagAndGoSettings>();
      mvt *= tag_settings.speed_multiplier;
    }

    if (!transform.is_reversing()) {
      transform.velocity += vec2{
          std::sin(transform.as_rad()) * mvt * dt,
          -std::cos(transform.as_rad()) * mvt * dt,
      };
    } else {
      transform.velocity += vec2{
          -std::sin(transform.as_rad()) * mvt * dt,
          std::cos(transform.as_rad()) * mvt * dt,
      };
    }

    transform.speed_dot_angle =
        transform.velocity.x * std::sin(transform.as_rad()) +
        transform.velocity.y * -std::cos(transform.as_rad());
  }
};

struct ProcessBoostRequests : PausableSystem<WantsBoost, Transform> {
  virtual void for_each_with(Entity &entity, WantsBoost &, Transform &transform,
                             float) override {
    if (transform.is_reversing() || transform.accel_mult > 1.f) {
      entity.removeComponent<WantsBoost>();
      return;
    }
    if (auto opt = EntityQuery({.force_merge = true})
                       .whereHasComponent<SoundEmitter>()
                       .gen_first();
        opt.valid()) {
      auto &ent = opt.asE();
      auto &req = ent.addComponentIfMissing<PlaySoundRequest>();
      req.policy = PlaySoundRequest::Policy::PrefixRandom;
      req.prefix = "AIRBrst_Steam_Release_Short_03_JSE_SG_Mono_";
    }
    transform.accel_mult = Config::get().boost_acceleration.data;
    const auto upfront_boost_speed = Config::get().max_speed.data * .2f;
    transform.velocity +=
        vec2{std::sin(transform.as_rad()) * upfront_boost_speed,
             -std::cos(transform.as_rad()) * upfront_boost_speed};
    entity.removeComponent<WantsBoost>();
  }
};

struct BoostDecay : PausableSystem<Transform> {
  virtual void for_each_with(Entity &, Transform &transform,
                             float dt) override {
    const auto decayed_accel_mult =
        transform.accel_mult -
        (transform.accel_mult * Config::get().boost_decay_percent.data * dt);
    transform.accel_mult = std::max(1.f, decayed_accel_mult);
  }
};

struct Move : PausableSystem<Transform> {

  virtual void for_each_with(Entity &, Transform &transform, float) override {
    transform.position += transform.velocity;
    float damp = transform.accel != 0 ? 0.99f : 0.98f;
    float speed_mult = affector_speed_multiplier(transform);
    transform.velocity = transform.velocity * (damp * speed_mult);
  }
};

struct DrainLife : System<HasLifetime> {

  virtual void for_each_with(Entity &entity, HasLifetime &hasLifetime,
                             float dt) override {

    hasLifetime.lifetime -= dt;
    if (hasLifetime.lifetime <= 0) {
      entity.cleanup = true;
    }
  }
};

struct ProcessDamage : PausableSystem<Transform, HasHealth> {

  virtual void for_each_with(Entity &entity, Transform &transform,
                             HasHealth &hasHealth, float dt) override {

    hasHealth.pass_time(dt);
    if (hasHealth.iframes > 0.f) {
      return;
    }

    auto can_damage = EQ().whereHasComponent<CanDamage>()
                          .whereNotID(entity.id)
                          .whereOverlaps(transform.rect())
                          .gen();

    for (Entity &damager : can_damage) {
      const CanDamage &cd = damager.get<CanDamage>();
      if (cd.id == entity.id)
        continue;
      hasHealth.amount -= cd.amount;
      hasHealth.iframes = hasHealth.iframesReset;

      // Track the entity that caused this damage for kill attribution
      hasHealth.last_damaged_by = cd.id;
      damager.cleanup = true;
    }
  }
};

struct ProcessCollisionAbsorption : System<Transform, CollisionAbsorber> {

  virtual void for_each_with(Entity &entity, Transform &transform,
                             CollisionAbsorber &collision_absorber,
                             float) override {
    // We let the absorbed things (e.g. bullets) manage cleaning themselves up,
    // rather than the other way around.
    if (collision_absorber.absorber_type ==
        CollisionAbsorber::AbsorberType::Absorber) {
      return;
    }

    const auto unrelated_absorber =
        [&collision_absorber](const Entity &collider) {
          const auto &other_absorber = collider.get<CollisionAbsorber>();
          const auto are_related = collision_absorber.parent_id.value_or(-1) ==
                                   other_absorber.parent_id.value_or(-2);

          if (are_related) {
            return false;
          }

          return other_absorber.absorber_type ==
                 CollisionAbsorber::AbsorberType::Absorber;
        };

    auto collided_with_absorber =
        EQ().whereHasComponent<CollisionAbsorber>()
            .whereNotID(entity.id)
            .whereNotID(collision_absorber.parent_id.value_or(-1))
            .whereOverlaps(transform.rect())
            .whereLambda(unrelated_absorber)
            .gen();

    if (!collided_with_absorber.empty()) {
      entity.cleanup = true;
    }
  }
};

struct ProcessDeath : PausableSystem<Transform, HasHealth> {

  virtual void for_each_with(Entity &entity, Transform &transform,
                             HasHealth &hasHealth, float) override {
    if (hasHealth.amount > 0) {
      return;
    }

    log_info("Entity {} died with health {}", entity.id, hasHealth.amount);
    make_explosion_anim(entity);

    // Handle player respawning
    if (entity.has<PlayerID>()) {
      transform.position =
          get_spawn_position(static_cast<size_t>(entity.get<PlayerID>().id));
    }

    // Handle kill attribution in kill-based rounds
    if (RoundManager::get().active_round_type == RoundType::Kills) {
      handle_kill_attribution(entity, hasHealth);
    }

    // Handle lives system
    if (entity.has<HasMultipleLives>()) {
      if (RoundManager::get().active_round_type == RoundType::Kills) {
        hasHealth.amount = hasHealth.max_amount;
        return;
      }

      entity.get<HasMultipleLives>().num_lives_remaining -= 1;
      if (entity.get<HasMultipleLives>().num_lives_remaining) {
        hasHealth.amount = hasHealth.max_amount;
        return;
      }
    }

    entity.cleanup = true;
  }

private:
  void handle_kill_attribution(const Entity &, const HasHealth &hasHealth) {
    if (!hasHealth.last_damaged_by.has_value()) {
      log_warn("Player died but we don't know why");
      return;
    }

    // Look up the entity that caused the damage
    auto damager_entities = EntityQuery({.force_merge = true})
                                .whereID(*hasHealth.last_damaged_by)
                                .gen();

    if (damager_entities.empty()) {
      log_warn("Player died but damager entity not found");
      return;
    }

    Entity &damager = damager_entities[0].get();

    if (damager.has<HasKillCountTracker>()) {
      damager.get<HasKillCountTracker>().kills++;
      return;
    }

    log_warn("Death from non-attributable source - no kill awarded");
  }
};

struct RenderLabels : System<Transform, HasLabels> {
  virtual void for_each_with(const Entity &, const Transform &transform,
                             const HasLabels &hasLabels, float) const override {

    const auto get_label_display_for_type = [](const Transform &transform_in,
                                               const LabelInfo &label_info_in) {
      switch (label_info_in.label_type) {
      case LabelInfo::LabelType::StaticText:
        return label_info_in.label_text;
      case LabelInfo::LabelType::VelocityText:
        return (transform_in.is_reversing() ? "-" : "") +
               fmt::format("{}", transform_in.speed()) + label_info_in.label_text;
      case LabelInfo::LabelType::AccelerationText:
        return fmt::format("{}", transform_in.accel * transform_in.accel_mult) +
               label_info_in.label_text;
      }

      return label_info_in.label_text;
    };

    const auto width = transform.rect().width;
    const auto height = transform.rect().height;

    // Makes the label percentages scale from top-left of the object rect as (0,
    // 0)
    const auto base_x_offset = transform.pos().x - width;
    const auto base_y_offset = transform.pos().y - height;

    for (const auto &label_info : hasLabels.label_info) {
      const auto label_to_display =
          get_label_display_for_type(transform, label_info);
      const auto label_pos_offset = label_info.label_pos_offset;

      const auto x_offset = base_x_offset + (width * label_pos_offset.x);
      const auto y_offset = base_y_offset + (height * label_pos_offset.y);

      draw_text_ex(
          EntityHelper::get_singleton_cmp<ui::FontManager>()->get_active_font(),
          label_to_display.c_str(), vec2{x_offset, y_offset},
          transform.rect().height / 2.f, 1.f, raylib::RAYWHITE);
    }
  }
};

struct RenderPlayerHUD : System<Transform, HasHealth> {

  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             const HasHealth &hasHealth, float) const override {
    // Always render health bar
    const float scale_x = 2.f;
    const float scale_y = 1.25f;

    raylib::Color color = entity.has_child_of<HasColor>()
                              ? entity.get_with_child<HasColor>().color()
                              : raylib::GREEN;

    float health_as_percent = static_cast<float>(hasHealth.amount) /
                              static_cast<float>(hasHealth.max_amount);

    vec2 rotation_origin{0, 0};

    // Render the red background bar
    raylib::DrawRectanglePro(
        Rectangle{
            transform.pos().x - ((transform.size.x * scale_x) / 2.f) +
                5.f, // Center with scaling
            transform.pos().y -
                (transform.size.y + 10.0f),    // Slightly above the entity
            transform.size.x * scale_x,        // Adjust length
            (transform.size.y / 4.f) * scale_y // Adjust height
        },
        rotation_origin, 0.0f, raylib::RED);

    // Render the green health bar
    raylib::DrawRectanglePro(
        Rectangle{
            transform.pos().x - ((transform.size.x * scale_x) / 2.f) +
                5.f, // Start at the same position as red bar
            transform.pos().y -
                (transform.size.y + 10.0f), // Same vertical position as red bar
            (transform.size.x * scale_x) *
                health_as_percent, // Adjust length based on health percentage
            (transform.size.y / 4.f) * scale_y // Adjust height
        },
        rotation_origin, 0.0f, color);

    // Render round-specific information above health bar
    switch (RoundManager::get().active_round_type) {
    case RoundType::Lives:
      render_lives(entity, transform, color);
      break;
    case RoundType::Kills:
      render_kills(entity, transform, color);
      break;
    case RoundType::TagAndGo:
      render_tagger_indicator(entity, transform, color);
      break;
    default:
      break;
    }
  }

private:
  void render_lives(const Entity &entity, const Transform &transform,
                    raylib::Color color) const {
    if (!entity.has<HasMultipleLives>())
      return;

    const auto &hasMultipleLives = entity.get<HasMultipleLives>();
    float rad = 5.f;
    vec2 off{rad * 2 + 2, 0.f};
    for (int i = 0; i < hasMultipleLives.num_lives_remaining; i++) {
      raylib::DrawCircleV(
          transform.pos() -
              vec2{transform.size.x / 2.f, transform.size.y + 15.f + rad} +
              (off * (float)i),
          rad, color);
    }
  }

  void render_kills(const Entity &entity, const Transform &transform,
                    raylib::Color color) const {
    if (!entity.has<HasKillCountTracker>())
      return;

    const auto &hasKillCountTracker = entity.get<HasKillCountTracker>();
    std::string kills_text = fmt::format("{} kills", hasKillCountTracker.kills);
    float text_size = 12.f;

    raylib::DrawText(
        kills_text.c_str(), static_cast<int>(transform.pos().x - 30.f),
        static_cast<int>(transform.pos().y - transform.size.y - 25.f),
        static_cast<int>(text_size), color);
  }

  void render_tagger_indicator(const Entity &entity, const Transform &transform,
                               raylib::Color) const {
    // TODO add color to entity
    if (!entity.has<HasTagAndGoTracking>())
      return;

    const auto &taggerTracking = entity.get<HasTagAndGoTracking>();

    // Draw crown for the tagger
    if (taggerTracking.is_tagger) {
      // Draw a crown above the player who is "it"
      const float crown_size = 15.f;
      const float crown_y_offset = transform.size.y + 20.f;

      // Crown position (centered above the player)
      vec2 crown_pos = transform.pos() - vec2{crown_size / 2.f, crown_y_offset};

      // Draw crown using simple shapes
      raylib::Color crown_color = raylib::GOLD;

      // Crown base
      raylib::DrawRectangle(static_cast<int>(crown_pos.x),
                            static_cast<int>(crown_pos.y),
                            static_cast<int>(crown_size),
                            static_cast<int>(crown_size / 3.f), crown_color);

      // Crown points (3 triangles)
      float point_width = crown_size / 3.f;
      for (int i = 0; i < 3; i++) {
        float x = crown_pos.x + (i * point_width);
        raylib::DrawTriangle(
            vec2{x, crown_pos.y},
            vec2{x + point_width / 2.f, crown_pos.y - crown_size / 2.f},
            vec2{x + point_width, crown_pos.y}, crown_color);
      }

      // Crown jewels (small circles)
      raylib::DrawCircleV(crown_pos + vec2{crown_size / 2.f, crown_size / 6.f},
                          2.f, raylib::RED);
    }

    // Draw shield for players in cooldown (safe period)
    float current_time = static_cast<float>(raylib::GetTime());
    auto &tag_settings =
        RoundManager::get().get_active_rt<RoundTagAndGoSettings>();
    if (current_time - taggerTracking.last_tag_time <
        tag_settings.tag_cooldown_time) {
      // TODO: Add pulsing animation to shield to make it more obvious
      // TODO: Add countdown timer above shield showing remaining safe time
      // Draw a shield above the player who is safe
      const float shield_size = 12.f;
      const float shield_y_offset = transform.size.y + 35.f; // Above crown

      // Shield position (centered above the player)
      vec2 shield_pos =
          transform.pos() - vec2{shield_size / 2.f, shield_y_offset};

      // Draw shield using simple shapes
      raylib::Color shield_color = raylib::SKYBLUE;

      // Shield base (triangle pointing down)
      raylib::DrawTriangle(
          vec2{shield_pos.x + shield_size / 2.f, shield_pos.y}, // Top point
          vec2{shield_pos.x, shield_pos.y + shield_size},       // Bottom left
          vec2{shield_pos.x + shield_size,
               shield_pos.y + shield_size}, // Bottom right
          shield_color);

      // Shield border
      raylib::DrawTriangleLines(
          vec2{shield_pos.x + shield_size / 2.f, shield_pos.y}, // Top point
          vec2{shield_pos.x, shield_pos.y + shield_size},       // Bottom left
          vec2{shield_pos.x + shield_size,
               shield_pos.y + shield_size}, // Bottom right
          raylib::WHITE);
    }
  }
};

struct ApplyWinnerShader : System<WasWinnerLastRound, HasShader> {
  virtual void for_each_with(Entity &entity, WasWinnerLastRound &,
                             HasShader &hasShader, float) override {
    // Apply the car_winner shader to the winner
    hasShader.shader_name = "car_winner";

    // Remove the winner marker component since we've applied the shader
    entity.removeComponent<WasWinnerLastRound>();
  }
};
