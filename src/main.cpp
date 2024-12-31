
#include "std_include.h"

#include "argh.h"
#include "rl.h"
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

#define ENABLE_SOUNDS

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

#include "math.h"
#include "resources.h"
#include "utils.h"

struct RenderFPS : System<window_manager::ProvidesCurrentResolution> {
  virtual ~RenderFPS() {}
  virtual void for_each_with(
      const Entity &,
      const window_manager::ProvidesCurrentResolution &pCurrentResolution,
      float) const override {
    raylib::DrawFPS((int)(pCurrentResolution.width() - 80), 0);
  }
};
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
  ValueUp,
  ValueDown,
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

  mapping[InputAction::WidgetNext] = {
      raylib::KEY_TAB,
  };

  mapping[InputAction::WidgetPress] = {
      raylib::KEY_ENTER,
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

#ifdef ENABLE_SOUNDS
enum struct SoundFile {
  Rumble,
  Skrt_Start,
  Skrt_Mid,
  Skrt_End,
};

std::map<SoundFile, raylib::Sound> sounds;

static void load_sounds() {
  magic_enum::enum_for_each<SoundFile>([](auto val) {
    constexpr SoundFile file = val;
    std::string filename;
    switch (file) {
    case SoundFile::Rumble:
      filename = "rumble.wav";
      break;
    case SoundFile::Skrt_Start:
      filename = "skrt_start.wav";
      break;
    case SoundFile::Skrt_Mid:
      filename = "skrt_mid.wav";
      break;
    case SoundFile::Skrt_End:
      filename = "skrt_end.wav";
      break;
    }
    raylib::Sound sound = raylib::LoadSound(GetAssetPath(filename.c_str()));
    sounds[file] = sound;
  });
}

static void play_sound(SoundFile sf) {
  raylib::SetSoundVolume(sounds[sf], 0.25f);
  raylib::PlaySound(sounds[sf]);
}
#endif

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

#ifdef ENABLE_SOUNDS
// For lack of a better filter
struct CarRumble : System<Transform, CanShoot> {
  virtual void for_each_with(const Entity &, const Transform &,
                             const CanShoot &, float) const override {
    play_sound(SoundFile::Rumble);
  }
};
#endif

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

    ui::register_update_systems<InputAction>(systems);
  }

  // renders
  {
    systems.register_render_system(
        [&](float) { raylib::ClearBackground(raylib::DARKGRAY); });
    systems.register_render_system(std::make_unique<RenderDebugUI>());
    systems.register_render_system(std::make_unique<RenderMainMenuUI>());
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

    // making a root component to attach the UI to
    sophie.addComponent<ui::AutoLayoutRoot>();
    sophie.addComponent<ui::UIComponentDebug>("sophie");
    sophie.addComponent<ui::UIComponent>(sophie.id)
        .set_desired_width(afterhours::ui::screen_pct(1.f))
        .set_desired_height(afterhours::ui::screen_pct(1.f))
        .enable_font(get_font_name(FontID::EQPro), 75.f);

    sophie.get<ui::FontManager>().load_font(
        get_font_name(FontID::EQPro), GetAssetPath("eqprorounded-regular.ttf"));
  }

  make_player(0);

  make_ai();

  // intro();
  game();

  return 0;
}
