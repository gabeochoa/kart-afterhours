#include "map_system.h"
#include "components.h"
#include "makers.h"
#include "rl.h"
#include "round_settings.h"

const std::array<MapConfig, 5> MapManager::available_maps = {
    {{.display_name = "Arena",
      .description = "Classic open arena with strategic obstacles",
      .create_map_func = create_arena_map,
      .compatible_round_types = std::bitset<4>(
          0b1111)}, // All round types (Lives, Kills, Score, CatAndMouse)
     {.display_name = "Maze",
      .description = "Complex maze layout for tactical gameplay",
      .create_map_func = create_maze_map,
      .compatible_round_types = std::bitset<4>(0b0011)}, // Lives, Kills
     {.display_name = "Race Track",
      .description = "Race track layout with speed-focused gameplay",
      .create_map_func = create_race_map,
      .compatible_round_types = std::bitset<4>(0b1100)}, // Score, CatAndMouse
     {.display_name = "Battle Arena",
      .description = "Combat-focused layout with cover points",
      .create_map_func = create_battle_map,
      .compatible_round_types = std::bitset<4>(0b0011)}, // Lives, Kills
     {.display_name = "Cat & Mouse",
      .description = "Special layout optimized for tag gameplay",
      .create_map_func = create_catmouse_map,
      .compatible_round_types = std::bitset<4>(0b1000)}}}; // CatAndMouse only

void MapManager::create_arena_map() {
  auto *pcr = EntityHelper::get_singleton_cmp<
      window_manager::ProvidesCurrentResolution>();
  window_manager::Resolution resolution = pcr->current_resolution;

  const auto screen_pct = [resolution](float x, float y, float w, float h) {
    return Rectangle{
        resolution.width * x,
        resolution.height * y,
        w,
        h,
    };
  };

  const CollisionConfig rock_config{
      .mass = std::numeric_limits<float>::max(),
      .friction = 1.f,
      .restitution = 0.f,
  };

  const CollisionConfig ball_config{
      .mass = 100.f,
      .friction = 0.f,
      .restitution = .75f,
  };

  // Corner obstacles
  make_obstacle(screen_pct(0.2f, 0.2f, 50, 50), raylib::BLACK, rock_config);
  make_obstacle(screen_pct(0.2f, 0.8f, 50, 50), raylib::BLACK, rock_config);
  make_obstacle(screen_pct(0.8f, 0.8f, 50, 50), raylib::BLACK, rock_config);
  make_obstacle(screen_pct(0.8f, 0.2f, 50, 50), raylib::BLACK, rock_config);

  // Center obstacles
  make_obstacle(screen_pct(0.5f, 0.2f, 50, 50), raylib::WHITE, ball_config);
  make_obstacle(screen_pct(0.5f, 0.8f, 50, 50), raylib::WHITE, ball_config);
}

void MapManager::create_maze_map() {
  auto *pcr = EntityHelper::get_singleton_cmp<
      window_manager::ProvidesCurrentResolution>();
  window_manager::Resolution resolution = pcr->current_resolution;

  const auto screen_pct = [resolution](float x, float y, float w, float h) {
    return Rectangle{
        resolution.width * x,
        resolution.height * y,
        w,
        h,
    };
  };

  const CollisionConfig wall_config{
      .mass = std::numeric_limits<float>::max(),
      .friction = 1.f,
      .restitution = 0.f,
  };

  // Create maze walls
  // Horizontal walls
  for (int i = 0; i < 5; i++) {
    make_obstacle(screen_pct(0.1f + i * 0.2f, 0.3f, 30, 30), raylib::DARKGRAY,
                  wall_config);
    make_obstacle(screen_pct(0.1f + i * 0.2f, 0.7f, 30, 30), raylib::DARKGRAY,
                  wall_config);
  }

  // Vertical walls
  for (int i = 0; i < 3; i++) {
    make_obstacle(screen_pct(0.3f, 0.1f + i * 0.3f, 30, 30), raylib::DARKGRAY,
                  wall_config);
    make_obstacle(screen_pct(0.7f, 0.1f + i * 0.3f, 30, 30), raylib::DARKGRAY,
                  wall_config);
  }
}

void MapManager::create_race_map() {
  auto *pcr = EntityHelper::get_singleton_cmp<
      window_manager::ProvidesCurrentResolution>();
  window_manager::Resolution resolution = pcr->current_resolution;

  const auto screen_pct = [resolution](float x, float y, float w, float h) {
    return Rectangle{
        resolution.width * x,
        resolution.height * y,
        w,
        h,
    };
  };

  const CollisionConfig barrier_config{
      .mass = std::numeric_limits<float>::max(),
      .friction = 1.f,
      .restitution = 0.f,
  };

  // Create race track barriers
  // Outer track
  for (int i = 0; i < 8; i++) {
    float angle = i * 0.785f; // 45 degrees
    float x = 0.5f + 0.3f * cos(angle);
    float y = 0.5f + 0.3f * sin(angle);
    make_obstacle(screen_pct(x, y, 40, 40), raylib::ORANGE, barrier_config);
  }

  // Inner track
  for (int i = 0; i < 6; i++) {
    float angle = i * 1.047f; // 60 degrees
    float x = 0.5f + 0.15f * cos(angle);
    float y = 0.5f + 0.15f * sin(angle);
    make_obstacle(screen_pct(x, y, 40, 40), raylib::RED, barrier_config);
  }
}

void MapManager::create_battle_map() {
  auto *pcr = EntityHelper::get_singleton_cmp<
      window_manager::ProvidesCurrentResolution>();
  window_manager::Resolution resolution = pcr->current_resolution;

  const auto screen_pct = [resolution](float x, float y, float w, float h) {
    return Rectangle{
        resolution.width * x,
        resolution.height * y,
        w,
        h,
    };
  };

  const CollisionConfig cover_config{
      .mass = std::numeric_limits<float>::max(),
      .friction = 1.f,
      .restitution = 0.f,
  };

  // Create cover points for battle
  // Corner cover
  make_obstacle(screen_pct(0.15f, 0.15f, 35, 35), raylib::BROWN, cover_config);
  make_obstacle(screen_pct(0.85f, 0.15f, 35, 35), raylib::BROWN, cover_config);
  make_obstacle(screen_pct(0.15f, 0.85f, 35, 35), raylib::BROWN, cover_config);
  make_obstacle(screen_pct(0.85f, 0.85f, 35, 35), raylib::BROWN, cover_config);

  // Center cover
  make_obstacle(screen_pct(0.5f, 0.3f, 35, 35), raylib::BROWN, cover_config);
  make_obstacle(screen_pct(0.5f, 0.7f, 35, 35), raylib::BROWN, cover_config);
  make_obstacle(screen_pct(0.3f, 0.5f, 35, 35), raylib::BROWN, cover_config);
  make_obstacle(screen_pct(0.7f, 0.5f, 35, 35), raylib::BROWN, cover_config);
}

void MapManager::create_catmouse_map() {
  auto *pcr = EntityHelper::get_singleton_cmp<
      window_manager::ProvidesCurrentResolution>();
  window_manager::Resolution resolution = pcr->current_resolution;

  const auto screen_pct = [resolution](float x, float y, float w, float h) {
    return Rectangle{
        resolution.width * x,
        resolution.height * y,
        w,
        h,
    };
  };

  const CollisionConfig safe_zone_config{
      .mass = std::numeric_limits<float>::max(),
      .friction = 1.f,
      .restitution = 0.f,
  };

  // Create safe zones and obstacles for cat and mouse
  // Safe zones (smaller, harder to reach)
  make_obstacle(screen_pct(0.1f, 0.1f, 25, 25), raylib::GREEN,
                safe_zone_config);
  make_obstacle(screen_pct(0.9f, 0.1f, 25, 25), raylib::GREEN,
                safe_zone_config);
  make_obstacle(screen_pct(0.1f, 0.9f, 25, 25), raylib::GREEN,
                safe_zone_config);
  make_obstacle(screen_pct(0.9f, 0.9f, 25, 25), raylib::GREEN,
                safe_zone_config);

  // Chase obstacles
  make_obstacle(screen_pct(0.5f, 0.2f, 25, 25), raylib::BLUE, safe_zone_config);
  make_obstacle(screen_pct(0.5f, 0.8f, 25, 25), raylib::BLUE, safe_zone_config);
  make_obstacle(screen_pct(0.2f, 0.5f, 25, 25), raylib::BLUE, safe_zone_config);
  make_obstacle(screen_pct(0.8f, 0.5f, 25, 25), raylib::BLUE, safe_zone_config);
}