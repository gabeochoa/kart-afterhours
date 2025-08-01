#pragma once

#include "library.h"
#include "makers.h"
#include "round_settings.h"
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

  static const std::array<MapConfig, 5> available_maps;
  int selected_map_index = 0;

  MapManager() = default;

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

  void create_map() {
    if (selected_map_index >= 0 &&
        selected_map_index < static_cast<int>(available_maps.size())) {
      available_maps[selected_map_index].create_map_func();
    }
  }

private:
  static void create_arena_map();
  static void create_maze_map();
  static void create_race_map();
  static void create_battle_map();
  static void create_catmouse_map();
};