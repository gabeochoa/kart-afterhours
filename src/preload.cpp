
#include "preload.h"

#include "rl.h"

#include "sound_library.h"

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
  afterhours::input::set_gamepad_mappings(buffer.str().c_str());
}

Preload::Preload() {}

void Preload::init(int width, int height, const char *title) {
  raylib::InitWindow(width, height, title);
  raylib::TraceLogLevel logLevel = raylib::LOG_WARNING;
  raylib::SetTraceLogLevel(logLevel);
  raylib::SetTargetFPS(200);
  raylib::SetWindowState(raylib::FLAG_WINDOW_RESIZABLE);
  raylib::InitAudioDevice();
  raylib::SetMasterVolume(1.f);

  load_gamepad_mappings();
  load_sounds();
}
Preload::~Preload() {}
