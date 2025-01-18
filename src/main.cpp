
#include "std_include.h"

#ifdef BACKWARD
#include "backward/backward.hpp"
namespace backward {
backward::SignalHandling sh;
} // namespace backward
#endif

#include "rl.h"
bool running = true;
//
#include "argh.h"
#include "intro.h"
#include "preload.h"
#include "shader_library.h"
#include "sound_systems.h"
#include "systems.h"
#include "ui_systems.h"

using namespace afterhours;

void game() {
  SystemManager systems;

  // debug systems
  {
    window_manager::enforce_singletons(systems);
    ui::enforce_singletons<InputAction>(systems);
    input::enforce_singletons<InputAction>(systems);
    texture_manager::enforce_singletons(systems);
  }

  // external plugins
  {
    input::register_update_systems<InputAction>(systems);
    window_manager::register_update_systems(systems);
  }

  // Fixed update
  {
    systems.register_fixed_update_system(std::make_unique<VelFromInput>());
    systems.register_fixed_update_system(std::make_unique<Move>());
  }

  // normal update
  {
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
    systems.register_update_system(std::make_unique<AIVelocity>());
    systems.register_update_system(std::make_unique<DrainLife>());
    systems.register_update_system(std::make_unique<UpdateTrackingEntities>());

    systems.register_update_system(std::make_unique<UpdateSpriteTransform>());
    systems.register_update_system(std::make_unique<UpdateShaderValues>());
    systems.register_update_system(
        std::make_unique<UpdateAnimationTransform>());
    texture_manager::register_update_systems(systems);

    ui::register_before_ui_updates<InputAction>(systems);
    {
      systems.register_update_system(std::make_unique<ScheduleMainMenuUI>());
      systems.register_update_system(std::make_unique<ScheduleDebugUI>());
    }
    ui::register_after_ui_updates<InputAction>(systems);
  }

  raylib::RenderTexture2D mainRT;
  mainRT = raylib::LoadRenderTexture(1280, 720);

  // renders
  {
    systems.register_render_system([&](float) {
      raylib::BeginTextureMode(mainRT);
      raylib::ClearBackground(raylib::DARKGRAY);
    });
    {
      ui::register_render_systems<InputAction>(
          systems, InputAction::ToggleUILayoutDebug);
      //
      systems.register_render_system(std::make_unique<RenderSkid>());
      systems.register_render_system(std::make_unique<RenderEntities>());
      texture_manager::register_render_systems(systems);
      //
      systems.register_render_system(std::make_unique<RenderHealthAndLives>());
      systems.register_render_system(std::make_unique<RenderWeaponCooldown>());
      systems.register_render_system(std::make_unique<RenderOOB>());
      systems.register_render_system(std::make_unique<RenderLabels>());
      systems.register_render_system(std::make_unique<CarRumble>());
    }
    systems.register_render_system([&](float) { raylib::EndTextureMode(); });
    systems.register_render_system([&](float) { raylib::BeginDrawing(); });
    {
      systems.register_render_system(
          std::make_unique<BeginShader>("post_processing"));
      systems.register_render_system([&](float) {
        raylib::DrawTextureRec(mainRT.texture, {0, 0, 1280, -720}, {0, 0},
                               raylib::WHITE);
      });
      systems.register_render_system([&](float) { raylib::EndShaderMode(); });
      systems.register_render_system(std::make_unique<RenderFPS>());
    }
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

  Preload::get() //
      .init(screenWidth, screenHeight, "Cart Chaos")
      .make_singleton();
  Settings::get().refresh_settings();

  make_player(0);

  const CollisionConfig rock_collision_config{
      .mass = std::numeric_limits<float>::max(),
      .friction = 1.f,
      .restitution = 0.f};

  make_obstacle({screenWidth * .2f, screenHeight * .2f, 50.f, 50.f},
                raylib::BLACK, rock_collision_config);
  make_obstacle({screenWidth * .2f, screenHeight * .8f, 50.f, 50.f},
                raylib::BLACK, rock_collision_config);
  make_obstacle({screenWidth * .8f, screenHeight * .2f, 50.f, 50.f},
                raylib::BLACK, rock_collision_config);
  make_obstacle({screenWidth * .8f, screenHeight * .8f, 50.f, 50.f},
                raylib::BLACK, rock_collision_config);

  const CollisionConfig ball_collision_config{
      .mass = 100.f, .friction = 0.f, .restitution = .75f};

  make_obstacle(
      {(screenWidth * .5f) - 25.f, (screenHeight * .2f) - 25.f, 50.f, 50.f},
      raylib::WHITE, ball_collision_config);

  if (cmdl[{"-i", "--show-intro"}]) {
    intro();
  }

  game();

  return 0;
}
