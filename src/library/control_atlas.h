#pragma once

#include <afterhours/src/singleton.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>

#include "../rl.h"
#include "texture_library.h"

// The 332 control-prompt PNGs used to be loaded as 332 separate textures at
// startup: 332 file opens, 332 GPU textures, and nothing able to batch with
// anything else. They are all 64x64, so scripts/pack_atlas.py grids them into
// one 2048x2048 sheet plus a name -> rect index.
//
// Look one up with ControlAtlas::get().find("keyboard_space"), which returns
// the source rect to pass to DrawTexturePro against texture().
SINGLETON_FWD(ControlAtlas)
struct ControlAtlas {
  SINGLETON(ControlAtlas)

  void load(const std::string &png_path, const std::string &json_path) {
    TextureLibrary::get().load(png_path.c_str(), kTextureName);

    std::ifstream ifs(json_path);
    if (!ifs.is_open()) {
      log_warn("control atlas index missing at {}", json_path);
      return;
    }
    nlohmann::json j;
    ifs >> j;
    for (auto it = j.begin(); it != j.end(); ++it) {
      const auto &r = it.value();
      rects_.emplace(it.key(),
                     raylib::Rectangle{r["x"].get<float>(), r["y"].get<float>(),
                                       r["w"].get<float>(),
                                       r["h"].get<float>()});
    }
  }

  [[nodiscard]] std::optional<raylib::Rectangle>
  find(const std::string &name) const {
    const auto it = rects_.find(name);
    if (it == rects_.end())
      return std::nullopt;
    return it->second;
  }

  [[nodiscard]] const raylib::Texture2D &texture() const {
    return TextureLibrary::get().get(kTextureName);
  }

  [[nodiscard]] size_t size() const { return rects_.size(); }

private:
  static constexpr const char *kTextureName = "controls_atlas";
  std::unordered_map<std::string, raylib::Rectangle> rects_;
};
