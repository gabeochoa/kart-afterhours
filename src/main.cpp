
#ifdef BACKWARD
#include "backward/backward.hpp"
namespace backward {
backward::SignalHandling sh;
} // namespace backward
#endif

#include "game.h"
//
#include "./ui/navigation.h"
#include "argh.h"
#include "map_system.h"
#include "preload.h"
#include "settings.h"
#include "sound_systems.h"
#include "systems.h"
#include "systems_ai.h"
#include "ui/ui_systems.h"
#include <afterhours/src/plugins/animation.h>

// TODO add honking

bool running = true;
// TODO move into library or somethign
raylib::RenderTexture2D mainRT;
raylib::RenderTexture2D screenRT;

using namespace afterhours;

// From intro.cpp
void intro();

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
    systems.register_update_system(std::make_unique<CheckLivesWinFFA>());
    systems.register_update_system(std::make_unique<CheckLivesWinTeam>());
    systems.register_update_system(std::make_unique<CheckKillsWinFFA>());
    systems.register_update_system(std::make_unique<CheckKillsWinTeam>());
    systems.register_update_system(std::make_unique<CheckHippoWinFFA>());
    systems.register_update_system(std::make_unique<CheckHippoWinTeam>());
    systems.register_update_system(std::make_unique<CheckTagAndGoWinFFA>());
    systems.register_update_system(std::make_unique<CheckTagAndGoWinTeam>());

    systems.register_update_system(std::make_unique<ProcessHippoCollection>());
    systems.register_update_system(std::make_unique<SpawnHippoItems>());
    systems.register_update_system(std::make_unique<InitializeTagAndGoGame>());
    systems.register_update_system(std::make_unique<UpdateTagAndGoTimers>());
    systems.register_update_system(std::make_unique<UpdateRoundCountdown>());
    systems.register_update_system(
        std::make_unique<HandleTagAndGoTagTransfer>());
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

    register_ui_systems(systems);
    register_sound_systems(systems);

    systems.register_update_system(std::make_unique<UpdateRenderTexture>());
    systems.register_update_system(std::make_unique<MarkEntitiesWithShaders>());

    // renders
    {
      systems.register_render_system(std::make_unique<BeginWorldRender>());

      {
        systems.register_render_system(std::make_unique<BeginCameraMode>());
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
        systems.register_render_system(
            std::make_unique<RenderWeaponCooldown>());
        systems.register_render_system(std::make_unique<RenderOOB>());
        systems.register_render_system(std::make_unique<EndCameraMode>());
        // (UI moved to pass 2 so it is after tag shader)
      }
      systems.register_render_system(std::make_unique<EndWorldRender>());
      // pass 2: render mainRT with tag shader into screenRT, then draw UI into
      // screenRT
      systems.register_render_system(
          std::make_unique<ConfigureTaggerSpotlight>());
      systems.register_render_system(std::make_unique<BeginTagShaderRender>());
      // render UI into screenRT (still in texture mode)
      systems.register_render_system(std::make_unique<RenderWeaponHUD>());
      ui::register_render_systems<InputAction>(
          systems, InputAction::ToggleUILayoutDebug);
      systems.register_render_system(std::make_unique<EndTagShaderRender>());
      // pass 3: draw to screen with base post-processing shader
      systems.register_render_system(
          std::make_unique<BeginPostProcessingRender>());
      systems.register_render_system(
          std::make_unique<SetupPostProcessingShader>());

      systems.register_render_system(std::make_unique<RenderScreenToWindow>());
      systems.register_render_system(
          std::make_unique<EndPostProcessingShader>());
      systems.register_render_system(std::make_unique<RenderLetterboxBars>());
      systems.register_render_system(std::make_unique<RenderRoundTimer>());
      systems.register_render_system(std::make_unique<RenderFPS>());
      systems.register_render_system(std::make_unique<RenderDebugWindowInfo>());

      systems.register_render_system(std::make_unique<EndDrawing>());
      //
    }

    while (running && !raylib::WindowShouldClose()) {
      systems.run(raylib::GetFrameTime());
    }

    std::cout << "Num entities: " << EntityHelper::get_entities().size()
              << std::endl;
  }
}

int main(int argc, char *argv[]) {
  // Debug: Show Transform component type ID
  log_info("Transform component type ID is {}",
           components::get_type_id<Transform>());

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
