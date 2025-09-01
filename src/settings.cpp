
#include "settings.h"

#include <algorithm>
#include <memory>
#include <nlohmann/json.hpp>

#include "rl.h"

#include "music_library.h"
#include "round_settings.h"
#include "sound_library.h"

template <typename T> struct ValueHolder {
  using value_type = T;
  T data;

  ValueHolder(const T &initial) : data(initial) {}
  T &get() { return data; }
  const T &get() const { return data; }
  void set(const T &data_) { data = data_; }

  operator const T &() { return get(); }
};

struct Pct : ValueHolder<float> {
  Pct(const float &initial) : ValueHolder(0.f) { set(initial); }
  void set(const float &data_) { data = std::min(1.f, std::max(0.f, data_)); }
  float str() const { return data; }
};

struct S_Data {
  afterhours::window_manager::Resolution resolution = {
      .width = 1280,
      .height = 720,
  };

  Pct master_volume = 0.1f;
  Pct music_volume = 0.1f;
  Pct sfx_volume = 0.1f;

  bool fullscreen_enabled = false;
  bool post_processing_enabled = true;

  translation_manager::Language language =
      translation_manager::Language::English;

  // Round settings
  nlohmann::json round_settings;

  // not serialized
  fs::path loaded_from;
};

void to_json(nlohmann::json &j,
             const afterhours::window_manager::Resolution &resolution) {
  j = nlohmann::json{
      {"width", resolution.width},
      {"height", resolution.height},
  };
}

void from_json(const nlohmann::json &j,
               afterhours::window_manager::Resolution &resolution) {
  j.at("width").get_to(resolution.width);
  j.at("height").get_to(resolution.height);
}

void to_json(nlohmann::json &j, const translation_manager::Language &language) {
  j = magic_enum::enum_name(language);
}

void from_json(const nlohmann::json &j,
               translation_manager::Language &language) {
  if (j.is_string()) {
    auto lang_enum = magic_enum::enum_cast<translation_manager::Language>(
        j.get<std::string>());
    if (lang_enum.has_value()) {
      language = lang_enum.value();
    }
  } else {
    language = translation_manager::Language::English;
  }
}

void to_json(nlohmann::json &j, const S_Data &data) {
  nlohmann::json rez_j;
  to_json(rez_j, data.resolution);
  j["resolution"] = rez_j;
  //
  nlohmann::json audio_j;
  audio_j["master_volume"] = data.master_volume.str();
  audio_j["music_volume"] = data.music_volume.str();
  audio_j["sfx_volume"] = data.sfx_volume.str();
  j["audio"] = audio_j;
  //
  j["fullscreen_enabled"] = data.fullscreen_enabled;
  j["post_processing_enabled"] = data.post_processing_enabled;
  //
  nlohmann::json lang_j;
  to_json(lang_j, data.language);
  j["language"] = lang_j;
  //
  j["round_settings"] = data.round_settings;
}

void from_json(const nlohmann::json &j, S_Data &data) {
  from_json(j.at("resolution"), data.resolution);

  nlohmann::json audio_j = j.at("audio");
  data.master_volume.set(audio_j.at("master_volume"));
  data.music_volume.set(audio_j.at("music_volume"));
  data.sfx_volume.set(audio_j.at("sfx_volume"));

  data.fullscreen_enabled = j.at("fullscreen_enabled");

  if (j.contains("post_processing_enabled")) {
    data.post_processing_enabled = j.at("post_processing_enabled");
  }

  if (j.contains("language")) {
    from_json(j.at("language"), data.language);
  }

  if (j.contains("round_settings")) {
    data.round_settings = j.at("round_settings");
  }
}

// TODO load last used settings
Settings::Settings() { data = new S_Data(); }
Settings::~Settings() { delete data; }

void Settings::reset() {
  delete data;
  data = new S_Data();
  refresh_settings();
}

int Settings::get_screen_width() const { return data->resolution.width; }
int Settings::get_screen_height() const { return data->resolution.height; }
float Settings::get_music_volume() const { return data->music_volume; }
float Settings::get_sfx_volume() const { return data->sfx_volume; }
float Settings::get_master_volume() const { return data->master_volume; }

void Settings::update_resolution(afterhours::window_manager::Resolution rez) {
  data->resolution = rez;
}

void Settings::update_music_volume(float vol) {
  MusicLibrary::get().update_volume(vol);
  data->music_volume = vol;
}
void Settings::update_sfx_volume(float vol) {
  SoundLibrary::get().update_volume(vol);
  data->sfx_volume = vol;
}

void Settings::update_master_volume(float vol) {
  // TODO should these be updated after?
  // because if not, then why are we doing these at all
  update_music_volume(data->music_volume);
  update_sfx_volume(data->sfx_volume);

  raylib::SetMasterVolume(vol);
  data->master_volume = vol;
}

void match_fullscreen_to_setting(bool fs_enabled) {
  if (raylib::IsWindowFullscreen() && fs_enabled)
    return;
  if (!raylib::IsWindowFullscreen() && !fs_enabled)
    return;
  raylib::ToggleFullscreen();
}

void Settings::refresh_settings() {

  // audio settings
  {
    update_music_volume(data->music_volume);
    update_sfx_volume(data->sfx_volume);
    update_master_volume(data->master_volume);
  }

  match_fullscreen_to_setting(data->fullscreen_enabled);
}

void Settings::toggle_fullscreen() {
  data->fullscreen_enabled = !data->fullscreen_enabled;
  raylib::ToggleFullscreen();
}

bool &Settings::get_fullscreen_enabled() { return data->fullscreen_enabled; }

bool &Settings::get_post_processing_enabled() {
  return data->post_processing_enabled;
}

void Settings::toggle_post_processing() {
  data->post_processing_enabled = !data->post_processing_enabled;
}

bool Settings::load_save_file(int width, int height) {

  this->data->resolution.width = width;
  this->data->resolution.height = height;

  auto settings_places = Files::get().relative_settings();

  size_t file_loc = 0;
  std::ifstream ifs;
  while (true) {
    if (file_loc >= settings_places.size()) {
      std::stringstream buffer;
      buffer << "Failed to find settings file (Read): \n";
      for (auto place : settings_places)
        buffer << place << ", \n";
      log_warn("{}", buffer.str());
      return false;
    }

    ifs = std::ifstream(settings_places[file_loc]);
    if (ifs.is_open()) {
      log_info("opened file {}", settings_places[file_loc]);
      break;
    }
    file_loc++;
  }
  data->loaded_from = settings_places[file_loc];

  try {
    const auto settingsJSON = nlohmann::json::parse(
        ifs, nullptr /*parser_callback_t*/, true /*allow_exceptions=*/,
        true /* ignore comments */);

    (*this->data) = settingsJSON;
    this->data->loaded_from = settings_places[file_loc];
    refresh_settings();
    load_round_settings();
    return true;

  } catch (const std::exception &e) {
    log_error("Settings::load_save_file: {} formatted improperly. {}",
              data->loaded_from, e.what());
    return false;
  }
  return false;
}

void Settings::write_save_file() {
  save_round_settings();

  std::ofstream ofs(data->loaded_from);
  if (!ofs.good()) {
    std::cerr << "write_json_config_file error: Couldn't open file "
                 "for writing: "
              << data->loaded_from << std::endl;
    return;
  }

  log_info("Saving to {}", data->loaded_from);

  nlohmann::json settingsJSON = *data;

  ofs << settingsJSON.dump(4);
  ofs.close();
}

void Settings::save_round_settings() {
  data->round_settings = RoundManager::get().to_json();
}

void Settings::load_round_settings() {
  if (!data->round_settings.is_null()) {
    RoundManager::get().from_json(data->round_settings);
  }
}

translation_manager::Language Settings::get_language() const {
  return data->language;
}

void Settings::set_language(translation_manager::Language language) {
  data->language = language;
}
