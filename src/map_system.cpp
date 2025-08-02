#include "map_system.h"
#include "components.h"
#include "makers.h"
#include "rl.h"
#include "round_settings.h"

const std::array<MapConfig, 5> MapManager::available_maps = {
    {{.display_name = "Arena",
      .description = "Classic open arena with strategic obstacles",
      .create_map_func = create_arena_map,
      .create_preview_map_func = create_arena_map_at_offset,
      .compatible_round_types = std::bitset<4>(
          0b1111)}, // All round types (Lives, Kills, Score, CatAndMouse)
     {.display_name = "Maze",
      .description = "Complex maze layout for tactical gameplay",
      .create_map_func = create_maze_map,
      .create_preview_map_func = create_maze_map_at_offset,
      .compatible_round_types = std::bitset<4>(0b0011)}, // Lives, Kills
     {.display_name = "Race Track",
      .description = "Race track layout with speed-focused gameplay",
      .create_map_func = create_race_map,
      .create_preview_map_func = create_race_map_at_offset,
      .compatible_round_types = std::bitset<4>(0b1100)}, // Score, CatAndMouse
     {.display_name = "Battle Arena",
      .description = "Combat-focused layout with cover points",
      .create_map_func = create_battle_map,
      .create_preview_map_func = create_battle_map_at_offset,
      .compatible_round_types = std::bitset<4>(0b0011)}, // Lives, Kills
     {.display_name = "Cat & Mouse",
      .description = "Special layout optimized for tag gameplay",
      .create_map_func = create_catmouse_map,
      .create_preview_map_func = create_catmouse_map_at_offset,
      .compatible_round_types = std::bitset<4>(0b1000)}}}; // CatAndMouse only

// Preview texture management
void MapManager::initialize_preview_textures() {
  if (preview_textures_initialized) return;
  
  const int preview_width = 300;
  const int preview_height = 300; // Changed from 200 to 300 for square preview
  
  for (size_t i = 0; i < preview_textures.size(); i++) {
    preview_textures[i] = raylib::LoadRenderTexture(preview_width, preview_height);
  }
  
  preview_textures_initialized = true;
  generate_all_previews();
}

void MapManager::generate_map_preview(int map_index) {
  if (!preview_textures_initialized || map_index < 0 || map_index >= 5) return;
  
  // Create preview entities at a completely isolated location
  vec2 preview_offset = {100000.0f, 100000.0f}; // Very far from any game activity
  available_maps[map_index].create_preview_map_func(preview_offset);
  
  // Calculate proper camera setup for 300x300 square texture showing 800x600 preview area
  raylib::Camera2D camera = {};
  
  // Calculate zoom to fit 800x600 area into 300x300 square texture
  float zoom_x = 300.0f / 800.0f; // 0.375
  float zoom_y = 300.0f / 600.0f; // 0.5
  camera.zoom = std::min(zoom_x, zoom_y) * 0.8f; // Use smaller zoom with margin, about 0.3
  
  camera.offset = {150.0f, 150.0f}; // Center of 300x300 square preview texture
  
  // Target the center of the preview map area
  vec2 preview_center = {preview_offset.x + 400.0f, preview_offset.y + 300.0f};
  camera.target = preview_center;
  
  raylib::BeginTextureMode(preview_textures[map_index]);
  raylib::ClearBackground(raylib::DARKGRAY);
  
  raylib::BeginMode2D(camera);
  
  // Get and render ONLY the preview entities we just created
  auto preview_entities = EntityQuery({.force_merge = true})
                              .whereHasComponent<MapPreviewGenerated>()
                              .gen();
  
  for (auto &entity : preview_entities) {
    if (entity.get().has<Transform>() && entity.get().has<HasColor>()) {
      auto &transform = entity.get().get<Transform>();
      auto &color = entity.get().get<HasColor>();
      
      // Draw rectangles using position directly (not centered)
      raylib::DrawRectangle(
          static_cast<int>(transform.position.x),
          static_cast<int>(transform.position.y),
          static_cast<int>(transform.size.x),
          static_cast<int>(transform.size.y),
          color.color());
    }
  }
  
  raylib::EndMode2D();
  raylib::EndTextureMode();
  
  // Immediately clean up preview entities to prevent them from appearing in main game
  cleanup_preview_entities();
}

void MapManager::generate_all_previews() {
  for (int i = 0; i < 5; i++) {
    generate_map_preview(i);
  }
}

const raylib::RenderTexture2D& MapManager::get_preview_texture(int map_index) const {
  return preview_textures[map_index];
}

void MapManager::cleanup_preview_textures() {
  if (!preview_textures_initialized) return;
  
  for (auto& texture : preview_textures) {
    raylib::UnloadRenderTexture(texture);
  }
  preview_textures_initialized = false;
}

void MapManager::cleanup_preview_entities() {
  auto preview_entities = EntityQuery({.force_merge = true})
                              .whereHasComponent<MapPreviewGenerated>()
                              .gen();
  for (auto &entity : preview_entities) {
    entity.get().cleanup = true;
  }
}

// Offset-based map creation functions for previews
void MapManager::create_arena_map_at_offset(vec2 offset) {
  // Use a fixed resolution for preview consistency
  const float preview_width = 800.0f;
  const float preview_height = 600.0f;

  const auto screen_pct = [preview_width, preview_height](float x, float y, float w, float h) {
    return Rectangle{
        preview_width * x,
        preview_height * y,
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
  make_preview_obstacle(screen_pct(0.2f, 0.2f, 50, 50), raylib::BLACK, rock_config, offset);
  make_preview_obstacle(screen_pct(0.2f, 0.8f, 50, 50), raylib::BLACK, rock_config, offset);
  make_preview_obstacle(screen_pct(0.8f, 0.8f, 50, 50), raylib::BLACK, rock_config, offset);
  make_preview_obstacle(screen_pct(0.8f, 0.2f, 50, 50), raylib::BLACK, rock_config, offset);

  // Center obstacles
  make_preview_obstacle(screen_pct(0.5f, 0.2f, 50, 50), raylib::WHITE, ball_config, offset);
  make_preview_obstacle(screen_pct(0.5f, 0.8f, 50, 50), raylib::WHITE, ball_config, offset);
}

void MapManager::create_maze_map_at_offset(vec2 offset) {
  const float preview_width = 800.0f;
  const float preview_height = 600.0f;

  const auto screen_pct = [preview_width, preview_height](float x, float y, float w, float h) {
    return Rectangle{
        preview_width * x,
        preview_height * y,
        w,
        h,
    };
  };

  const CollisionConfig wall_config{
      .mass = std::numeric_limits<float>::max(),
      .friction = 1.f,
      .restitution = 0.f,
  };

  auto wall_color = afterhours::colors::increase(raylib::DARKGRAY, 2);
  
  // Horizontal walls
  for (int i = 0; i < 5; i++) {
    make_preview_obstacle(screen_pct(0.1f + i * 0.2f, 0.3f, 30, 30), wall_color, wall_config, offset);
    make_preview_obstacle(screen_pct(0.1f + i * 0.2f, 0.7f, 30, 30), wall_color, wall_config, offset);
  }

  // Vertical walls
  for (int i = 0; i < 3; i++) {
    make_preview_obstacle(screen_pct(0.3f, 0.1f + i * 0.3f, 30, 30), wall_color, wall_config, offset);
    make_preview_obstacle(screen_pct(0.7f, 0.1f + i * 0.3f, 30, 30), wall_color, wall_config, offset);
  }
}

void MapManager::create_race_map_at_offset(vec2 offset) {
  const float preview_width = 800.0f;
  const float preview_height = 600.0f;

  const auto screen_pct = [preview_width, preview_height](float x, float y, float w, float h) {
    return Rectangle{
        preview_width * x,
        preview_height * y,
        w,
        h,
    };
  };

  const CollisionConfig barrier_config{
      .mass = std::numeric_limits<float>::max(),
      .friction = 1.f,
      .restitution = 0.f,
  };

  // Outer track
  for (int i = 0; i < 8; i++) {
    float angle = i * 0.785f; // 45 degrees
    float x = 0.5f + 0.3f * cos(angle);
    float y = 0.5f + 0.3f * sin(angle);
    make_preview_obstacle(screen_pct(x, y, 40, 40), raylib::ORANGE, barrier_config, offset);
  }

  // Inner track
  for (int i = 0; i < 6; i++) {
    float angle = i * 1.047f; // 60 degrees
    float x = 0.5f + 0.15f * cos(angle);
    float y = 0.5f + 0.15f * sin(angle);
    make_preview_obstacle(screen_pct(x, y, 40, 40), raylib::RED, barrier_config, offset);
  }
}

void MapManager::create_battle_map_at_offset(vec2 offset) {
  const float preview_width = 800.0f;
  const float preview_height = 600.0f;

  const auto screen_pct = [preview_width, preview_height](float x, float y, float w, float h) {
    return Rectangle{
        preview_width * x,
        preview_height * y,
        w,
        h,
    };
  };

  const CollisionConfig cover_config{
      .mass = std::numeric_limits<float>::max(),
      .friction = 1.f,
      .restitution = 0.f,
  };

  // Corner cover
  make_preview_obstacle(screen_pct(0.15f, 0.15f, 35, 35), raylib::BROWN, cover_config, offset);
  make_preview_obstacle(screen_pct(0.85f, 0.15f, 35, 35), raylib::BROWN, cover_config, offset);
  make_preview_obstacle(screen_pct(0.15f, 0.85f, 35, 35), raylib::BROWN, cover_config, offset);
  make_preview_obstacle(screen_pct(0.85f, 0.85f, 35, 35), raylib::BROWN, cover_config, offset);

  // Center cover
  make_preview_obstacle(screen_pct(0.5f, 0.3f, 35, 35), raylib::BROWN, cover_config, offset);
  make_preview_obstacle(screen_pct(0.5f, 0.7f, 35, 35), raylib::BROWN, cover_config, offset);
  make_preview_obstacle(screen_pct(0.3f, 0.5f, 35, 35), raylib::BROWN, cover_config, offset);
  make_preview_obstacle(screen_pct(0.7f, 0.5f, 35, 35), raylib::BROWN, cover_config, offset);
}

void MapManager::create_catmouse_map_at_offset(vec2 offset) {
  const float preview_width = 800.0f;
  const float preview_height = 600.0f;

  const auto screen_pct = [preview_width, preview_height](float x, float y, float w, float h) {
    return Rectangle{
        preview_width * x,
        preview_height * y,
        w,
        h,
    };
  };

  const CollisionConfig safe_zone_config{
      .mass = std::numeric_limits<float>::max(),
      .friction = 1.f,
      .restitution = 0.f,
  };

  // Safe zones (smaller, harder to reach)
  make_preview_obstacle(screen_pct(0.1f, 0.1f, 25, 25), raylib::GREEN, safe_zone_config, offset);
  make_preview_obstacle(screen_pct(0.9f, 0.1f, 25, 25), raylib::GREEN, safe_zone_config, offset);
  make_preview_obstacle(screen_pct(0.1f, 0.9f, 25, 25), raylib::GREEN, safe_zone_config, offset);
  make_preview_obstacle(screen_pct(0.9f, 0.9f, 25, 25), raylib::GREEN, safe_zone_config, offset);

  // Chase obstacles
  make_preview_obstacle(screen_pct(0.5f, 0.2f, 25, 25), raylib::BLUE, safe_zone_config, offset);
  make_preview_obstacle(screen_pct(0.5f, 0.8f, 25, 25), raylib::BLUE, safe_zone_config, offset);
  make_preview_obstacle(screen_pct(0.2f, 0.5f, 25, 25), raylib::BLUE, safe_zone_config, offset);
  make_preview_obstacle(screen_pct(0.8f, 0.5f, 25, 25), raylib::BLUE, safe_zone_config, offset);
}

// Original map creation functions
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

  // Horizontal walls
  auto wall_color = afterhours::colors::increase(raylib::DARKGRAY, 2);
  for (int i = 0; i < 5; i++) {
    make_obstacle(screen_pct(0.1f + i * 0.2f, 0.3f, 30, 30), wall_color,
                  wall_config);
    make_obstacle(screen_pct(0.1f + i * 0.2f, 0.7f, 30, 30), wall_color,
                  wall_config);
  }

  // Vertical walls
  for (int i = 0; i < 3; i++) {
    make_obstacle(screen_pct(0.3f, 0.1f + i * 0.3f, 30, 30), wall_color,
                  wall_config);
    make_obstacle(screen_pct(0.7f, 0.1f + i * 0.3f, 30, 30), wall_color,
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