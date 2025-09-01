#include "preload.h"

#include <iostream>
#include <sstream>
#include <vector>

#include "rl.h"

#include "./ui/navigation.h"
#include "font_info.h"
#include "music_library.h"
#include "settings.h"
#include "shader_library.h"
#include "sound_library.h"
#include "texture_library.h"
#include "translation_manager.h"

using namespace afterhours;
// for HasTexture
#include "components.h"

std::string get_font_name(FontID id) {
  switch (id) {
  case FontID::English:
    // return "eqprorounded-regular.ttf";
    return "NotoSansMonoCJKkr-Bold.otf";
  case FontID::Korean:
    return "NotoSansMonoCJKkr-Bold.otf";
  case FontID::Japanese:
    return "NotoSansMonoCJKjp-Bold.otf";
  case FontID::raylibFont:
    return afterhours::ui::UIComponent::DEFAULT_FONT;
  case FontID::SYMBOL_FONT:
    return "NotoSansMonoCJKkr-Bold.otf";
    // return "eqprorounded-regular.ttf";
  }
  return afterhours::ui::UIComponent::DEFAULT_FONT;
}

static void load_gamepad_mappings() {
  std::ifstream ifs(
      Files::get().fetch_resource_path("", "gamecontrollerdb.txt").c_str());
  if (!ifs.is_open()) {
    std::cout << "Failed to load game controller db" << std::endl;
    return;
  }
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  input::set_gamepad_mappings(buffer.str().c_str());
}

Preload::Preload() {}

Preload &Preload::init(const char *title) {

  int width = Settings::get().get_screen_width();
  int height = Settings::get().get_screen_height();

  // raylib::SetConfigFlags(raylib::FLAG_WINDOW_HIGHDPI);
  raylib::InitWindow(width, height, title);
  raylib::SetWindowSize(width, height);
  // Back to warnings
  raylib::TraceLogLevel logLevel = raylib::LOG_ERROR;
  raylib::SetTraceLogLevel(logLevel);
  raylib::SetTargetFPS(200);
  raylib::SetWindowState(raylib::FLAG_WINDOW_RESIZABLE);

  // Enlarge stream buffer to reduce dropouts on macOS/miniaudio
  raylib::SetAudioStreamBufferSizeDefault(4096);
  raylib::InitAudioDevice();
  if (!raylib::IsAudioDeviceReady()) {
    log_warn("audio device not ready; continuing without audio");
  }
  raylib::SetMasterVolume(1.f);

  // Disable default escape key exit behavior so we can handle it manually
  raylib::SetExitKey(0);

  load_gamepad_mappings();
  load_sounds();
  MusicLibrary::get().load(
      Files::get().fetch_resource_path("sounds", "replace/cobolt.mp3").c_str(),
      "menu_music");

  // TODO add load folder for shaders

  ShaderLibrary::get().load(
      Files::get().fetch_resource_path("shaders", "post_processing.fs").c_str(),
      "post_processing");

  ShaderLibrary::get().load(
      Files::get()
          .fetch_resource_path("shaders", "post_processing_tag.fs")
          .c_str(),
      "post_processing_tag");

  ShaderLibrary::get().load(
      Files::get().fetch_resource_path("shaders", "entity_test.fs").c_str(),
      "entity_test");

  ShaderLibrary::get().load(
      Files::get().fetch_resource_path("shaders", "car.fs").c_str(), "car");

  ShaderLibrary::get().load(
      Files::get().fetch_resource_path("shaders", "car.fs").c_str(),
      "car_winner");

  ShaderLibrary::get().load(
      Files::get().fetch_resource_path("shaders", "entity_enhanced.fs").c_str(),
      "entity_enhanced");

  ShaderLibrary::get().load(
      Files::get().fetch_resource_path("shaders", "text_mask.fs").c_str(),
      "text_mask");

  // TODO how safe is the path combination here esp for mac vs windows
  Files::get().for_resources_in_folder(
      "images", "controls/keyboard_default",
      [](const std::string &name, const std::string &filename) {
        TextureLibrary::get().load(filename.c_str(), name.c_str());
      });

  // TODO how safe is the path combination here esp for mac vs windows
  Files::get().for_resources_in_folder(
      "images", "controls/xbox_default",
      [](const std::string &name, const std::string &filename) {
        TextureLibrary::get().load(filename.c_str(), name.c_str());
      });

  // TODO add to spritesheet
  TextureLibrary::get().load(
      Files::get().fetch_resource_path("images", "dollar_sign.png").c_str(),
      "dollar_sign");
  TextureLibrary::get().load(
      Files::get().fetch_resource_path("images", "trashcan.png").c_str(),
      "trashcan");

  return *this;
}

void setup_fonts(Entity &sophie) {
  sophie.get<ui::FontManager>().load_font(
      get_font_name(FontID::English),
      Files::get()
          .fetch_resource_path("", get_font_name(FontID::English))
          .c_str());

  // Load CJK fonts using our helper function
  auto &font_manager = sophie.get<ui::FontManager>();
  std::string font_file =
      Files::get()
          .fetch_resource_path("", get_font_name(FontID::Korean))
          .c_str();

  translation_manager::TranslationManager::get().load_cjk_fonts(font_manager,
                                                                font_file);

  font_manager.load_font(
      afterhours::ui::UIComponent::SYMBOL_FONT,
      Files::get()
          .fetch_resource_path("", get_font_name(FontID::SYMBOL_FONT))
          .c_str());
}

Preload &Preload::make_singleton() {
  // sophie
  auto &sophie = EntityHelper::createEntity();
  {
    input::add_singleton_components(sophie, get_mapping());
    window_manager::add_singleton_components(sophie, 200);
    ui::add_singleton_components<InputAction>(sophie);

    auto &settings = Settings::get();
    translation_manager::set_language(settings.get_language());

    texture_manager::add_singleton_components(
        sophie, raylib::LoadTexture(
                    Files::get()
                        .fetch_resource_path("images", "spritesheet.png")
                        .c_str()));

    setup_fonts(sophie);
    // making a root component to attach the UI to
    sophie.addComponent<ui::AutoLayoutRoot>();
    sophie.addComponent<ui::UIComponentDebug>("sophie");
    sophie.addComponent<ui::UIComponent>(sophie.id)
        .set_desired_width(afterhours::ui::screen_pct(1.f))
        .set_desired_height(afterhours::ui::screen_pct(1.f))
        .enable_font(get_font_name(FontID::English), 75.f);

    sophie.addComponent<ManagesAvailableColors>();
    EntityHelper::registerSingleton<ManagesAvailableColors>(sophie);

    // Navigation stack singleton for consistent UI navigation
    sophie.addComponent<MenuNavigationStack>();
    EntityHelper::registerSingleton<MenuNavigationStack>(sophie);
  }
  {
    // Audio emitter singleton for centralized sound requests
    auto &audio = EntityHelper::createEntity();
    audio.addComponent<SoundEmitter>();
    EntityHelper::registerSingleton<SoundEmitter>(audio);
  }
  {
    // Camera singleton for game world rendering
    auto &camera = EntityHelper::createEntity();
    camera.addComponent<HasCamera>();
    EntityHelper::registerSingleton<HasCamera>(camera);
  }
  return *this;
}

Preload::~Preload() {
  if (raylib::IsAudioDeviceReady()) {
    // nothing to stop currently
  }
  raylib::CloseAudioDevice();
  raylib::CloseWindow();
}