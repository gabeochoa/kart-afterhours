
#include "std_include.h"

#ifdef BACKWARD
#include "backward/backward.hpp"
namespace backward {
backward::SignalHandling sh;
} // namespace backward
#endif

#include "argh.h"
#include "rl.h"
#include "sound_library.h"
//
int sophie_id = -1;
bool running = true;
//
//
enum FontID {
  EQPro,
  raylibFont,
};

std::string get_font_name(FontID id) {
  switch (id) {
  case FontID::EQPro:
    return "eqpro";
  case FontID::raylibFont:
    return afterhours::ui::UIComponent::DEFAULT_FONT;
  }
  return afterhours::ui::UIComponent::DEFAULT_FONT;
}
#include "intro.h"

static int next_id = 0;
const vec2 button_size = vec2{100, 50};

// 12 gives us these options:
// 1,2,3,4,6,12
int MAX_HEALTH = 120;

int kill_shots_to_base_dmg(int num_shots) {
  if (!(num_shots == 1 || num_shots == 2 || num_shots == 3 || num_shots == 4 ||
        num_shots == 6 || num_shots == 12)) {
    log_error("You are setting a non divisible number of shots: {}", num_shots);
  }
  return MAX_HEALTH / num_shots;
}

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

enum class InputAction {
  None,
  Accel,
  Left,
  Right,
  Brake,
  ShootLeft,
  ShootRight,
  //
  WidgetNext,
  WidgetPress,
  WidgetMod,
  WidgetBack,
  ToggleUIDebug,
  ToggleUILayoutDebug,
};

using afterhours::input;

auto get_mapping() {
  std::map<InputAction, input::ValidInputs> mapping;

  mapping[InputAction::Accel] = {
      raylib::KEY_UP,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_RIGHT_Y,
          .dir = -1,
      },
  };

  mapping[InputAction::Brake] = {
      raylib::KEY_DOWN,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_RIGHT_Y,
          .dir = 1,
      },
  };

  mapping[InputAction::Left] = {
      raylib::KEY_LEFT,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_LEFT_X,
          .dir = -1,
      },
  };

  mapping[InputAction::Right] = {
      raylib::KEY_RIGHT,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_LEFT_X,
          .dir = 1,
      },
  };

  mapping[InputAction::ShootLeft] = {
      raylib::KEY_Q,
      raylib::GAMEPAD_BUTTON_LEFT_TRIGGER_1,
  };

  mapping[InputAction::ShootRight] = {
      raylib::KEY_E,
      raylib::GAMEPAD_BUTTON_RIGHT_TRIGGER_1,
  };

  mapping[InputAction::WidgetBack] = {
      raylib::GAMEPAD_BUTTON_LEFT_FACE_UP,
      raylib::KEY_UP,
  };

  mapping[InputAction::WidgetNext] = {
      raylib::KEY_TAB,
      raylib::GAMEPAD_BUTTON_LEFT_FACE_DOWN,
      raylib::KEY_DOWN,
  };

  mapping[InputAction::WidgetPress] = {
      raylib::KEY_ENTER,
      raylib::GAMEPAD_BUTTON_RIGHT_FACE_DOWN,
  };

  mapping[InputAction::WidgetMod] = {
      raylib::KEY_LEFT_SHIFT,
  };

  mapping[InputAction::ToggleUIDebug] = {
      raylib::KEY_GRAVE,
  };

  mapping[InputAction::ToggleUILayoutDebug] = {
      raylib::KEY_EQUAL,
  };

  return mapping;
}

#include "components.h"
#include "makers.h"
#include "query.h"
#include "systems.h"
#include "ui_systems.h"

static void load_gamepad_mappings() {
  std::ifstream ifs(GetAssetPath("gamecontrollerdb.txt"));
  if (!ifs.is_open()) {
    std::cout << "Failed to load game controller db" << std::endl;
    return;
  }
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  input::set_gamepad_mappings(buffer.str().c_str());
}

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

  raylib::InitWindow(screenWidth, screenHeight, "kart-afterhours");
  raylib::SetTargetFPS(200);

  raylib::SetWindowState(raylib::FLAG_WINDOW_RESIZABLE);

  raylib::InitAudioDevice();
  raylib::SetMasterVolume(1.f);

  load_gamepad_mappings();

  load_sounds();

  // sophie
  auto &sophie = EntityHelper::createEntity();
  sophie_id = sophie.id;
  {
    input::add_singleton_components<InputAction>(sophie, get_mapping());
    window_manager::add_singleton_components(sophie, 200);
    ui::add_singleton_components<InputAction>(sophie);

    sophie.addComponent<HasTexture>(
        raylib::LoadTexture(GetAssetPath("spritesheet.png")));
    EntityHelper::registerSingleton<HasTexture>(sophie);

    sophie.get<ui::FontManager>().load_font(
        get_font_name(FontID::EQPro), GetAssetPath("eqprorounded-regular.ttf"));

    // making a root component to attach the UI to
    sophie.addComponent<ui::AutoLayoutRoot>();
    sophie.addComponent<ui::UIComponentDebug>("sophie");
    sophie.addComponent<ui::UIComponent>(sophie.id)
        .set_desired_width(afterhours::ui::screen_pct(1.f))
        .set_desired_height(afterhours::ui::screen_pct(1.f))
        .enable_font(get_font_name(FontID::EQPro), 75.f);
  }

  make_player(0);
  make_ai();

  // intro();
  game();

  return 0;
}
