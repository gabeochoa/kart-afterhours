
#include "preload.h"

#include "rl.h"

#include "input_mapping.h"
#include "shader_library.h"
#include "sound_library.h"
#include "texture_library.h"

using namespace afterhours;
// for HasTexture
#include "components.h"

std::string get_font_name(FontID id) {
  switch (id) {
  case FontID::EQPro:
    return "eqpro";
  case FontID::raylibFont:
    return afterhours::ui::UIComponent::DEFAULT_FONT;
  }
  return afterhours::ui::UIComponent::DEFAULT_FONT;
}

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

Preload::Preload() {}

Preload &Preload::init(int width, int height, const char *title) {
  raylib::SetConfigFlags(raylib::FLAG_WINDOW_HIGHDPI);
  raylib::InitWindow(width, height, title);
  auto scale = raylib::GetWindowScaleDPI();
  if (scale.x != 1.f) {
    width *= scale.x;
    height *= scale.y;
  }
  raylib::SetWindowSize(width, height);
  // TODO this doesnt seem to do anything when in this file
  raylib::TraceLogLevel logLevel = raylib::LOG_WARNING;
  raylib::SetTraceLogLevel(logLevel);
  raylib::SetTargetFPS(200);
  raylib::SetWindowState(raylib::FLAG_WINDOW_RESIZABLE);
  raylib::InitAudioDevice();
  raylib::SetMasterVolume(1.f);

  load_gamepad_mappings();
  load_sounds();

  ShaderLibrary::get().load(GetAssetPath("shaders/post_processing.fs"),
                            "post_processing");

  // TODO load all controls
  TextureLibrary::get().load(
      GetAssetPath("controls/xbox_default/xbox_button_color_a.png"),
      "xbox_button_color_a");

  return *this;
}

Preload &Preload::make_singleton() {
  // sophie
  auto &sophie = EntityHelper::createEntity();
  {
    input::add_singleton_components<InputAction>(sophie, get_mapping());
    window_manager::add_singleton_components(sophie, 200);
    ui::add_singleton_components<InputAction>(sophie);

    texture_manager::add_singleton_components(
        sophie, raylib::LoadTexture(GetAssetPath("spritesheet.png")));

    sophie.get<ui::FontManager>().load_font(
        get_font_name(FontID::EQPro), GetAssetPath("eqprorounded-regular.ttf"));

    // making a root component to attach the UI to
    sophie.addComponent<ui::AutoLayoutRoot>();
    sophie.addComponent<ui::UIComponentDebug>("sophie");
    sophie.addComponent<ui::UIComponent>(sophie.id)
        .set_desired_width(afterhours::ui::screen_pct(1.f))
        .set_desired_height(afterhours::ui::screen_pct(1.f))
        .enable_font(get_font_name(FontID::EQPro), 75.f);

    sophie.addComponent<ManagesAvailableColors>();
    EntityHelper::registerSingleton<ManagesAvailableColors>(sophie);
  }

  return *this;
}

Preload::~Preload() {
  raylib::CloseAudioDevice();
  raylib::CloseWindow();
}
