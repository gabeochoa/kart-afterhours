
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
#include "preload.h"
#include "shader_library.h"
#include "sound_systems.h"
#include "systems.h"
#include "ui_systems.h"

bool running = true;
// TODO move into library or somethign
raylib::RenderTexture2D mainRT;

using namespace afterhours;

void make_default_map() {
  make_player(0);

  auto *pcr = EntityHelper::get_singleton_cmp<
      window_manager::ProvidesCurrentResolution>();

  window_manager::Resolution resolution = pcr->current_resolution;

  const auto screen_pct = [resolution](float x, float y) {
    return Rectangle{
        resolution.width * x,
        resolution.height * y,
        50.f,
        50.f,
    };
  };

  const CollisionConfig rock_collision_config{
      .mass = std::numeric_limits<float>::max(),
      .friction = 1.f,
      .restitution = 0.f,
  };

  make_obstacle(screen_pct(0.2f, 0.2f), raylib::BLACK, rock_collision_config);
  make_obstacle(screen_pct(0.2f, 0.8f), raylib::BLACK, rock_collision_config);
  make_obstacle(screen_pct(0.8f, 0.8f), raylib::BLACK, rock_collision_config);
  make_obstacle(screen_pct(0.8f, 0.2f), raylib::BLACK, rock_collision_config);

  const CollisionConfig ball_collision_config{
      .mass = 100.f,
      .friction = 0.f,
      .restitution = .75f,
  };

  make_obstacle(screen_pct(0.5f, 0.2f), raylib::WHITE, ball_collision_config);
}

void game() {
  mainRT = raylib::LoadRenderTexture(Settings::get().get_screen_width(),
                                     Settings::get().get_screen_height());

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
    bool create_map = true;
    systems.register_render_system([&](float) {
      if (create_map) {
        make_default_map();
        create_map = false;
      }
    });
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
    systems.register_update_system(std::make_unique<CheckLivesWinCondition>());
    systems.register_update_system(std::make_unique<CheckKillsWinCondition>());

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
    systems.register_update_system(std::make_unique<UpdateRenderTexture>());
  }

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
      systems.register_render_system(std::make_unique<RenderKills>());
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
      systems.register_render_system(std::make_unique<RenderRenderTexture>());
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
