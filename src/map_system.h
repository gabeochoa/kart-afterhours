#pragma once

#include "makers.h"
#include "rl.h"
#include "round_settings.h"
#include "tags.h"
#include <afterhours/src/library.h>
#include <bitset>
#include <functional>
#include <magic_enum/magic_enum.hpp>

struct MapConfig {
  std::string display_name;
  std::string description;
  std::bitset<magic_enum::enum_count<RoundType>()> compatible_round_types;
  std::function<void()> create_map_func; // Function to create the map
};

SINGLETON_FWD(MapManager)
struct MapManager {
  SINGLETON(MapManager)

  static constexpr int RANDOM_MAP_INDEX = -1;
  static constexpr int MAP_COUNT = 6;
  static const std::array<MapConfig, MAP_COUNT> available_maps;
  int selected_map_index = 0;

  std::array<raylib::RenderTexture2D, MAP_COUNT> preview_textures;
  bool preview_textures_initialized = false;

  MapManager() = default;
  ~MapManager() { cleanup_preview_textures(); }

  std::vector<std::pair<int, MapConfig>>
  get_maps_for_round_type(RoundType round_type) {
    std::vector<std::pair<int, MapConfig>> compatible_maps;
    int round_type_index = static_cast<int>(round_type);
    for (size_t i = 0; i < available_maps.size(); i++) {
      const auto &map = available_maps[i];
      if (map.compatible_round_types.test(round_type_index)) {
        compatible_maps.push_back({static_cast<int>(i), map});
      }
    }
    return compatible_maps;
  }

  void set_selected_map(const int map_index) { selected_map_index = map_index; }

  [[nodiscard]] int get_selected_map() const { return selected_map_index; }

  void cleanup_map_generated_entities() {
    auto map_generated_entities = EntityQuery({.force_merge = true})
                                      .whereHasTag(GameTag::MapGenerated)
                                      .gen();
    for (auto &entity : map_generated_entities) {
      entity.get().cleanup = true;
    }
  }

  void create_map() {
    cleanup_map_generated_entities();

    if (selected_map_index == RANDOM_MAP_INDEX) {
      auto maps =
          get_maps_for_round_type(RoundManager::get().active_round_type);
      if (!maps.empty()) {
        int random_index =
            raylib::GetRandomValue(0, static_cast<int>(maps.size()) - 1);
        selected_map_index = maps[static_cast<size_t>(random_index)].first;
      }
    }

    if (selected_map_index >= 0 &&
        selected_map_index < static_cast<int>(available_maps.size())) {
      available_maps[selected_map_index].create_map_func();
    }
  }

  void initialize_preview_textures();
  void generate_map_preview(int map_index);
  void generate_all_previews();
  [[nodiscard]] const raylib::RenderTexture2D &
  get_preview_texture(int map_index) const;
  void cleanup_preview_textures();
  void cleanup_preview_area(int map_index);

private:
  static void create_arena_map();
  static void create_maze_map();
  static void create_race_map();
  static void create_battle_map();
  static void create_tagandgo_map();
  static void create_test_map();
};
