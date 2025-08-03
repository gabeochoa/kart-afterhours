#include "map_system.h"
#include "components.h"
#include "makers.h"
#include "rl.h"
#include "round_settings.h"

// Map preview constants
namespace {
// Preview texture dimensions (square for UI consistency)
// Range: 200-500px works well for most screen resolutions
constexpr int PREVIEW_TEXTURE_SIZE = 300;



// Preview isolation offset (keeps preview entities far from main game)
// Range: 50000+ to avoid any collision with main game area
// Should be >> largest possible screen resolution + wrap padding
constexpr float PREVIEW_BASE_OFFSET = 100000.0f;

// Spacing between different map previews
// Range: 20000+ to ensure complete separation between map preview areas
constexpr float PREVIEW_MAP_SPACING = 10000.0f;

// Camera zoom margin (prevents edge clipping in preview)
// Range: 0.7-0.9, lower = more margin, higher = tighter crop
constexpr float PREVIEW_ZOOM_MARGIN = 0.8f;

// Helper function to calculate preview offset for a given map index
vec2 get_preview_offset(int map_index) {
  float offset = PREVIEW_BASE_OFFSET + (map_index * PREVIEW_MAP_SPACING);
  return {offset, offset};
}
} // namespace

const std::array<MapConfig, MapManager::MAP_COUNT> MapManager::available_maps =
    {{{.display_name = "Test Map",
       .description = "Test map with green walls and big X for preview testing",
       .create_map_func = create_test_map,
       .compatible_round_types = std::bitset<4>(
           0b1111)}, // All round types (Lives, Kills, Score, CatAndMouse)
      {.display_name = "Arena",
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

void MapManager::initialize_preview_textures() {
  if (preview_textures_initialized)
    return;

  for (size_t i = 0; i < preview_textures.size(); i++) {
    preview_textures[i] =
        raylib::LoadRenderTexture(PREVIEW_TEXTURE_SIZE, PREVIEW_TEXTURE_SIZE);
  }

  preview_textures_initialized = true;
  generate_all_previews();
}

void MapManager::generate_map_preview(int map_index) {
  if (!preview_textures_initialized || map_index < 0 || map_index >= MAP_COUNT)
    return;

  cleanup_preview_area(map_index);
  available_maps[map_index].create_map_func();

  vec2 preview_offset = get_preview_offset(map_index);

  auto map_entities = EntityQuery({.force_merge = true})
                          .whereHasComponent<MapGenerated>()
                          .gen();

  for (auto &entity : map_entities) {
    if (entity.get().has<Transform>()) {
      auto &transform = entity.get().get<Transform>();
      transform.position.x += preview_offset.x;
      transform.position.y += preview_offset.y;
    }
  }

  // Calculate the actual map bounds to center the camera properly
  auto *pcr = EntityHelper::get_singleton_cmp<
      window_manager::ProvidesCurrentResolution>();
  window_manager::Resolution resolution = pcr->current_resolution;

  raylib::Camera2D camera = {};
  float zoom_x = PREVIEW_TEXTURE_SIZE / static_cast<float>(resolution.width);
  float zoom_y = PREVIEW_TEXTURE_SIZE / static_cast<float>(resolution.height);
  camera.zoom = std::min(zoom_x, zoom_y) * PREVIEW_ZOOM_MARGIN;
  camera.offset = {PREVIEW_TEXTURE_SIZE / 2.0f, PREVIEW_TEXTURE_SIZE / 2.0f};

  // The map is created using screen coordinates, so the actual map bounds are:
  // X: preview_offset.x + 0 to preview_offset.x + resolution.width
  // Y: preview_offset.y + 0 to preview_offset.y + resolution.height
  vec2 map_center = {preview_offset.x + resolution.width / 2.0f,
                     preview_offset.y + resolution.height / 2.0f};
  camera.target = map_center;

  raylib::BeginTextureMode(preview_textures[map_index]);
  raylib::ClearBackground(raylib::DARKGRAY);
  raylib::BeginMode2D(camera);

  auto preview_entities = EntityQuery({.force_merge = true})
                              .whereHasComponent<Transform>()
                              .whereHasComponent<HasColor>()
                              .gen();

  for (auto &entity : preview_entities) {
    auto &transform = entity.get().get<Transform>();
    auto &color = entity.get().get<HasColor>();

    if (transform.position.x >= preview_offset.x &&
        transform.position.x < preview_offset.x + resolution.width &&
        transform.position.y >= preview_offset.y &&
        transform.position.y < preview_offset.y + resolution.height) {

      raylib::DrawRectangle(static_cast<int>(transform.position.x),
                            static_cast<int>(transform.position.y),
                            static_cast<int>(transform.size.x),
                            static_cast<int>(transform.size.y), color.color());
    }
  }

  raylib::EndMode2D();
  raylib::EndTextureMode();
  cleanup_preview_area(map_index);
}

void MapManager::generate_all_previews() {
  for (int i = 0; i < MAP_COUNT; i++) {
    generate_map_preview(i);
  }
}

const raylib::RenderTexture2D &
MapManager::get_preview_texture(int map_index) const {
  return preview_textures[map_index];
}

void MapManager::cleanup_preview_textures() {
  if (!preview_textures_initialized)
    return;

  for (auto &texture : preview_textures) {
    raylib::UnloadRenderTexture(texture);
  }
  preview_textures_initialized = false;
}

void MapManager::cleanup_preview_area(int map_index) {
  vec2 preview_offset = get_preview_offset(map_index);

  auto *pcr = EntityHelper::get_singleton_cmp<
      window_manager::ProvidesCurrentResolution>();
  window_manager::Resolution resolution = pcr->current_resolution;

  auto entities_in_area =
      EntityQuery({.force_merge = true}).whereHasComponent<Transform>().gen();

  for (auto &entity : entities_in_area) {
    if (entity.get().has<Transform>()) {
      auto &transform = entity.get().get<Transform>();

      if (transform.position.x >= preview_offset.x &&
          transform.position.x < preview_offset.x + resolution.width &&
          transform.position.y >= preview_offset.y &&
          transform.position.y < preview_offset.y + resolution.height) {
        entity.get().cleanup = true;
      }
    }
  }
}

// Map creation functions
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

void MapManager::create_test_map() {
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

  // Create green walls along the edges
  // Top wall
  for (int i = 0; i < 10; i++) {
    float x = 0.05f + i * 0.09f;
    make_obstacle(screen_pct(x, 0.05f, 30, 30), raylib::GREEN, wall_config);
  }

  // Bottom wall
  for (int i = 0; i < 10; i++) {
    float x = 0.05f + i * 0.09f;
    make_obstacle(screen_pct(x, 0.95f, 30, 30), raylib::GREEN, wall_config);
  }

  // Left wall
  for (int i = 0; i < 8; i++) {
    float y = 0.15f + i * 0.1f;
    make_obstacle(screen_pct(0.05f, y, 30, 30), raylib::GREEN, wall_config);
  }

  // Right wall
  for (int i = 0; i < 8; i++) {
    float y = 0.15f + i * 0.1f;
    make_obstacle(screen_pct(0.95f, y, 30, 30), raylib::GREEN, wall_config);
  }

  // Create a big X in the center using red obstacles
  const CollisionConfig x_config{
      .mass = std::numeric_limits<float>::max(),
      .friction = 1.f,
      .restitution = 0.f,
  };

  // Diagonal from top-left to bottom-right
  for (int i = 0; i < 6; i++) {
    float offset = 0.1f + i * 0.15f;
    make_obstacle(screen_pct(offset, offset, 25, 25), raylib::RED, x_config);
  }

  // Diagonal from top-right to bottom-left
  for (int i = 0; i < 6; i++) {
    float offset = 0.1f + i * 0.15f;
    make_obstacle(screen_pct(0.9f - offset, offset, 25, 25), raylib::RED,
                  x_config);
  }
}