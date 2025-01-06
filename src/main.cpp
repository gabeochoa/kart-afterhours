
#include "std_include.h"

#ifdef BACKWARD
#include "backward/backward.hpp"
namespace backward {
backward::SignalHandling sh;
} // namespace backward
#endif

#include "argh.h"
#include "preload.h"
#include "rl.h"
//
bool running = true;
//
//
#include "config.h"
#include "intro.h"

static int next_id = 0;
const vec2 button_size = vec2{100, 50};

//
using namespace afterhours;

#include "math_util.h"
#include "resources.h"
#include "utils.h"

#include "input_mapping.h"

#include "components.h"
#include "makers.h"
#include "query.h"
#include "sound_systems.h"
#include "systems.h"
#include "ui_systems.h"

void game() {
  SystemManager systems;

  // debug systems
  {
    window_manager::enforce_singletons(systems);
    ui::enforce_singletons<InputAction>(systems);
    input::enforce_singletons<InputAction>(systems);
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
    systems.register_update_system(std::make_unique<ProcessDeath>());
    systems.register_update_system(std::make_unique<SkidMarks>());
    systems.register_update_system(std::make_unique<UpdateCollidingEntities>());
    systems.register_update_system(std::make_unique<WrapAroundTransform>());
    systems.register_update_system(
        std::make_unique<AnimationUpdateCurrentFrame>());
    systems.register_update_system(
        std::make_unique<UpdateColorBasedOnEntityID>());
    systems.register_update_system(std::make_unique<AIVelocity>());
    systems.register_update_system(std::make_unique<DrainLife>());
    systems.register_update_system(std::make_unique<UpdateTrackingEntities>());

    systems.register_update_system(std::make_unique<ScheduleMainMenuUI>());
    systems.register_update_system(std::make_unique<ScheduleDebugUI>());

    ui::register_update_systems<InputAction>(systems);
  }

  // renders
  {
    systems.register_render_system(
        [&](float) { raylib::ClearBackground(raylib::DARKGRAY); });
    ui::register_render_systems<InputAction>(systems,
                                             InputAction::ToggleUILayoutDebug);
    systems.register_render_system(std::make_unique<RenderSkid>());
    systems.register_render_system(std::make_unique<RenderEntities>());
    systems.register_render_system(std::make_unique<RenderHealthAndLives>());
    systems.register_render_system(std::make_unique<RenderSprites>());
    systems.register_render_system(std::make_unique<RenderAnimation>());
    systems.register_render_system(std::make_unique<RenderWeaponCooldown>());
    systems.register_render_system(std::make_unique<RenderOOB>());
    systems.register_render_system(std::make_unique<CarRumble>());
    //
    systems.register_render_system(std::make_unique<RenderFPS>());
  }

  while (running && !raylib::WindowShouldClose()) {
    raylib::BeginDrawing();
    { systems.run(raylib::GetFrameTime()); }
    raylib::EndDrawing();
  }

  std::cout << "Num entities: " << EntityHelper::get_entities().size()
            << std::endl;
}

int main(int argc, char *argv[]) {
  int screenWidth = 1280;
  int screenHeight = 720;

  // if nothing else ends up using this, we should move into preload.cpp
  argh::parser cmdl(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);

  cmdl({"-w", "--width"}) >> screenWidth;
  cmdl({"-h", "--height"}) >> screenHeight;

  Preload::get() //
      .init(screenWidth, screenHeight, "Cart Chaos")
      .make_singleton();
  Settings::get().refresh_settings();

  make_player(0);
  make_ai();

  // intro();
  game();

  return 0;
}
