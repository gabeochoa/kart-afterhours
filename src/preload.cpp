#include "preload.h"

#include <iostream>
#include <sstream>
#include <vector>

#include "log.h"
#include "rl.h"

#include "./ui/navigation.h"
#include "font_info.h"
#include "settings.h"

#include "library/music_library.h"
#include "library/shader_library.h"
#include "library/sound_library.h"
#include "library/texture_library.h"
#include "library/control_atlas.h"
#include "translation_manager.h"
#include <afterhours/src/plugins/camera.h>
#include <afterhours/src/plugins/files.h>
#include <afterhours/src/graphics.h>

using namespace afterhours;
// for HasTexture
#include "components.h"

std::string get_font_name(FontID id) {
  switch (id) {
  case FontID::English:
    return "PxPlus_IBM_BIOS-2y.ttf";
  case FontID::Korean:
    return "NotoSansMonoCJKkr-Bold.otf";
  case FontID::Japanese:
    return "NotoSansMonoCJKjp-Bold.otf";
  case FontID::raylibFont:
    return ui::UIComponent::DEFAULT_FONT;
  case FontID::SYMBOL_FONT:
    return "NotoSansMonoCJKkr-Bold.otf";
  }
  return ui::UIComponent::DEFAULT_FONT;
}

static void load_gamepad_mappings() {
  std::ifstream ifs(
      files::get_resource_path("", "gamecontrollerdb.txt").string().c_str());
  if (!ifs.is_open()) {
    std::cout << "Failed to load game controller db" << std::endl;
    return;
  }
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  input::set_gamepad_mappings(buffer.str().c_str());
}

Preload::Preload() {}

Preload &Preload::init(const char *title, afterhours::graphics::DisplayMode mode) {

  int width = Settings::get_screen_width();
  int height = Settings::get_screen_height();

  // Configure graphics module
  afterhours::graphics::Config cfg;
  cfg.display = mode;
  cfg.width = width;
  cfg.height = height;
  cfg.title = title;
  cfg.target_fps = 200;
  cfg.time_scale = 10.0f;  // 10x speed for headless

  afterhours::graphics::init(cfg);

  if (mode == afterhours::graphics::DisplayMode::Windowed) {
    // Audio only in windowed mode
    raylib::SetAudioStreamBufferSizeDefault(4096);
    raylib::InitAudioDevice();
    if (!raylib::IsAudioDeviceReady()) {
      log_warn("audio device not ready; continuing without audio");
    }
    raylib::SetMasterVolume(1.f);

    // Disable default escape key exit behavior so we can handle it manually
    // Only relevant in windowed mode
    raylib::SetExitKey(0);

    // Sound and music only in windowed mode
    load_gamepad_mappings();
    load_sounds();
    MusicLibrary::get().load(
        files::get_resource_path("sounds", "replace/cobolt.mp3").string().c_str(),
        "menu_music");
  }

  ShaderLibrary::get().load(
      files::get_resource_path("shaders", "post_processing.fs").string().c_str(),
      "post_processing");

  ShaderLibrary::get().load(
      files::get_resource_path("shaders", "post_processing_tag.fs").string().c_str(),
      "post_processing_tag");

  ShaderLibrary::get().load(
      files::get_resource_path("shaders", "car.fs").string().c_str(), "car");

  ShaderLibrary::get().load(
      files::get_resource_path("shaders", "car.fs").string().c_str(),
      "car_winner");

  ShaderLibrary::get().load(
      files::get_resource_path("shaders", "text_mask.fs").string().c_str(),
      "text_mask");

  // One atlas instead of 332 individual textures. Regenerate after changing
  // the source PNGs with: python3 scripts/pack_atlas.py
  ControlAtlas::get().load(
      files::get_resource_path("images", "controls_atlas.png").string(),
      files::get_resource_path("images", "controls_atlas.json").string());

  TextureLibrary::get().load(
      files::get_resource_path("images", "dollar_sign.png").string().c_str(),
      "dollar_sign");
  TextureLibrary::get().load(
      files::get_resource_path("images", "trashcan.png").string().c_str(),
      "trashcan");

  return *this;
}

raylib::Font load_font_for_mode(const char *filename, int fontSize = 32) {
  if (afterhours::graphics::is_headless()) {
    raylib::Font font = {0};
    int dataSize = 0;
    unsigned char *fontData = raylib::LoadFileData(filename, &dataSize);
    if (!fontData || dataSize <= 0) {
      log_warn("failed to load font file in headless: {}", filename);
      return font;
    }
    font.baseSize = fontSize;
    font.glyphCount = 95;
    font.glyphs = raylib::LoadFontData(fontData, dataSize, fontSize, nullptr, 0, raylib::FONT_DEFAULT);
    raylib::Image atlas = raylib::GenImageFontAtlas(
        font.glyphs, &font.recs, font.glyphCount, fontSize, 1, 0);
    font.texture = raylib::LoadTextureFromImage(atlas);
    raylib::SetTextureFilter(font.texture, raylib::TEXTURE_FILTER_BILINEAR);
    raylib::UnloadImage(atlas);
    raylib::UnloadFileData(fontData);
    return font;
  }
  return afterhours::load_font_from_file(filename);
}

void setup_fonts() {
  auto *font_manager = EntityHelper::get_singleton_cmp<ui::FontManager>();
  if (!font_manager) return;

  std::string eng_path = files::get_resource_path("", get_font_name(FontID::English)).string();
  font_manager->load_font(get_font_name(FontID::English), load_font_for_mode(eng_path.c_str()));

  if (!afterhours::graphics::is_headless()) {
    std::string font_file =
        files::get_resource_path("", get_font_name(FontID::Korean)).string();
    translation_manager::TranslationPlugin::load_cjk_fonts(
        *font_manager, font_file, get_font_name,
        translation_manager::get_font_for_language_mapper);
  }

  std::string sym_path = files::get_resource_path("", get_font_name(FontID::SYMBOL_FONT)).string();
  font_manager->load_font(ui::UIComponent::SYMBOL_FONT, load_font_for_mode(sym_path.c_str()));
}

Preload &Preload::make_singleton() {
  // sophie
  auto &sophie = EntityHelper::createEntity();
  {
    input::add_singleton_components(sophie, get_mapping());
    window_manager::add_singleton_components(sophie, 200);
    translation_manager::TranslationPlugin::add_singleton_components(
        sophie, translation_manager::get_translation_data(),
        Settings::get_language(), translation_manager::translation_param);
    translation_manager::set_language(Settings::get_language());

    texture_manager::add_singleton_components(
        sophie, raylib::LoadTexture(
                    files::get_resource_path("images", "spritesheet.png").string().c_str()));

    ui::init_ui_plugin<InputAction>();

    if (afterhours::graphics::is_headless()) {
      auto *fm = EntityHelper::get_singleton_cmp<ui::FontManager>();
      if (fm) {
        std::string eng_path = files::get_resource_path("", get_font_name(FontID::English)).string();
        raylib::Font fallback = load_font_for_mode(eng_path.c_str());
        fm->load_font(ui::UIComponent::DEFAULT_FONT, fallback);
        fm->load_font(ui::UIComponent::UNSET_FONT, fallback);
      }
    }

    setup_fonts();

    sophie.addComponent<ManagesAvailableColors>();
    EntityHelper::registerSingleton<ManagesAvailableColors>(sophie);

    // Navigation stack singleton for consistent UI navigation
    sophie.addComponent<MenuNavigationStack>();
    EntityHelper::registerSingleton<MenuNavigationStack>(sophie);
  }
  {
    // Audio emitter singleton for centralized sound requests
    auto &audio = EntityHelper::createEntity();
    sound_system::add_singleton_components(audio);
  }
  {
    // Camera singleton for game world rendering
    auto &camera = EntityHelper::createEntity();
    camera::add_singleton_components(camera);
  }
  return *this;
}

Preload::~Preload() {
  if (!afterhours::graphics::is_headless() && raylib::IsAudioDeviceReady()) {
    raylib::CloseAudioDevice();
  }
  afterhours::graphics::shutdown();
}
