
#ifdef BACKWARD
#include "backward/backward.hpp"
namespace backward {
backward::SignalHandling sh;
} // namespace backward
#endif

#include "game.h"
//
#include "argh.h"
#include "intro.h"
#include "map_system.h"
#include "navigation.h"
#include "preload.h"
#include "settings.h"
#include "sound_systems.h"
#include "systems.h"
#include "systems_ai.h"
#include "ui/animation_button_wiggle.h"
#include "ui/animation_key.h"
#include "ui/animation_slide_in.h"
#include "ui_systems.h"
#include <afterhours/src/plugins/animation.h>

// TODO add honking

bool running = true;
// TODO move into library or somethign
raylib::RenderTexture2D mainRT;
raylib::RenderTexture2D screenRT;

using namespace afterhours;

void game() {
  mainRT = raylib::LoadRenderTexture(Settings::get().get_screen_width(),
                                     Settings::get().get_screen_height());
  screenRT = raylib::LoadRenderTexture(Settings::get().get_screen_width(),
                                       Settings::get().get_screen_height());

  SystemManager systems;

  // debug systems
  {
    window_manager::enforce_singletons(systems);
    ui::enforce_singletons<InputAction>(systems);
    input::enforce_singletons(systems);
    texture_manager::enforce_singletons(systems);
  }

  // external plugins
  {
    input::register_update_systems(systems);
    window_manager::register_update_systems(systems);
  }

  // Fixed update
  {
    systems.register_fixed_update_system(std::make_unique<VelFromInput>());
    systems.register_fixed_update_system(
        std::make_unique<ProcessBoostRequests>());
    systems.register_fixed_update_system(std::make_unique<BoostDecay>());
    systems.register_fixed_update_system(std::make_unique<Move>());
  }

  // normal update
  {
    bool create_startup = true;
    systems.register_update_system([&](float) {
      if (create_startup) {
        make_player(0);
        // TODO id love to have this but its hard to read the UI
        // because the racing lines and stuff go over it
        // MapManager::get().create_map();
        make_ai();
        make_ai();

        create_startup = false;
      }
    });
    systems.register_update_system(std::make_unique<AISetActiveMode>());
    systems.register_update_system(std::make_unique<AIUpdateAIParamsSystem>());
    systems.register_update_system(std::make_unique<Shoot>());
    systems.register_update_system(std::make_unique<MatchKartsToPlayers>());
    systems.register_update_system(std::make_unique<ProcessDamage>());
    systems.register_update_system(
        std::make_unique<ProcessCollisionAbsorption>());
    systems.register_update_system(std::make_unique<ProcessDeath>());
    systems.register_update_system(std::make_unique<SkidMarks>());
    systems.register_update_system(std::make_unique<UpdateCollidingEntities>());
    systems.register_update_system(std::make_unique<WrapAroundTransform>());
    systems.register_update_system(
        std::make_unique<UpdateColorBasedOnEntityID>());
    systems.register_update_system(std::make_unique<AITargetSelection>());
    systems.register_update_system(std::make_unique<AIVelocity>());
    systems.register_update_system(std::make_unique<AIShoot>());
    systems.register_update_system(std::make_unique<WeaponCooldownSystem>());
    systems.register_update_system(std::make_unique<WeaponFireSystem>());
    systems.register_update_system(std::make_unique<ProjectileSpawnSystem>());
    systems.register_update_system(std::make_unique<WeaponRecoilSystem>());
    systems.register_update_system(std::make_unique<WeaponSoundSystem>());
    systems.register_update_system(
        std::make_unique<WeaponFiredCleanupSystem>());
    systems.register_update_system(std::make_unique<DrainLife>());
    systems.register_update_system(std::make_unique<UpdateTrackingEntities>());
    systems.register_update_system(std::make_unique<CheckLivesWinCondition>());
    systems.register_update_system(std::make_unique<CheckKillsWinCondition>());
    systems.register_update_system(std::make_unique<ProcessHippoCollection>());
    systems.register_update_system(std::make_unique<SpawnHippoItems>());
    systems.register_update_system(std::make_unique<CheckHippoWinCondition>());
    systems.register_update_system(std::make_unique<InitializeTagAndGoGame>());
    systems.register_update_system(std::make_unique<UpdateTagAndGoTimers>());
    systems.register_update_system(std::make_unique<UpdateRoundCountdown>());
    systems.register_update_system(
        std::make_unique<HandleTagAndGoTagTransfer>());
    systems.register_update_system(
        std::make_unique<CheckTagAndGoWinCondition>());
    systems.register_update_system(std::make_unique<ScaleTaggerSize>());

    systems.register_update_system(std::make_unique<UpdateSpriteTransform>());
    systems.register_update_system(std::make_unique<UpdateShaderValues>());
    systems.register_update_system(
        std::make_unique<UpdateAnimationTransform>());
    systems.register_update_system(std::make_unique<MarkEntitiesWithShaders>());
    systems.register_update_system(std::make_unique<ApplyWinnerShader>());
    texture_manager::register_update_systems(systems);

    // Initialize map previews
    systems.register_update_system([](float) {
      static bool initialized = false;
      if (!initialized) {
        MapManager::get().initialize_preview_textures();
        initialized = true;
      }
    });

    ui::register_before_ui_updates<InputAction>(systems);
    {
      afterhours::animation::register_update_systems<UIKey>(systems);
#if __APPLE__
      afterhours::animation::register_update_systems<
          afterhours::animation::CompositeKey>(systems);
#endif
      systems.register_update_system(
          std::make_unique<SetupGameStylingDefaults>());
#if __APPLE__
      systems.register_update_system(
      std::make_unique<ui_game::UpdateUIButtonWiggle<InputAction>>());
      systems.register_update_system(
      std::make_unique<ui_game::UpdateUISlideIn<InputAction>>());
      systems.register_update_system(std::make_unique<NavigationSystem>());
      systems.register_update_system(std::make_unique<ScheduleMainMenuUI>());
      systems.register_update_system(std::make_unique<ScheduleDebugUI>());
      systems.register_update_system(std::make_unique<SchedulePauseUI>());
#endif 
    }
    ui::register_after_ui_updates<InputAction>(systems);

    systems.register_update_system(std::make_unique<BackgroundMusic>());

#if __APPLE__
    systems.register_update_system(std::make_unique<UISoundBindingSystem>());
    systems.register_update_system(std::make_unique<SoundPlaybackSystem>());
    systems.register_update_system(std::make_unique<UIClickSounds>());
    systems.register_update_system(std::make_unique<UpdateRenderTexture>());
#endif
    systems.register_update_system(std::make_unique<MarkEntitiesWithShaders>());
  }

  // renders
  {
    systems.register_render_system([&](float) {
      // pass 1: render world into mainRT
      raylib::BeginTextureMode(mainRT);
      raylib::ClearBackground(raylib::DARKGRAY);
    });

    {
      systems.register_render_system(std::make_unique<RenderSkid>());
      systems.register_render_system(std::make_unique<RenderEntities>());
      texture_manager::register_render_systems(systems);
      systems.register_render_system(
          std::make_unique<RenderSpritesWithShaders>());
      systems.register_render_system(
          std::make_unique<RenderAnimationsWithShaders>());
      //
      systems.register_render_system(std::make_unique<RenderPlayerHUD>());
      systems.register_render_system(std::make_unique<RenderLabels>());
      systems.register_render_system(std::make_unique<RenderWeaponCooldown>());
      systems.register_render_system(std::make_unique<RenderOOB>());
      systems.register_render_system(std::make_unique<CarRumble>());
      // (UI moved to pass 2 so it is after tag shader)
    }
    systems.register_render_system([&](float) { raylib::EndTextureMode(); });
    // pass 2: render mainRT with tag shader into screenRT, then draw UI into
    // screenRT
    systems.register_render_system(
        std::make_unique<ConfigureTaggerSpotlight>());
    systems.register_render_system([&](float) {
      raylib::BeginTextureMode(screenRT);
      raylib::ClearBackground(raylib::BLANK);
      // bind tag shader if available; spotlight enabled is controlled via
      // uniforms
      bool useTagShader =
          ShaderLibrary::get().contains(ShaderType::post_processing_tag);
      if (useTagShader) {
        const auto &shader =
            ShaderLibrary::get().get(ShaderType::post_processing_tag);
        raylib::BeginShaderMode(shader);
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
      // draw mainRT into screenRT (1:1, same size)
      const raylib::Rectangle src{0.0f, 0.0f, (float)mainRT.texture.width,
                                  -(float)mainRT.texture.height};
      const raylib::Rectangle dst{0.0f, 0.0f, (float)screenRT.texture.width,
                                  (float)screenRT.texture.height};
      raylib::DrawTexturePro(mainRT.texture, src, dst, {0.0f, 0.0f}, 0.0f,
                             raylib::WHITE);
      if (useTagShader) {
        raylib::EndShaderMode();
      }
    });
    // render UI into screenRT (still in texture mode)
    ui::register_render_systems<InputAction>(systems,
                                             InputAction::ToggleUILayoutDebug);
    systems.register_render_system([&](float) { raylib::EndTextureMode(); });
    // pass 3: draw to screen with base post-processing shader
    systems.register_render_system([&](float) { raylib::BeginDrawing(); });
    systems.register_render_system([&](float) {
      if (ShaderLibrary::get().contains(ShaderType::post_processing)) {
        const auto &shader =
            ShaderLibrary::get().get(ShaderType::post_processing);
        raylib::BeginShaderMode(shader);
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
    });
    systems.register_render_system([&](float) {
      const int window_w = raylib::GetScreenWidth();
      const int window_h = raylib::GetScreenHeight();
      const int content_w = screenRT.texture.width;
      const int content_h = screenRT.texture.height;
      const LetterboxLayout layout =
          compute_letterbox_layout(window_w, window_h, content_w, content_h);
      const raylib::Rectangle src{0.0f, 0.0f, (float)screenRT.texture.width,
                                  -(float)screenRT.texture.height};
      raylib::DrawTexturePro(screenRT.texture, src, layout.dst, {0.0f, 0.0f},
                             0.0f, raylib::WHITE);
    });
    systems.register_render_system([&](float) { raylib::EndShaderMode(); });
    systems.register_render_system(std::make_unique<RenderLetterboxBars>());
    systems.register_render_system(std::make_unique<RenderRoundTimer>());
    systems.register_render_system(std::make_unique<RenderFPS>());
    systems.register_render_system(std::make_unique<RenderDebugWindowInfo>());

    systems.register_render_system([&](float) { raylib::EndDrawing(); });
    //
  }

  while (running && !raylib::WindowShouldClose()) {
    systems.run(raylib::GetFrameTime());
  }

  std::cout << "Num entities: " << EntityHelper::get_entities().size()
            << std::endl;
}

int main(int argc, char *argv[]) {

  // if nothing else ends up using this, we should move into preload.cpp
  argh::parser cmdl(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);

  int screenWidth, screenHeight;
  cmdl({"-w", "--width"}, 1280) >> screenWidth;
  cmdl({"-h", "--height"}, 720) >> screenHeight;

  // Load savefile first
  Settings::get().load_save_file(screenWidth, screenHeight);

  Preload::get() //
      .init("Cart Chaos")
      .make_singleton();
  Settings::get().refresh_settings();

  if (cmdl[{"-i", "--show-intro"}]) {
    intro();
  }

  game();

  Settings::get().write_save_file();

  return 0;
}
