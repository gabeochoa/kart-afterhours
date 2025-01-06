
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
#include "sound_library.h"
//
bool running = true;
//
//
#include "intro.h"

static int next_id = 0;
const vec2 button_size = vec2{100, 50};

struct ConfigurableValues {

  template <typename T> struct ValueInRange {
    T data;
    T min;
    T max;

    ValueInRange(T default_, T min_, T max_)
        : data(default_), min(min_), max(max_) {}

    void operator=(ValueInRange<T> &new_value) { set(new_value.data); }

    void set(T &nv) { data = std::min(max, std::max(min, nv)); }

    void set_pct(const float &pct) {
      // lerp
      T nv = min + pct * (max - min);
      set(nv);
    }

    float get_pct() const { return (data - min) / (max - min); }

    operator T() { return data; }

    operator T() const { return data; }
    operator T &() { return data; }
  };

  ValueInRange<float> max_speed{10.f, 1.f, 20.f};
  ValueInRange<float> skid_threshold{98.5f, 0.f, 100.f};
  ValueInRange<float> steering_sensitivity{2.f, 1.f, 10.f};
};

static ConfigurableValues config;

//
using namespace afterhours;

#include "math_util.h"
#include "resources.h"
#include "utils.h"

#include "input_mapping.h"

#include "components.h"
#include "makers.h"
#include "query.h"
#include "systems.h"
#include "ui_systems.h"

// TODO this needs to be converted into a component with SoundAlias's
// in raylib a Sound can only be played once and hitting play again
// will restart it.
//
// we need to make aliases so that each one can play at the same time
struct CarRumble : System<Transform, CanShoot> {
  virtual void for_each_with(const Entity &, const Transform &,
                             const CanShoot &, float) const override {
    SoundLibrary::get().play(SoundFile::Rumble);
  }
};

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

  raylib::CloseAudioDevice();
  raylib::CloseWindow();

  std::cout << "Num entities: " << EntityHelper::get_entities().size()
            << std::endl;
}

int main(int argc, char *argv[]) {
  int screenWidth = 1280;
  int screenHeight = 720;

  argh::parser cmdl(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);

  cmdl({"-w", "--width"}) >> screenWidth;
  cmdl({"-h", "--height"}) >> screenHeight;

  Preload::get().init(screenWidth, screenHeight, "Cart Chaos");
  Preload::get().make_singleton();
  Settings::get().refresh_settings();

  make_player(0);
  make_ai();

  // intro();
  game();

  return 0;
}
