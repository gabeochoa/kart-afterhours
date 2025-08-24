#include "../game_state_manager.h"
#include "../makers.h"
#include "../map_system.h"
#include "../round_settings.h"
#include "../ui_systems.h"
#include "../config.h"
#include "../navigation.h"
#include "../preload.h"
#include "../texture_library.h"
#include "../input_mapping.h"
#include "../components.h"
#include "../query.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/ui.h>
#include <afterhours/src/plugins/color.h>
#include "animation_key.h"
#include "reusable_components.h"
#include <afterhours/src/plugins/animation.h>
#include <fmt/format.h>

using namespace afterhours;
using namespace afterhours::ui;
using namespace afterhours::ui::imm;

#pragma region externs
extern bool running;
#pragma endregion

#if 1
static inline void apply_slide_mods(afterhours::Entity &ent, float slide_v) {
  if (!ent.has<afterhours::ui::UIComponent>())
    return;
  auto &mods = ent.addComponentIfMissing<afterhours::ui::HasUIModifiers>();
  auto rect_now = ent.get<afterhours::ui::UIComponent>().rect();
  float off_left = -(rect_now.x + rect_now.width + 20.0f);
  float tx = (1.0f - std::min(slide_v, 1.0f)) * off_left;
  mods.translate_x = tx;
  mods.translate_y = 0.0f;
  ent.addComponentIfMissing<afterhours::ui::HasOpacity>().value =
      std::clamp(slide_v, 0.0f, 1.0f);
}
#endif

using Screen = GameStateManager::Screen;

struct ScheduleMainMenuUI : System<afterhours::ui::UIContext<InputAction>> {
  Screen get_active_screen() { return GameStateManager::get().active_screen; }

  void set_active_screen(Screen screen) {
    GameStateManager::get().set_screen(screen);
  }

  // settings cache stuff for now
  window_manager::ProvidesAvailableWindowResolutions *resolution_provider{
      nullptr};
  window_manager::ProvidesCurrentResolution *current_resolution_provider{
      nullptr};
  std::vector<std::string> resolution_strs;
  size_t resolution_index{0};

  // character creators
  std::vector<RefEntity> players;
  std::vector<RefEntity> ais;
  input::PossibleInputCollector inpc;

  void update_resolution_cache();
  void character_selector_column(Entity &parent,
                                 UIContext<InputAction> &context,
                                 const size_t index, const size_t num_slots);
  void round_end_player_column(Entity &parent, UIContext<InputAction> &context,
                               const size_t index,
                               const std::vector<OptEntity> &round_players,
                               const std::vector<OptEntity> &round_ais,
                               std::optional<int> ranking = std::nullopt);
  std::map<EntityID, int>
  get_tag_and_go_rankings(const std::vector<OptEntity> &round_players,
                          const std::vector<OptEntity> &round_ais);
  void render_round_settings_preview(UIContext<InputAction> &context,
                                     Entity &parent);

  void render_map_preview(UIContext<InputAction> &context, Entity &preview_box,
                          int effective_preview_index, int selected_map_index,
                          const std::vector<std::pair<int, MapConfig>> &compatible_maps,
                          bool overriding_preview, int prev_preview_index);

  void start_game_with_random_animation();

  // Screen handlers
  Screen character_creation(Entity &entity, UIContext<InputAction> &context);
  Screen map_selection(Entity &entity, UIContext<InputAction> &context);
  Screen round_settings(Entity &entity, UIContext<InputAction> &context);
  Screen main_screen(Entity &entity, UIContext<InputAction> &context);
  Screen settings_screen(Entity &entity, UIContext<InputAction> &context);
  Screen about_screen(Entity &entity, UIContext<InputAction> &context);
  Screen round_end_screen(Entity &entity, UIContext<InputAction> &context);

  void exit_game() { running = false; }

  virtual void once(float) override;
  virtual bool should_run(float) override;
  virtual void for_each_with(Entity &entity,
                             afterhours::ui::UIContext<InputAction> &context,
                             float) override;
};

// Minimal stub implementations to satisfy Bazel build
Screen ScheduleMainMenuUI::character_creation(Entity &entity, UIContext<InputAction> &context) {
  (void)entity; (void)context; return get_active_screen();
}

Screen ScheduleMainMenuUI::map_selection(Entity &entity, UIContext<InputAction> &context) {
  (void)entity; (void)context; return get_active_screen();
}

Screen ScheduleMainMenuUI::round_settings(Entity &entity, UIContext<InputAction> &context) {
  (void)entity; (void)context; return get_active_screen();
}

Screen ScheduleMainMenuUI::main_screen(Entity &entity, UIContext<InputAction> &context) {
  (void)entity; (void)context; return get_active_screen();
}

Screen ScheduleMainMenuUI::settings_screen(Entity &entity, UIContext<InputAction> &context) {
  (void)entity; (void)context; return get_active_screen();
}

Screen ScheduleMainMenuUI::about_screen(Entity &entity, UIContext<InputAction> &context) {
  (void)entity; (void)context; return get_active_screen();
}

Screen ScheduleMainMenuUI::round_end_screen(Entity &entity, UIContext<InputAction> &context) {
  (void)entity; (void)context; return get_active_screen();
}

void ScheduleMainMenuUI::update_resolution_cache() {
  resolution_provider = EntityHelper::get_singleton_cmp<
      window_manager::ProvidesAvailableWindowResolutions>();

  resolution_strs.clear();

  std::vector<std::string> temp;
  std::ranges::transform(resolution_provider->fetch_data(),
                         std::back_inserter(temp),
                         [](const auto &rez) { return std::string(rez); });
  resolution_strs = std::move(temp);
  resolution_index = resolution_provider->current_index();
}

void ScheduleMainMenuUI::once(float) {

  current_resolution_provider = EntityHelper::get_singleton_cmp<
      window_manager::ProvidesCurrentResolution>();

  if (GameStateManager::get().active_screen ==
      GameStateManager::Screen::Settings) {
    update_resolution_cache();
  }

  {
    players = EQ().whereHasComponent<PlayerID>().orderByPlayerID().gen();
    ais = EQ().whereHasComponent<AIControlled>().gen();
    inpc = input::get_input_collector();
  }
}

bool ScheduleMainMenuUI::should_run(float) {
  auto *nav = EntityHelper::get_singleton_cmp<MenuNavigationStack>();
  return GameStateManager::get().is_menu_active() &&
         (nav ? nav->ui_visible : true);
}

void ScheduleMainMenuUI::character_selector_column(
    Entity &parent, UIContext<InputAction> &context, const size_t index,
    const size_t num_slots) {

  bool is_last_slot = index == num_slots - 1;
  bool is_last_slot_ai = index >= players.size();
  bool is_slot_ai = index >= players.size();

  OptEntity car;
  if (!is_last_slot || index < (ais.size() + players.size())) {
    car = index < players.size() //
              ? players[index]
              : ais[index - players.size()];
  }

  ManagesAvailableColors &colorManager =
      *EntityHelper::get_singleton_cmp<ManagesAvailableColors>();

  auto bg_color = car.has_value() //
                      ? car->get<HasColor>().color()
                      : afterhours::colors::opacity_pct(
                            colorManager.get_next_NO_STORE(index), 0.1f);

  const auto num_cols = std::min(4.f, static_cast<float>(num_slots));

  if (is_last_slot && (players.size() + ais.size()) >= input::MAX_GAMEPAD_ID) {
    return;
  }

  auto column =
      imm::div(context, mk(parent, (int)index),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(1.f / num_cols, 0.1f),
                                            percent(1.f, 0.4f)})
                   .with_margin(Margin{
                       .top = percent(0.02f),
                       .bottom = percent(0.02f),
                       .left = percent(0.02f),
                       .right = percent(0.02f),
                   })
                   .with_color_usage(Theme::Usage::Custom)
                   .with_custom_color(bg_color)
                   .disable_rounded_corners());

  std::string label = car.has_value() ? fmt::format("{} {}", index, car->id)
                                      : fmt::format("{} Empty", index);

  bool player_right = false;
  if (index < players.size()) {
    for (const auto &actions_done : inpc.inputs_pressed()) {
      if (static_cast<size_t>(actions_done.id) != index) {
        continue;
      }

      if (actions_done.medium == input::DeviceMedium::GamepadAxis) {
        continue;
      }

              player_right |= action_matches(actions_done.action, InputAction::WidgetRight);

      if (player_right) {
        break;
      }
    }
  }

  bool show_next_color_button =
      (is_last_slot && !is_last_slot_ai) ||
      (!is_last_slot && colorManager.any_available_colors());

  std::function<void()> on_next_color = nullptr;
  if (show_next_color_button && car.has_value()) {
    on_next_color = [&colorManager, &car]() {
      colorManager.release_and_get_next(car->id);
    };
  }

  std::function<void()> on_remove = nullptr;
  if (is_slot_ai && car.has_value()) {
    on_remove = [&colorManager, &car]() {
      colorManager.release_only(car->id);
      car->cleanup = true;
    };
  }

  std::function<void()> on_add_ai = nullptr;
  bool show_add_ai = false;
  if (num_slots <= input::MAX_GAMEPAD_ID && is_last_slot) {
    show_add_ai = true;
    on_add_ai = []() { make_ai(); };
  }

  std::optional<AIDifficulty::Difficulty> ai_difficulty = std::nullopt;
  std::function<void(AIDifficulty::Difficulty)> on_difficulty_change = nullptr;

  if (is_slot_ai && car.has_value()) {
    if (car->has<AIDifficulty>()) {
      ai_difficulty = car->get<AIDifficulty>().difficulty;
    } else {
      ai_difficulty = AIDifficulty::Difficulty::Medium;
    }

    on_difficulty_change = [&car](AIDifficulty::Difficulty new_difficulty) {
      if (car.has_value()) {
        if (car->has<AIDifficulty>()) {
          car->get<AIDifficulty>().difficulty = new_difficulty;
        } else {
          car->addComponent<AIDifficulty>(new_difficulty);
        }
      }
    };
  }

  ui_reusable_components::create_player_card(
      context, column.ent(), label, bg_color, is_slot_ai, std::nullopt,
      std::nullopt, on_next_color, on_remove, show_add_ai, on_add_ai,
      ai_difficulty, on_difficulty_change);
}

void ScheduleMainMenuUI::round_end_player_column(
    Entity &parent, UIContext<InputAction> &context, const size_t index,
    const std::vector<OptEntity> &round_players,
    const std::vector<OptEntity> &round_ais, std::optional<int> ranking) {

  bool is_slot_ai = index >= round_players.size();

  OptEntity car;
  if (index < round_players.size()) {
    car = round_players[index];
  } else {
    car = round_ais[index - round_players.size()];
  }

  if (!car.has_value()) {
    return;
  }

  auto bg_color = car->get<HasColor>().color();
  const auto num_cols = std::min(
      4.f, static_cast<float>(round_players.size() + round_ais.size()));

  afterhours::animation::one_shot(UIKey::RoundEndCard, index,
                                  ui_anims::make_round_end_card_stagger(index));
  float card_v = afterhours::animation::clamp_value(UIKey::RoundEndCard, index,
                                                    0.0f, 1.0f);
  auto column =
      imm::div(context, mk(parent, (int)index),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(1.f / num_cols, 0.1f),
                                            percent(1.f, 0.4f)})
                   .with_margin(Margin{.top = percent(0.05f),
                                       .bottom = percent(0.05f),
                                       .left = percent(0.05f),
                                       .right = percent(0.05f)})
                   .with_color_usage(Theme::Usage::Custom)
                   .with_custom_color(bg_color)
                   .with_translate(0.0f, (1.0f - card_v) * 20.0f)
                   .with_opacity(card_v)
                   .disable_rounded_corners());

  std::string player_label = fmt::format("{} {}", index, car->id);

  std::optional<std::string> stats_text;
  switch (RoundManager::get().active_round_type) {
  case RoundType::Lives:
    if (car->has<HasMultipleLives>()) {
      stats_text = fmt::format(
          "Lives: {}", car->get<HasMultipleLives>().num_lives_remaining);
    }
    break;
  case RoundType::Kills:
    if (car->has<HasKillCountTracker>()) {
      stats_text =
          fmt::format("Kills: {}", car->get<HasKillCountTracker>().kills);
    }
    break;
  case RoundType::Hippo:
    if (car->has<HasHippoCollection>()) {
      stats_text = fmt::format(
          "Hippos: {}", car->get<HasHippoCollection>().get_hippo_count());
    } else {
      stats_text = "Hippos: 0";
    }
    break;
  case RoundType::TagAndGo:
    if (car->has<HasTagAndGoTracking>()) {
      stats_text = fmt::format("Not It: {:.1f}s",
                               car->get<HasTagAndGoTracking>().time_as_not_it);
    }
    break;
  default:
    stats_text = "Unknown";
    break;
  }

  afterhours::animation::one_shot(UIKey::RoundEndScore, index, [](auto h) {
    h.from(0.0f).to(1.0f, 0.8f, afterhours::animation::EasingType::EaseOutQuad);
  });
  float score_t = afterhours::animation::clamp_value(UIKey::RoundEndScore,
                                                     index, 0.0f, 1.0f);

  std::optional<std::string> animated_stats = std::nullopt;
  switch (RoundManager::get().active_round_type) {
  case RoundType::Lives: {
    if (car->has<HasMultipleLives>()) {
      int final_val = car->get<HasMultipleLives>().num_lives_remaining;
      int shown = static_cast<int>(std::round(score_t * final_val));
      animated_stats = fmt::format("Lives: {}", shown);
    }
    break;
  }
  case RoundType::Kills: {
    if (car->has<HasKillCountTracker>()) {
      int final_val = car->get<HasKillCountTracker>().kills;
      int shown = static_cast<int>(std::round(score_t * final_val));
      animated_stats = fmt::format("Kills: {}", shown);
    }
    break;
  }
  case RoundType::Hippo: {
    int final_val = car->has<HasHippoCollection>()
                        ? car->get<HasHippoCollection>().get_hippo_count()
                        : 0;
    int shown = static_cast<int>(std::round(score_t * final_val));
          animated_stats = fmt::format("Hippos: {}", shown);
    break;
  }
  case RoundType::TagAndGo: {
    if (car->has<HasTagAndGoTracking>()) {
      float final_val = car->get<HasTagAndGoTracking>().time_as_not_it;
      float shown = std::round(score_t * final_val * 10.0f) / 10.0f;
      animated_stats = fmt::format("Not It: {:.1f}s", shown);
    }
    break;
  }
  default: {
    break;
  }
  }

  ui_reusable_components::create_player_card(
      context, column.ent(), player_label, bg_color, is_slot_ai, ranking,
      animated_stats.has_value() ? animated_stats : stats_text);
}

std::map<EntityID, int> ScheduleMainMenuUI::get_tag_and_go_rankings(
    const std::vector<OptEntity> &round_players,
    const std::vector<OptEntity> &round_ais) {
  std::map<EntityID, int> rankings;

  std::vector<std::pair<EntityID, float>> player_times;

  for (const auto &player : round_players) {
    if (player->has<HasTagAndGoTracking>()) {
      player_times.emplace_back(
          player->id, player->get<HasTagAndGoTracking>().time_as_not_it);
    }
  }

  for (const auto &ai : round_ais) {
    if (ai->has<HasTagAndGoTracking>()) {
      player_times.emplace_back(ai->id,
                                ai->get<HasTagAndGoTracking>().time_as_not_it);
    }
  }

  std::sort(player_times.begin(), player_times.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; });

  for (size_t i = 0; i < player_times.size(); ++i) {
    rankings[player_times[i].first] = static_cast<int>(i + 1);
  }

  return rankings;
}

void ScheduleMainMenuUI::render_round_settings_preview(
    UIContext<InputAction> &context, Entity &parent) {
  imm::div(context, mk(parent),
           ComponentConfig{}.with_label(fmt::format(
               "Win Condition: {}",
               magic_enum::enum_name(RoundManager::get().active_round_type))));

  auto *spritesheet_component = EntityHelper::get_singleton_cmp<
      afterhours::texture_manager::HasSpritesheet>();
  if (spritesheet_component) {
    raylib::Texture2D sheet = spritesheet_component->texture;
    const auto &weps = RoundManager::get().get_enabled_weapons();
    const size_t num_enabled = weps.count();
    if (num_enabled > 0) {
      float icon_px =
          current_resolution_provider
              ? (current_resolution_provider->current_resolution.height /
                 720.f) *
                    32.f
              : 32.f;

      std::vector<afterhours::texture_manager::Rectangle> frames;
      frames.reserve(num_enabled);
      for (size_t i = 0; i < WEAPON_COUNT; ++i) {
        if (!weps.test(i))
          continue;
        frames.push_back(weapon_icon_frame(static_cast<Weapon::Type>(i)));
      }

      imm::icon_row(context, mk(parent), sheet, frames, icon_px / 32.f,
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.f), pixels(icon_px)})
                        .with_skip_tabbing(true)
                        .with_debug_name("weapon_icon_row"));
    }
  }

  switch (RoundManager::get().active_round_type) {
  case RoundType::Lives: {
    auto &s = RoundManager::get().get_active_rt<RoundLivesSettings>();
    imm::div(context, mk(parent),
             ComponentConfig{}.with_label(
                 fmt::format("Num Lives: {}", s.num_starting_lives)));
    break;
  }
  case RoundType::Kills: {
    auto &s = RoundManager::get().get_active_rt<RoundKillsSettings>();
    std::string time_display;
    switch (s.time_option) {
    case RoundSettings::TimeOptions::Unlimited:
      time_display = "Unlimited";
      break;
    case RoundSettings::TimeOptions::Seconds_10:
      time_display = "10s";
      break;
    case RoundSettings::TimeOptions::Seconds_30:
      time_display = "30s";
      break;
    case RoundSettings::TimeOptions::Minutes_1:
      time_display = "1m";
      break;
    }
    imm::div(context, mk(parent),
             ComponentConfig{}.with_label(
                 fmt::format("Round Length: {}", time_display)));
    break;
  }
  case RoundType::Hippo: {
    auto &s = RoundManager::get().get_active_rt<RoundHippoSettings>();
    imm::div(context, mk(parent),
             ComponentConfig{}.with_label(
                 fmt::format("Total Hippos: {}", s.total_hippos)));
    break;
  }
  case RoundType::TagAndGo: {
    auto &s = RoundManager::get().get_active_rt<RoundTagAndGoSettings>();
    std::string time_display;
    switch (s.time_option) {
    case RoundSettings::TimeOptions::Unlimited:
      time_display = "Unlimited";
      break;
    case RoundSettings::TimeOptions::Seconds_10:
      time_display = "10s";
      break;
    case RoundSettings::TimeOptions::Seconds_30:
      time_display = "30s";
      break;
    case RoundSettings::TimeOptions::Minutes_1:
      time_display = "1m";
      break;
    }
    imm::div(context, mk(parent),
             ComponentConfig{}.with_label(
                 fmt::format("Round Length: {}", time_display)));
    break;
  }
  default:
    imm::div(context, mk(parent),
             ComponentConfig{}.with_label("Round Settings"));
    break;
  }
}

void ScheduleMainMenuUI::render_map_preview(
    UIContext<InputAction> &context, Entity &preview_box,
    int effective_preview_index, int selected_map_index,
    const std::vector<std::pair<int, MapConfig>> &compatible_maps,
    bool overriding_preview, int prev_preview_index) {
  auto maybe_shuffle =
      afterhours::animation::manager<UIKey>().get_value(UIKey::MapShuffle);

  {
    float container_fade = afterhours::animation::manager<UIKey>()
                               .get_value(UIKey::MapPreviewFade)
                               .value_or(1.0f);
    container_fade = std::clamp(container_fade, 0.0f, 1.0f);
    preview_box.addComponentIfMissing<afterhours::ui::HasOpacity>().value =
        container_fade;
  }

  float fade_v = afterhours::animation::manager<UIKey>()
                     .get_value(UIKey::MapPreviewFade)
                     .value_or(1.0f);
  fade_v = std::clamp(fade_v, 0.0f, 1.0f);

  if (effective_preview_index == MapManager::RANDOM_MAP_INDEX &&
      maybe_shuffle.has_value() && !compatible_maps.empty()) {
    int n = static_cast<int>(compatible_maps.size());
    int animated_idx = std::clamp(
        static_cast<int>(std::floor(maybe_shuffle.value())) % n, 0, n - 1);
    const auto &animated_pair =
        compatible_maps[static_cast<size_t>(animated_idx)];
    const auto &animated_map = animated_pair.second;

    imm::div(context, mk(preview_box),
             ComponentConfig{}
                 .with_label(animated_map.display_name)
                 .with_size(ComponentSize{percent(1.f), percent(0.3f)})
                 .with_opacity(fade_v)
                 .with_debug_name("map_title"));

    if (MapManager::get().preview_textures_initialized) {
      int abs_idx = animated_pair.first;
      const auto &rt = MapManager::get().get_preview_texture(abs_idx);
      imm::image(
          context, mk(preview_box),
          ComponentConfig{}
              .with_size(ComponentSize{percent(1.f), percent(0.7f, 0.1f)})
              .with_opacity(fade_v)
              .with_debug_name("map_preview")
              .with_texture(
                  rt.texture,
                  afterhours::texture_manager::HasTexture::Alignment::Center));
    }

    return;
  }

  if (effective_preview_index == MapManager::RANDOM_MAP_INDEX) {
    imm::div(context, mk(preview_box),
             ComponentConfig{}
                 .with_label("???")
                 .with_size(ComponentSize{percent(1.f), percent(0.3f)})
                 .with_opacity(fade_v)
                 .with_debug_name("map_title"));
    return;
  }

  auto selected_map_it =
      std::find_if(compatible_maps.begin(), compatible_maps.end(),
                   [effective_preview_index](const auto &pair) {
                     return pair.first == effective_preview_index;
                   });
  if (selected_map_it == compatible_maps.end()) {
    return;
  }

  const auto &preview_map = selected_map_it->second;
  imm::div(context, mk(preview_box),
           ComponentConfig{}
               .with_label(preview_map.display_name)
               .with_size(ComponentSize{percent(1.f), percent(0.3f)})
               .with_opacity(fade_v)
               .with_debug_name("map_title"));

  if (!MapManager::get().preview_textures_initialized) {
    return;
  }

  if (!overriding_preview && prev_preview_index >= 0 &&
      prev_preview_index != selected_map_index && fade_v < 1.0f) {
    const auto &rt_prev =
        MapManager::get().get_preview_texture(prev_preview_index);
    afterhours::texture_manager::Rectangle full_src_prev{
        .x = 0,
        .y = 0,
        .width = (float)rt_prev.texture.width,
        .height = (float)rt_prev.texture.height,
    };
    imm::sprite(context, mk(preview_box), rt_prev.texture, full_src_prev,
                ComponentConfig{}
                    .with_size(ComponentSize{percent(1.f), percent(1.0f)})
                    .with_debug_name("map_preview_prev")
                    .with_opacity(1.0f - fade_v)
                    .with_render_layer(0));
  }

  const auto &rt_cur =
      MapManager::get().get_preview_texture(effective_preview_index);
  imm::sprite(context, mk(preview_box), rt_cur.texture,
              afterhours::texture_manager::Rectangle{
                  .x = 0,
                  .y = 0,
                  .width = (float)rt_cur.texture.width,
                  .height = (float)rt_cur.texture.height,
              },
              ComponentConfig{}
                  .with_size(ComponentSize{percent(1.f), percent(0.5f)})
                  .with_debug_name("map_preview_cur")
                  .with_opacity((!overriding_preview &&
                                 prev_preview_index >= 0 && fade_v < 1.0f)
                                    ? fade_v
                                    : fade_v)
                  .with_render_layer(1));
}

void ScheduleMainMenuUI::start_game_with_random_animation() {
  auto round_type = RoundManager::get().active_round_type;
  auto maps = MapManager::get().get_maps_for_round_type(round_type);
  if (maps.empty())
    return;

  int n = static_cast<int>(maps.size());
  int chosen = raylib::GetRandomValue(0, n - 1);
  int final_map_index = maps[static_cast<size_t>(chosen)].first;

  afterhours::animation::anim(UIKey::MapShuffle)
      .from(0.0f)
      .sequence({
          {.to_value = static_cast<float>(n * 2),
           .duration = 0.45f,
           .easing = afterhours::animation::animation::EasingType::Linear},
          {.to_value = static_cast<float>(n + chosen),
           .duration = 0.55f,
           .easing = afterhours::animation::animation::EasingType::EaseOutQuad},
      })
      .hold(0.5f)
      .on_step(1.0f,
               [](int) {
                 auto opt = EntityQuery({.force_merge = true})
                                .whereHasComponent<SoundEmitter>()
                                .gen_first();
                 if (opt.valid()) {
                   auto &ent = opt.asE();
                   auto &req = ent.addComponentIfMissing<PlaySoundRequest>();
                   req.policy = PlaySoundRequest::Policy::Enum;
                   req.file = SoundFile::UI_Move;
                 }
               })
      .on_complete([final_map_index]() {
        MapManager::get().set_selected_map(final_map_index);
        MapManager::get().create_map();
        GameStateManager::get().start_game();
      });
}

void ScheduleMainMenuUI::for_each_with(Entity &entity,
                                       UIContext<InputAction> &context, float) {
  GameStateManager::get().update_screen();

  switch (get_active_screen()) {
  case Screen::None:
    break;
  case Screen::CharacterCreation:
    set_active_screen(character_creation(entity, context));
    break;
  case Screen::About:
    set_active_screen(about_screen(entity, context));
    break;
  case Screen::Settings:
    set_active_screen(settings_screen(entity, context));
    break;
  case Screen::Main:
    set_active_screen(main_screen(entity, context));
    break;
  case Screen::RoundSettings:
    set_active_screen(round_settings(entity, context));
    break;
  case Screen::MapSelection:
    set_active_screen(map_selection(entity, context));
    break;
  case Screen::RoundEnd:
    set_active_screen(round_end_screen(entity, context));
    break;
  }
}
