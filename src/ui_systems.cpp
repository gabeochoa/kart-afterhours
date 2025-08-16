#include "ui_systems.h"
#include "car_affectors.h"
#include "components.h"
#include "components_weapons.h"
#include "game_state_manager.h"
#include "input_mapping.h"
#include "intro.h"
#include "log.h"
#include "makers.h"
#include "map_system.h"
#include "navigation.h"
#include "preload.h" // FontID
#include "texture_library.h"
#include "ui/animation_key.h"
#include "math_util.h"
#include "max_health.h"
#include "music_library.h"
#include "query.h"
#include "resources.h"
#include "rl.h"
#include "round_settings.h"
#include "settings.h"
#include "shader_library.h"
#include "sound_library.h"
#include "sound_systems.h"
#include "systems.h"
#include "systems_ai.h"
#include "systems_roundtypes.h"
#include "ui/animation_button_wiggle.h"
#include "ui/animation_key.h"
#include "ui/animation_slide_in.h"
#include "utils.h"
#include "weapons.h"
#include <fmt/format.h>
#include <afterhours/src/plugins/animation.h>

using namespace afterhours;

ScheduleMainMenuUI::~ScheduleMainMenuUI() = default;
ScheduleDebugUI::~ScheduleDebugUI() = default;
SchedulePauseUI::~SchedulePauseUI() = default;

ScheduleMainMenuUI::ScheduleMainMenuUI() = default;
ScheduleDebugUI::ScheduleDebugUI() = default;
SchedulePauseUI::SchedulePauseUI() = default;

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



// Reusable UI component functions
namespace ui_helpers {

// Reusable player card component
ElementResult create_player_card(
    UIContext<InputAction> &context, Entity &parent, const std::string &label,
    raylib::Color bg_color, bool is_ai = false,
    std::optional<int> ranking = std::nullopt,
    std::optional<std::string> stats_text = std::nullopt,
    std::function<void()> on_next_color = nullptr,
    std::function<void()> on_remove = nullptr, bool show_add_ai = false,
    std::function<void()> on_add_ai = nullptr,
    std::optional<AIDifficulty::Difficulty> ai_difficulty = std::nullopt,
    std::function<void(AIDifficulty::Difficulty)> on_difficulty_change =
        nullptr) {

  auto card = imm::div(context, mk(parent),
                       ComponentConfig{}
                           .with_size(ComponentSize{percent(1.f), percent(1.f)})
                           .with_margin(Margin{.top = percent(0.1f),
                                               .bottom = percent(0.1f),
                                               .left = percent(0.1f),
                                               .right = percent(0.1f)})
                           .with_color_usage(Theme::Usage::Custom)
                           .with_custom_color(bg_color)
                           .disable_rounded_corners());

  std::string player_label = label;
  if (is_ai) {
    player_label += " (AI)";
  }

  auto header_row =
      imm::div(context, mk(card.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
                   .with_flex_direction(FlexDirection::Row)
                   .with_debug_name("player_card_header"));

  bool show_next_color_icon = static_cast<bool>(on_next_color);
  bool show_remove_icon = is_ai && static_cast<bool>(on_remove);
  int header_icon_count =
      (show_next_color_icon ? 1 : 0) + (show_remove_icon ? 1 : 0);
  const float header_icon_width = 0.2f;
  float label_width =
      1.f - (header_icon_width * static_cast<float>(header_icon_count));
  if (label_width < 0.2f)
    label_width = 0.2f;

  imm::div(context, mk(header_row.ent()),
           ComponentConfig{}
               .with_size(ComponentSize{percent(label_width), percent(1.f)})
               .with_label(player_label)
               .with_color_usage(Theme::Usage::Custom)
               .with_custom_color(bg_color)
               .disable_rounded_corners()
               .with_debug_name("player_card_label"));

  // Stats text (if provided)
  if (stats_text.has_value()) {
    imm::div(context, mk(card.ent(), 1),
             ComponentConfig{}
                 .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
                 .with_label(stats_text.value())
                 .with_color_usage(Theme::Usage::Custom)
                 .with_custom_color(bg_color)
                 .disable_rounded_corners());
  }

  // Ranking (for top 3)
  if (ranking.has_value() && ranking.value() <= 3) {
    std::string ranking_label = fmt::format("#{}", ranking.value());

    imm::div(context, mk(card.ent(), 2),
             ComponentConfig{}
                 .with_size(ComponentSize{percent(1.f), percent(0.3f, 0.4f)})
                 .with_label(ranking_label)
                 .with_font(get_font_name(FontID::EQPro), 120.f)
                 .with_color_usage(Theme::Usage::Custom)
                 .with_custom_color(bg_color)
                 .disable_rounded_corners()
                 .with_debug_name("player_card_ranking"));
  }

  if (show_next_color_icon) {
    raylib::Texture2D sheet = EntityHelper::get_singleton_cmp<
                                  afterhours::texture_manager::HasSpritesheet>()
                                  ->texture;
    auto src = afterhours::texture_manager::idx_to_sprite_frame(0, 6);

    auto icon_cell = imm::div(
        context, mk(header_row.ent()),
        ComponentConfig{}
            .with_size(ComponentSize{percent(header_icon_width), percent(1.f)})
            .with_padding(Padding{.top = percent(0.02f),
                                  .left = percent(0.02f),
                                  .bottom = percent(0.02f),
                                  .right = percent(0.02f)})
            .with_debug_name("next_color_cell"));

    if (imm::image_button(
            context, mk(icon_cell.ent()), sheet, src,
            ComponentConfig{}
                .with_size(ComponentSize{percent(1.f), percent(1.f)})
                .with_debug_name("next_color_icon")))
      on_next_color();
  }

  if (show_remove_icon) {
    auto &trash_tex = TextureLibrary::get().get("trashcan");
    raylib::Rectangle src{0.f, 0.f, static_cast<float>(trash_tex.width),
                          static_cast<float>(trash_tex.height)};

    auto icon_cell = imm::div(
        context, mk(header_row.ent()),
        ComponentConfig{}
            .with_size(ComponentSize{percent(header_icon_width), percent(1.f)})
            .with_padding(Padding{.top = percent(0.02f),
                                  .left = percent(0.02f),
                                  .bottom = percent(0.02f),
                                  .right = percent(0.02f)})
            .with_debug_name("remove_ai_cell"));

    if (imm::image_button(
            context, mk(icon_cell.ent()), trash_tex, src,
            ComponentConfig{}
                .with_size(ComponentSize{percent(1.f), percent(1.f)})
                .with_debug_name("remove_ai_icon"))) {
      on_remove();
    }
  }

  // AI Difficulty navigation bar
  if (is_ai && ai_difficulty.has_value() && on_difficulty_change) {
    auto difficulty_options =
        std::vector<std::string>{"Easy", "Medium", "Hard", "Expert"};
    auto current_difficulty = static_cast<size_t>(ai_difficulty.value());

    if (auto result = imm::navigation_bar(
            context, mk(card.ent()), difficulty_options, current_difficulty,
            ComponentConfig{}
                .with_size(ComponentSize{percent(1.f), pixels(50.f)})
                .disable_rounded_corners()
                .with_debug_name("ai_difficulty_navigation_bar"))) {
      on_difficulty_change(
          static_cast<AIDifficulty::Difficulty>(current_difficulty));
    }
  }

  // Remove AI button (trashcan icon)
  // removed: separate remove ai button; handled in header

  // Add AI button (dollar_sign icon)
  if (show_add_ai && on_add_ai) {
    auto &dollar_tex = TextureLibrary::get().get("dollar_sign");
    raylib::Rectangle src{0.f, 0.f, static_cast<float>(dollar_tex.width),
                          static_cast<float>(dollar_tex.height)};

    if (imm::image_button(
            context, mk(card.ent()), dollar_tex, src,
            ComponentConfig{}
                .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
                .with_padding(Padding{.top = percent(0.25f)})
                .disable_rounded_corners()
                .with_debug_name("add_ai_button"))) {
      on_add_ai();
    }
  }

  return {true, card.ent()};
}

// Reusable styled button component
ElementResult create_styled_button(UIContext<InputAction> &context,
                                   Entity &parent, const std::string &label,
                                   std::function<void()> on_click,
                                   int index = 0) {

  if (imm::button(context, mk(parent, index),
                  ComponentConfig{}
                      .with_padding(Padding{.top = pixels(5.f),
                                            .left = pixels(0.f),
                                            .bottom = pixels(5.f),
                                            .right = pixels(0.f)})
                      .with_label(label))) {
    on_click();
    return {true, parent};
  }

  return {false, parent};
}

// Reusable volume slider component
ElementResult create_volume_slider(UIContext<InputAction> &context,
                                   Entity &parent, const std::string &label,
                                   float &volume,
                                   std::function<void(float)> on_change,
                                   int index = 0) {

  auto volume_label = fmt::format("{}\n {:2.0f}", label, volume * 100.f);

  if (auto result =
          slider(context, mk(parent, index), volume,
                 ComponentConfig{}.with_label(std::move(volume_label)))) {
    volume = result.as<float>();
    on_change(volume);
    return {true, parent};
  }

  return {false, parent};
}

// Reusable screen container component
ElementResult create_screen_container(UIContext<InputAction> &context,
                                      Entity &parent,
                                      const std::string &debug_name) {

  return imm::div(
      context, mk(parent),
      ComponentConfig{}
          .with_font(get_font_name(FontID::EQPro), 75.f)
          .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
          .with_absolute_position()
          .with_debug_name(debug_name));
}

// Reusable control group component
ElementResult create_control_group(UIContext<InputAction> &context,
                                   Entity &parent,
                                   const std::string &debug_name) {

  return imm::div(
      context, mk(parent),
      ComponentConfig{}
          .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
          .with_padding(Padding{.top = screen_pct(0.4f),
                                .left = screen_pct(0.4f),
                                .bottom = pixels(0.f),
                                .right = pixels(0.f)})
          .with_absolute_position()
          .with_debug_name(debug_name));
}

ElementResult create_top_left_container(UIContext<InputAction> &context,
                                        Entity &parent,
                                        const std::string &debug_name,
                                        int index) {

  return imm::div(
      context, mk(parent, index),
      ComponentConfig{}
          .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
          .with_padding(Padding{.top = screen_pct(0.02f),
                                .left = screen_pct(0.02f),
                                .bottom = pixels(0.f),
                                .right = pixels(0.f)})
          .with_absolute_position()
          .with_debug_name(debug_name));
}

} // namespace ui_helpers

using Screen = GameStateManager::Screen;
//

constexpr static vec2 button_size = vec2{100, 50};

Padding button_group_padding = Padding{
    .top = screen_pct(0.4f),
    .left = screen_pct(0.4f),
    .bottom = pixels(0.f),
    .right = pixels(0.f),
};

Padding control_group_padding = Padding{
    .top = screen_pct(0.4f),
    .left = screen_pct(0.4f),
    .bottom = pixels(0.f),
    .right = pixels(0.f),
};

Padding button_padding = Padding{
    .top = pixels(button_size.y / 10.f),
    .left = pixels(0.f),
    .bottom = pixels(button_size.y / 10.f),
    .right = pixels(0.f),
};

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

  // character creator

  {
    players = EQ().whereHasComponent<PlayerID>().orderByPlayerID().gen();
    ais = EQ().whereHasComponent<AIControlled>().gen();
    inpc = input::get_input_collector<InputAction>();
  }
}

bool ScheduleMainMenuUI::should_run(float) {
  // Visibility managed by NavigationSystem; render if menu active and UI
  // visible
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
                      // Make it more transparent for empty slots
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

  // Create player card using helper function
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

      player_right |= actions_done.action == InputAction::WidgetRight;

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

  // AI Difficulty handling
  std::optional<AIDifficulty::Difficulty> ai_difficulty = std::nullopt;
  std::function<void(AIDifficulty::Difficulty)> on_difficulty_change = nullptr;

  if (is_slot_ai && car.has_value()) {
    if (car->has<AIDifficulty>()) {
      ai_difficulty = car->get<AIDifficulty>().difficulty;
    } else {
      // Default to Medium if no difficulty component exists
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

  ui_helpers::create_player_card(
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

  // Create player label
        std::string player_label = fmt::format("{} {}", index, car->id);

  // Get stats text based on round type
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

  // Score roll-up value (0..1). We keep it generic regardless of round type
  afterhours::animation::one_shot(UIKey::RoundEndScore, index, [](auto h) {
    h.from(0.0f).to(1.0f, 0.8f, afterhours::animation::EasingType::EaseOutQuad);
  });
  float score_t = afterhours::animation::clamp_value(UIKey::RoundEndScore,
                                                     index, 0.0f, 1.0f);

  // Compute animated stats text per-round
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

  ui_helpers::create_player_card(
      context, column.ent(), player_label, bg_color, is_slot_ai, ranking,
      animated_stats.has_value() ? animated_stats : stats_text);
}

void ScheduleMainMenuUI::render_round_end_stats(UIContext<InputAction> &context,
                                                Entity &parent,
                                                const OptEntity &car,
                                                raylib::Color bg_color) {
  switch (RoundManager::get().active_round_type) {
  case RoundType::Lives:
    render_lives_stats(context, parent, car, bg_color);
    break;
  case RoundType::Kills:
    render_kills_stats(context, parent, car, bg_color);
    break;
  case RoundType::Hippo:
    render_hippo_stats(context, parent, car, bg_color);
    break;
  case RoundType::TagAndGo:
    render_tag_and_go_stats(context, parent, car, bg_color);
    break;
  default:
    render_unknown_stats(context, parent, car, bg_color);
    break;
  }
}

void ScheduleMainMenuUI::render_lives_stats(UIContext<InputAction> &context,
                                            Entity &parent,
                                            const OptEntity &car,
                                            raylib::Color bg_color) {
  if (!car->has<HasMultipleLives>()) {
    return;
  }

  std::string stats_text = fmt::format(
      "Lives: {}", car->get<HasMultipleLives>().num_lives_remaining);

  imm::div(context, mk(parent, 1),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
               .with_label(stats_text)
               .with_color_usage(Theme::Usage::Custom)
               .with_custom_color(bg_color)
               .disable_rounded_corners());
}

void ScheduleMainMenuUI::render_kills_stats(UIContext<InputAction> &context,
                                            Entity &parent,
                                            const OptEntity &car,
                                            raylib::Color bg_color) {
  if (!car->has<HasKillCountTracker>()) {
    return;
  }

  std::string stats_text =
      fmt::format("Kills: {}", car->get<HasKillCountTracker>().kills);

  imm::div(context, mk(parent, 1),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
               .with_label(stats_text)
               .with_color_usage(Theme::Usage::Custom)
               .with_custom_color(bg_color)
               .disable_rounded_corners());
}

void ScheduleMainMenuUI::render_hippo_stats(UIContext<InputAction> &context,
                                            Entity &parent,
                                            const OptEntity &car,
                                            raylib::Color bg_color) {
  if (!car->has<HasHippoCollection>()) {
    return;
  }

  std::string stats_text = fmt::format(
      "Hippos: {}", car->get<HasHippoCollection>().get_hippo_count());

  imm::div(context, mk(parent, 1),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
               .with_label(stats_text)
               .with_color_usage(Theme::Usage::Custom)
               .with_custom_color(bg_color)
               .disable_rounded_corners());
}

void ScheduleMainMenuUI::render_tag_and_go_stats(
    UIContext<InputAction> &context, Entity &parent, const OptEntity &car,
    raylib::Color bg_color) {
  if (!car->has<HasTagAndGoTracking>()) {
    return;
  }

  const auto &tracking = car->get<HasTagAndGoTracking>();
  std::string stats_text =
      fmt::format("Not It: {:.1f}s", tracking.time_as_not_it);

  imm::div(context, mk(parent, 1),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
               .with_label(stats_text)
               .with_color_usage(Theme::Usage::Custom)
               .with_custom_color(bg_color)
               .disable_rounded_corners());
}

void ScheduleMainMenuUI::render_unknown_stats(UIContext<InputAction> &context,
                                              Entity &parent, const OptEntity &,
                                              raylib::Color bg_color) {
  std::string stats_text = "Unknown";

  imm::div(context, mk(parent, 1),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
               .with_label(stats_text)
               .with_color_usage(Theme::Usage::Custom)
               .with_custom_color(bg_color)
               .disable_rounded_corners());
}

std::map<EntityID, int> ScheduleMainMenuUI::get_tag_and_go_rankings(
    const std::vector<OptEntity> &round_players,
    const std::vector<OptEntity> &round_ais) {
  std::map<EntityID, int> rankings;

  // Combine all players and AIs
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

  // Sort by runner time (highest first - most time not it wins)
  std::sort(player_times.begin(), player_times.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; });

  // Assign rankings (1-based)
  for (size_t i = 0; i < player_times.size(); ++i) {
    rankings[player_times[i].first] = static_cast<int>(i + 1);
  }

  return rankings;
}

Screen ScheduleMainMenuUI::character_creation(Entity &entity,
                                              UIContext<InputAction> &context) {
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("character_creation"));

  auto top_left = ui_helpers::create_top_left_container(
      context, elem.ent(), "character_top_left", 0);

  ui_helpers::create_styled_button(
      context, top_left.ent(), "round settings",
      []() { navigation::to(GameStateManager::Screen::RoundSettings); }, 0);

  ui_helpers::create_styled_button(
      context, top_left.ent(), "back", []() { navigation::back(); }, 1);

  size_t num_slots = players.size() + ais.size() + 1;
  // 0-4 => 1, 5->8 -> 2
  int fours = static_cast<int>(std::ceil(static_cast<float>(num_slots) / 4.f));

  auto btn_group = imm::div(
      context, mk(elem.ent()),
      ComponentConfig{}
          .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
          .with_margin(Margin{.top = screen_pct(fours == 1 ? 0.2f : 0.05f),
                              .left = screen_pct(0.2f),
                              .right = screen_pct(0.1f)})
          .with_absolute_position()
          .with_debug_name("btn_group"));

  for (int row_id = 0; row_id < fours; row_id++) {
    auto row = imm::div(
        context, mk(btn_group.ent(), row_id),
        ComponentConfig{}
            .with_size(ComponentSize{percent(1.f), percent(0.5f, 0.4f)})
            .with_flex_direction(FlexDirection::Row)
            .with_debug_name("row"));
    size_t start = row_id * 4;
    for (size_t i = start; i < std::min(num_slots, start + 4); i++) {
      character_selector_column(row.ent(), context, i, num_slots);
    }
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
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

  // fade_v computed above

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

void ScheduleMainMenuUI::for_each_with(Entity &entity,
                                       UIContext<InputAction> &context, float) {
  // Apply any queued screen changes at the start of the frame
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

void round_lives_settings(Entity &entity, UIContext<InputAction> &context) {
  auto &rl_settings = RoundManager::get().get_active_rt<RoundLivesSettings>();

  imm::div(context, mk(entity),
           ComponentConfig{}
               .with_label(
                   fmt::format("Num Lives: {}", rl_settings.num_starting_lives))
               .with_size(ComponentSize{percent(1.f), percent(0.2f)})
               .with_margin(Margin{.top = screen_pct(0.01f)}));
}

void round_kills_settings(Entity &entity, UIContext<InputAction> &context) {
  auto &rl_settings = RoundManager::get().get_active_rt<RoundKillsSettings>();

  imm::div(context, mk(entity),
           ComponentConfig{}
               .with_label(fmt::format("Round Length: {}",
                                       rl_settings.current_round_time))
               .with_size(ComponentSize{screen_pct(0.3f), screen_pct(0.06f)})
               .with_margin(Margin{.top = screen_pct(0.01f)}));

  {
    // TODO replace with actual strings
    auto options = magic_enum::enum_names<RoundSettings::TimeOptions>();
    auto option_index = magic_enum::enum_index(rl_settings.time_option).value();

    if (auto result =
            imm::dropdown(context, mk(entity), options, option_index,
                          ComponentConfig{}.with_label("Round Length"));
        result) {
      rl_settings.set_time_option(result.as<int>());
    }
  }
}

void round_hippo_settings(Entity &entity, UIContext<InputAction> &context) {
  auto &rl_settings = RoundManager::get().get_active_rt<RoundHippoSettings>();

  imm::div(
      context, mk(entity),
      ComponentConfig{}
          .with_label(fmt::format("Total Hippos: {}", rl_settings.total_hippos))
          .with_size(ComponentSize{percent(1.f), percent(0.2f)}));
}

void round_tag_and_go_settings(Entity &entity,
                               UIContext<InputAction> &context) {
  auto &cm_settings =
      RoundManager::get().get_active_rt<RoundTagAndGoSettings>();

  {
    auto options = magic_enum::enum_names<RoundSettings::TimeOptions>();
    auto option_index = magic_enum::enum_index(cm_settings.time_option).value();

    if (auto result =
            imm::dropdown(context, mk(entity), options, option_index,
                          ComponentConfig{}.with_label("Round Length"));
        result) {
      cm_settings.set_time_option(result.as<int>());
    }
  }

  if (imm::checkbox(context, mk(entity), cm_settings.allow_tag_backs,
                    ComponentConfig{}.with_label("Allow Tag Backs"))) {
    // value already toggled by checkbox binding
  }
}

Screen ScheduleMainMenuUI::round_settings(Entity &entity,
                                          UIContext<InputAction> &context) {
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_debug_name("round_settings")
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position());

  // Top-left controls scheduled first so "select map" gets initial focus
  {
    auto settings_group =
        imm::div(context, mk(elem.ent()),
                 ComponentConfig{}
                     .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                     .with_padding(Padding{.top = screen_pct(0.02f),
                                           .left = screen_pct(0.02f),
                                           .bottom = pixels(0.f),
                                           .right = pixels(0.f)})
                     .with_absolute_position()
                     .with_debug_name("round_settings_top_left"));

    ui_helpers::create_styled_button(
        context, settings_group.ent(), "select map",
        []() { navigation::to(GameStateManager::Screen::MapSelection); }, 0);

    {
      auto win_condition_div =
          imm::div(context, mk(settings_group.ent()),
                   ComponentConfig{}
                       .with_size(ComponentSize{percent(1.f), percent(0.2f)})
                       .with_debug_name("win_condition_div"));

      static size_t selected_round_type =
          static_cast<size_t>(RoundManager::get().active_round_type);

      if (auto result = imm::navigation_bar(
              context, mk(win_condition_div.ent()), RoundType_NAMES,
              selected_round_type, ComponentConfig{});
          result) {
        RoundManager::get().set_active_round_type(
            static_cast<int>(selected_round_type));
      }
    }

    // shared across all round types
    auto enabled_weapons = RoundManager::get().get_enabled_weapons();

    if (auto result = imm::checkbox_group(
            context, mk(settings_group.ent()), enabled_weapons,
            WEAPON_STRING_LIST, {1, 3},
            ComponentConfig{}
                .with_flex_direction(FlexDirection::Column)
                .with_margin(Margin{.top = screen_pct(0.01f)}));
        result) {
      auto mask = result.as<unsigned long>();
      log_info("weapon checkbox_group changed; mask={}", mask);
      RoundManager::get().set_enabled_weapons(mask);
    }

    switch (RoundManager::get().active_round_type) {
    case RoundType::Lives:
      round_lives_settings(settings_group.ent(), context);
      break;
    case RoundType::Kills:
      round_kills_settings(settings_group.ent(), context);
      break;
    case RoundType::Hippo:
      round_hippo_settings(settings_group.ent(), context);
      break;
    case RoundType::TagAndGo:
      round_tag_and_go_settings(settings_group.ent(), context);
      break;
    default:
      log_error("You need to add a handler for UI settings for round type {}",
                (int)RoundManager::get().active_round_type);
      break;
    }

    ui_helpers::create_styled_button(
        context, settings_group.ent(), "back", []() { navigation::back(); }, 2);
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::map_selection(Entity &entity,
                                         UIContext<InputAction> &context) {
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_flex_direction(FlexDirection::Row)
                   .with_absolute_position()
                   .with_debug_name("map_selection"));

  auto left_col =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(0.2f), percent(1.0f)})
                   .with_padding(Padding{.top = screen_pct(0.02f),
                                         .left = screen_pct(0.02f)})
                   .with_flex_direction(FlexDirection::Column)
                   .with_debug_name("map_selection_left"));

  auto preview_box =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(0.8f), percent(1.0f)})
                   .with_margin(Margin{.top = percent(0.05f),
                                       .bottom = percent(0.05f),
                                       .right = percent(0.05f)})
                   .with_opacity(0.0f)
                   .with_debug_name("preview_box")
                   .with_skip_tabbing(true));

  auto current_round_type = RoundManager::get().active_round_type;
  auto compatible_maps =
      MapManager::get().get_maps_for_round_type(current_round_type);
  auto selected_map_index = MapManager::get().get_selected_map();
  static int prev_preview_index = -2; // previous preview used for fade-out
  static int last_effective_preview_index = -2; // track last preview we showed

  constexpr int NO_PREVIEW_INDEX = -1000;
  int hovered_preview_index = NO_PREVIEW_INDEX;
  int focused_preview_index = NO_PREVIEW_INDEX;
  static int persisted_hovered_preview_index = NO_PREVIEW_INDEX;

  // Round settings preview above map list
  {
    auto round_preview = imm::div(
        context, mk(left_col.ent(), 1),
        ComponentConfig{}
            .with_debug_name("round_settings_preview")
            // TODO i really just want to do children() but then everything
            // below needs to not use percent... .
            .with_size(ComponentSize{percent(1.f), percent(0.3f, 0.5f)})
            .with_margin(Margin{.top = screen_pct(0.008f)}));

    render_round_settings_preview(context, round_preview.ent());
  }

  auto map_list =
      imm::div(context, mk(left_col.ent(), 2),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(1.f), percent(0.5f)})
                   .with_margin(Margin{.top = screen_pct(0.01f)})
                   .with_flex_direction(FlexDirection::Row)
                   .with_debug_name("map_list"));

  auto map_grid_button_size =
      ComponentSize{percent(0.48f), screen_pct(100.f / 720.f)};

  {
    float inner_margin = 0.01f;
    auto random_btn = imm::button(
        context,
        mk(map_list.ent(), static_cast<EntityID>(compatible_maps.size())),
        ComponentConfig{}
            .with_label("?")
            .with_size(map_grid_button_size)
            .with_margin(Margin{.top = percent(inner_margin),
                                .bottom = percent(inner_margin),
                                .left = percent(inner_margin),
                                .right = percent(inner_margin)})
            .with_flex_direction(FlexDirection::Row)
            .with_opacity(0.0f)
            .with_translate(-2000.0f, 0.0f)
            .with_debug_name("map_card_random"));
    // apply one-time slide-in from off-screen left, and persist final state
    {
      size_t random_index = compatible_maps.size();
      afterhours::animation::one_shot(
          UIKey::MapCard, random_index,
          ui_anims::make_map_card_slide(random_index));

      static int random_card_anim_state = 0; // 0:not started, 1:playing, 2:done
      float slide_v = 0.0f;
      if (auto mv =
              afterhours::animation::get_value(UIKey::MapCard, random_index);
          mv.has_value()) {
        slide_v = std::clamp(mv.value(), 0.0f, 1.0f);
        random_card_anim_state = 1;
      } else {
        if (random_card_anim_state == 1) {
          random_card_anim_state = 2;
          slide_v = 1.0f;
        } else if (random_card_anim_state == 2) {
          slide_v = 1.0f;
        } else {
          slide_v = 0.0f;
        }
      }

      auto opt_ent = afterhours::EntityHelper::getEntityForID(random_btn.id());
      if (opt_ent) {
        apply_slide_mods(opt_ent.asE(), slide_v);
      }
    }
    if (random_btn) {
      start_game_with_random_animation();
    }
    auto random_btn_id = random_btn.id();
    // TODO preview on hover via is_hot is delayed since hot is computed in
    // afterhours ui HandleClicks after this UI is built; use rect checks or
    // reorder systems if same-frame hover preview is needed
    if (context.is_hot(random_btn_id)) {
      hovered_preview_index = MapManager::RANDOM_MAP_INDEX;
      persisted_hovered_preview_index = hovered_preview_index;
    }
    if (context.has_focus(random_btn_id)) {
      focused_preview_index = MapManager::RANDOM_MAP_INDEX;
    }
    {
      auto opt_ent = afterhours::EntityHelper::getEntityForID(random_btn_id);
      if (opt_ent) {
        auto &ent = opt_ent.asE();
        if (ent.has<afterhours::ui::UIComponent>()) {
          auto rect = ent.get<afterhours::ui::UIComponent>().rect();
          auto mp = context.mouse_pos;
          if (mp.x >= rect.x && mp.x <= rect.x + rect.width && mp.y >= rect.y &&
              mp.y <= rect.y + rect.height) {
            hovered_preview_index = MapManager::RANDOM_MAP_INDEX;
            persisted_hovered_preview_index = hovered_preview_index;
          }
        }
      }
    }
  }

  static int map_card_anim_state[256] = {0}; // 0:not started, 1:playing, 2:done
  for (size_t i = 0; i < compatible_maps.size(); i++) {
    const auto &map_pair = compatible_maps[i];
    const auto &map_config = map_pair.second;
    int map_index = map_pair.first;

    // trigger once per app run
    afterhours::animation::one_shot(UIKey::MapCard, i,
                                    ui_anims::make_map_card_slide(i));

    // selection pulse value for this card (0..1 anim value)
    float pulse_v =
        afterhours::animation::get_value(UIKey::MapCardPulse, i).value_or(0.0f);
    float inner_margin_base = 0.02f;
    float inner_margin_scale = 0.004f;
    float inner_margin = inner_margin_base - (inner_margin_scale * pulse_v);

    float slide_v = 0.0f;
    if (auto mv = afterhours::animation::get_value(UIKey::MapCard, i);
        mv.has_value()) {
      slide_v = std::clamp(mv.value(), 0.0f, 1.0f);
      map_card_anim_state[i] = 1;
    } else {
      if (map_card_anim_state[i] == 1) {
        map_card_anim_state[i] = 2;
        slide_v = 1.0f;
      } else if (map_card_anim_state[i] == 2) {
        slide_v = 1.0f;
      } else {
        slide_v = 0.0f;
      }
    }
    // off-screen-left translation applied below per-entity
    auto map_btn =
        imm::button(context, mk(map_list.ent(), static_cast<EntityID>(i)),
                    ComponentConfig{}
                        .with_label(map_config.display_name)
                        .with_size(map_grid_button_size)
                        .with_margin(Margin{.top = percent(inner_margin),
                                            .bottom = percent(inner_margin),
                                            .left = percent(inner_margin),
                                            .right = percent(inner_margin)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_opacity(0.0f)
                        .with_translate(-2000.0f, 0.0f)
                        .with_debug_name("map_card"));
    if (map_btn) {
      MapManager::get().set_selected_map(map_index);
      MapManager::get().create_map();
      GameStateManager::get().start_game();
    }

    auto btn_id = map_btn.id();
    {
      auto opt_ent = afterhours::EntityHelper::getEntityForID(btn_id);
      if (opt_ent) {
        auto &ent = opt_ent.asE();
        auto &mods =
            ent.addComponentIfMissing<afterhours::ui::HasUIModifiers>();
        (void)mods; // ensure component exists before applying via helper
        apply_slide_mods(ent, slide_v);
      }
    }
    // TODO preview on hover via is_hot is delayed since hot is computed in
    // afterhours ui HandleClicks after this UI is built; use rect checks or
    // reorder systems if same-frame hover preview is needed
    if (context.is_hot(btn_id)) {
      hovered_preview_index = map_index;
      persisted_hovered_preview_index = hovered_preview_index;
    }
    if (context.has_focus(btn_id)) {
      focused_preview_index = map_index;
    }
    {
      auto opt_ent = afterhours::EntityHelper::getEntityForID(btn_id);
      if (opt_ent) {
        auto &ent = opt_ent.asE();
        if (ent.has<afterhours::ui::UIComponent>()) {
          auto rect = ent.get<afterhours::ui::UIComponent>().rect();
          float mod_x = rect.x;
          float mod_y = rect.y;
          float comp_tx = 0.0f;
          float comp_ty = 0.0f;
          if (ent.has<afterhours::ui::HasUIModifiers>()) {
            const auto &mods = ent.get<afterhours::ui::HasUIModifiers>();
            comp_tx = mods.translate_x;
            comp_ty = mods.translate_y;
            auto mod_rect = mods.apply_modifier(rect);
            mod_x = mod_rect.x;
            mod_y = mod_rect.y;
          }

          auto mp = context.mouse_pos;
          if (mp.x >= rect.x && mp.x <= rect.x + rect.width && mp.y >= rect.y &&
              mp.y <= rect.y + rect.height) {
            hovered_preview_index = map_index;
            persisted_hovered_preview_index = hovered_preview_index;
          }
        }
      }
    }
  }

  int effective_preview_index = selected_map_index;
  if (hovered_preview_index != NO_PREVIEW_INDEX) {
    effective_preview_index = hovered_preview_index;
  } else if (persisted_hovered_preview_index != NO_PREVIEW_INDEX) {
    effective_preview_index = persisted_hovered_preview_index;
  } else if (focused_preview_index != NO_PREVIEW_INDEX) {
    effective_preview_index = focused_preview_index;
  }

  if (effective_preview_index >= 0 && last_effective_preview_index < 0) {
    afterhours::animation::anim(UIKey::MapPreviewFade)
        .from(0.0f)
        .to(1.0f, 0.2f, afterhours::animation::EasingType::EaseOutQuad);
  } else if (effective_preview_index >= 0 &&
             last_effective_preview_index >= 0 &&
             effective_preview_index != last_effective_preview_index) {
    prev_preview_index = last_effective_preview_index;
    afterhours::animation::anim(UIKey::MapPreviewFade)
        .from(0.0f)
        .to(1.0f, 0.12f, afterhours::animation::EasingType::EaseOutQuad);
  }
  last_effective_preview_index = effective_preview_index;

  auto selected_map_it =
      std::find_if(compatible_maps.begin(), compatible_maps.end(),
                   [effective_preview_index](const auto &pair) {
                     return pair.first == effective_preview_index;
                   });

  auto maybe_shuffle =
      afterhours::animation::manager<UIKey>().get_value(UIKey::MapShuffle);
  bool overriding_preview = effective_preview_index != selected_map_index;
  render_map_preview(context, preview_box.ent(), effective_preview_index,
                     selected_map_index, compatible_maps, overriding_preview,
                     prev_preview_index);

  ui_helpers::create_styled_button(
      context, left_col.ent(), "back", []() { navigation::back(); }, 0);

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
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

Screen ScheduleMainMenuUI::main_screen(Entity &entity,
                                       UIContext<InputAction> &context) {
  auto elem =
      ui_helpers::create_screen_container(context, entity, "main_screen");
  auto top_left = ui_helpers::create_top_left_container(context, elem.ent(),
                                                        "main_top_left", 0);

  // Play button
  ui_helpers::create_styled_button(
      context, top_left.ent(), "play",
      []() { navigation::to(GameStateManager::Screen::CharacterCreation); }, 0);

  // About button
  ui_helpers::create_styled_button(
      context, top_left.ent(), "about",
      []() { navigation::to(GameStateManager::Screen::About); }, 1);

  // Settings button
  ui_helpers::create_styled_button(
      context, top_left.ent(), "settings",
      []() { navigation::to(GameStateManager::Screen::Settings); }, 2);

  // Exit button
  ui_helpers::create_styled_button(
      context, top_left.ent(), "exit", [this]() { exit_game(); }, 3);

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::settings_screen(Entity &entity,
                                           UIContext<InputAction> &context) {
  auto elem =
      ui_helpers::create_screen_container(context, entity, "settings_screen");
  {
    auto top_left = ui_helpers::create_top_left_container(
        context, elem.ent(), "settings_top_left", 0);
    ui_helpers::create_styled_button(
        context, top_left.ent(), "back",
        []() {
          Settings::get().update_resolution(
              EntityHelper::get_singleton_cmp<
                  window_manager::ProvidesCurrentResolution>()
                  ->current_resolution);
          navigation::back();
        },
        0);
  }
  auto control_group =
      ui_helpers::create_control_group(context, elem.ent(), "control_group");

  // Master volume slider
  {
    float master_volume = Settings::get().get_master_volume();
    ui_helpers::create_volume_slider(
        context, control_group.ent(), "Master Volume", master_volume,
        [](float volume) { Settings::get().update_master_volume(volume); }, 0);
  }

  // Music volume slider
  {
    float music_volume = Settings::get().get_music_volume();
    ui_helpers::create_volume_slider(
        context, control_group.ent(), "Music Volume", music_volume,
        [](float volume) { Settings::get().update_music_volume(volume); }, 1);
  }

  // SFX volume slider
  {
    float sfx_volume = Settings::get().get_sfx_volume();
    ui_helpers::create_volume_slider(
        context, control_group.ent(), "SFX Volume", sfx_volume,
        [](float volume) { Settings::get().update_sfx_volume(volume); }, 2);
  }

  // Resolution dropdown
  {
    if (imm::dropdown(context, mk(control_group.ent(), 3), resolution_strs,
                      resolution_index,
                      ComponentConfig{}.with_label("Resolution"))) {
      resolution_provider->on_data_changed(resolution_index);
    }
  }

  // Fullscreen checkbox
  if (imm::checkbox(context, mk(control_group.ent(), 4),
                    Settings::get().get_fullscreen_enabled(),
                    ComponentConfig{}.with_label("Fullscreen"))) {
    Settings::get().toggle_fullscreen();
  }

  // leave control group without the back button now that it's top-left

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::about_screen(Entity &entity,
                                        UIContext<InputAction> &context) {
  if (!current_resolution_provider)
    return GameStateManager::get().active_screen;

  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("about_screen"));

  {
    auto top_left = ui_helpers::create_top_left_container(context, elem.ent(),
                                                          "about_top_left", 0);
    ui_helpers::create_styled_button(
        context, top_left.ent(), "back", []() { navigation::back(); }, 0);
  }

  auto control_group =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_padding(button_group_padding)
                   .with_absolute_position()
                   .with_debug_name("control_group"));

  raylib::Texture2D sheet = EntityHelper::get_singleton_cmp<
                                afterhours::texture_manager::HasSpritesheet>()
                                ->texture;
  const auto scale = 5.f;

  const std::array<afterhours::texture_manager::Rectangle, 3> about_frames{
      afterhours::texture_manager::idx_to_sprite_frame(0, 4),
      afterhours::texture_manager::idx_to_sprite_frame(1, 4),
      afterhours::texture_manager::idx_to_sprite_frame(2, 4),
  };

  imm::icon_row(context, mk(control_group.ent()), sheet, about_frames, scale,
                ComponentConfig{}
                    .with_size(ComponentSize{percent(1.f), percent(0.4f)})
                    .with_margin(Margin{.top = percent(0.1f)})
                    .with_debug_name("about_icons"));

  // back button moved to top-left
  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

bool ScheduleDebugUI::should_run(float dt) {
  enableCooldown -= dt;

  if (enableCooldown < 0) {
    enableCooldown = enableCooldownReset;
    input::PossibleInputCollector<InputAction> inpc =
        input::get_input_collector<InputAction>();

    bool debug_pressed =
        std::ranges::any_of(inpc.inputs(), [](const auto &actions_done) {
          return actions_done.action == InputAction::ToggleUIDebug;
        });
    if (debug_pressed) {
      enabled = !enabled;
    }
  }
  return enabled;
}

///////
///////
///////
///////
///////
///////
///////
///////

void ScheduleDebugUI::for_each_with(Entity &entity,
                                    UIContext<InputAction> &context, float) {

  if (!enabled) {
    return;
  }

  struct SliderSpec {
    const char *debug_name;
    std::function<std::string()> make_label;
    std::function<float()> get_pct;
    std::function<void(float)> set_pct;
  };

  const std::array<SliderSpec, 11> all_specs{
      SliderSpec{"max_speed",
                 []() {
                   return fmt::format("Max Speed\n {:.2f} m/s",
                                      Config::get().max_speed.data);
                 },
                 []() { return Config::get().max_speed.get_pct(); },
                 [](float value) { Config::get().max_speed.set_pct(value); }},
      SliderSpec{"breaking_acceleration",
                 []() {
                   return fmt::format("Breaking \nPower \n -{:.2f} m/s^2",
                                      Config::get().breaking_acceleration.data);
                 },
                 []() { return Config::get().breaking_acceleration.get_pct(); },
                 [](float value) {
                   Config::get().breaking_acceleration.set_pct(value);
                 }},
      SliderSpec{"forward_acceleration",
                 []() {
                   return fmt::format("Forward \nAcceleration \n {:.2f} m/s^2",
                                      Config::get().forward_acceleration.data);
                 },
                 []() { return Config::get().forward_acceleration.get_pct(); },
                 [](float value) {
                   Config::get().forward_acceleration.set_pct(value);
                 }},
      SliderSpec{"reverse_acceleration",
                 []() {
                   return fmt::format("Reverse \nAcceleration \n {:.2f} m/s^2",
                                      Config::get().reverse_acceleration.data);
                 },
                 []() { return Config::get().reverse_acceleration.get_pct(); },
                 [](float value) {
                   Config::get().reverse_acceleration.set_pct(value);
                 }},
      SliderSpec{
          "boost_acceleration",
          []() {
            return fmt::format("Boost \nAcceleration \n {:.2f} m/s^2",
                               Config::get().boost_acceleration.data);
          },
          []() { return Config::get().boost_acceleration.get_pct(); },
          [](float value) { Config::get().boost_acceleration.set_pct(value); }},
      SliderSpec{"boost_decay_percent",
                 []() {
                   return fmt::format("Boost \nDecay \n {:.2f} decay%/frame",
                                      Config::get().boost_decay_percent.data);
                 },
                 []() { return Config::get().boost_decay_percent.get_pct(); },
                 [](float value) {
                   Config::get().boost_decay_percent.set_pct(value);
                 }},
      SliderSpec{
          "skid_threshold",
          []() {
            return fmt::format("Skid \nThreshold \n {:.2f} %",
                               Config::get().skid_threshold.data);
          },
          []() { return Config::get().skid_threshold.get_pct(); },
          [](float value) { Config::get().skid_threshold.set_pct(value); }},
      SliderSpec{"steering_sensitivity",
                 []() {
                   return fmt::format("Steering \nSensitivity \n {:.2f} %",
                                      Config::get().steering_sensitivity.data);
                 },
                 []() { return Config::get().steering_sensitivity.get_pct(); },
                 [](float value) {
                   Config::get().steering_sensitivity.set_pct(value);
                 }},
      SliderSpec{
          "minimum_steering_radius",
          []() {
            return fmt::format("Min Steering \nSensitivity \n {:.2f} m",
                               Config::get().minimum_steering_radius.data);
          },
          []() { return Config::get().minimum_steering_radius.get_pct(); },
          [](float value) {
            Config::get().minimum_steering_radius.set_pct(value);
          }},
      SliderSpec{
          "maximum_steering_radius",
          []() {
            return fmt::format("Max Steering \nSensitivity \n {:.2f} m",
                               Config::get().maximum_steering_radius.data);
          },
          []() { return Config::get().maximum_steering_radius.get_pct(); },
          [](float value) {
            Config::get().maximum_steering_radius.set_pct(value);
          }},
      SliderSpec{
          "collision_scalar",
          []() {
            return fmt::format("Collision \nScalar \n {:.4f}",
                               Config::get().collision_scalar.data);
          },
          []() { return Config::get().collision_scalar.get_pct(); },
          [](float value) { Config::get().collision_scalar.set_pct(value); }},
  };

  auto screen_container =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(0.5f)})
                   .with_absolute_position()
                   .with_debug_name("debug_screen_container"));

  const int items_per_row = 3;
  const int num_rows =
      static_cast<int>((all_specs.size() + items_per_row - 1) / items_per_row);
  for (int row = 0; row < num_rows; ++row) {
    const int start = row * items_per_row;
    const int remaining = static_cast<int>(all_specs.size()) - start;
    if (remaining <= 0)
      break;
    const int count_in_row = std::min(items_per_row, remaining);
    const float row_height = 1.f / static_cast<float>(num_rows);

    auto row_elem = imm::div(
        context, mk(screen_container.ent(), row),
        ComponentConfig{}
            .with_size(ComponentSize{percent(1.f), percent(row_height)})
            .with_flex_direction(FlexDirection::Row));

    for (int j = 0; j < count_in_row; ++j) {
      const auto &spec = all_specs[start + j];
      float pct = spec.get_pct();
      auto label = spec.make_label();
      if (auto result =
              slider(context, mk(row_elem.ent(), row * items_per_row + j), pct,
                     ComponentConfig{}
                         .with_size(ComponentSize{pixels(200.f), pixels(50.f)})
                         .with_label(std::move(label))
                         .with_skip_tabbing(true));
          result) {
        spec.set_pct(result.as<float>());
      }
    }
  }
}

bool SchedulePauseUI::should_run(float) {
  inpc = input::get_input_collector<InputAction>();
  return GameStateManager::get().is_game_active() ||
         GameStateManager::get().is_paused();
}

void SchedulePauseUI::for_each_with(Entity &entity,
                                    UIContext<InputAction> &context, float) {
  // Handle pause button input
  const bool pause_pressed =
      std::ranges::any_of(inpc.inputs_pressed(), [](const auto &actions_done) {
        return actions_done.action == InputAction::PauseButton;
      });

  if (pause_pressed) {
    if (GameStateManager::get().is_paused()) {
      GameStateManager::get().unpause_game();
      return;
    } else if (GameStateManager::get().is_game_active()) {
      GameStateManager::get().pause_game();
      return;
    }
  }

  // Only show pause UI when paused
  if (!GameStateManager::get().is_paused()) {
    return;
  }

  auto elem =
      ui_helpers::create_screen_container(context, entity, "pause_screen");

  auto left_col =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(0.2f), percent(1.0f)})
                   .with_padding(Padding{.top = screen_pct(0.02f),
                                         .left = screen_pct(0.02f)})
                   .with_flex_direction(FlexDirection::Column)
                   .with_debug_name("pause_left"));

  imm::div(context, mk(left_col.ent(), 0),
           ComponentConfig{}
               .with_label("paused")
               .with_font(get_font_name(FontID::EQPro), 100.f)
               .with_skip_tabbing(true)
               .with_size(ComponentSize{pixels(400.f), pixels(100.f)}));

  ui_helpers::create_styled_button(
      context, left_col.ent(), "resume",
      []() { GameStateManager::get().unpause_game(); }, 1);

  ui_helpers::create_styled_button(
      context, left_col.ent(), "back to setup",
      []() { GameStateManager::get().end_game(); }, 2);

  ui_helpers::create_styled_button(
      context, left_col.ent(), "exit game", [this]() { exit_game(); }, 3);
}

Screen ScheduleMainMenuUI::round_end_screen(Entity &entity,
                                            UIContext<InputAction> &context) {
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("round_end_screen"));

  // Get players from the round (filter out entities marked for cleanup)
  std::vector<OptEntity> round_players;
  std::vector<OptEntity> round_ais;

  try {
    auto round_players_ref =
        EQ(EntityQuery<EQ>::QueryOptions{.ignore_temp_warning = true})
            .whereHasComponent<PlayerID>()
            .orderByPlayerID()
            .gen();
    for (const auto &player_ref : round_players_ref) {
      if (!player_ref.get().cleanup) {
        round_players.push_back(OptEntity{player_ref.get()});
      }
    }
  } catch (...) {
    // If query fails, just continue with empty list
  }

  try {
    auto round_ais_ref =
        EQ(EntityQuery<EQ>::QueryOptions{.ignore_temp_warning = true})
            .whereHasComponent<AIControlled>()
            .gen();
    for (const auto &ai_ref : round_ais_ref) {
      if (!ai_ref.get().cleanup) {
        round_ais.push_back(OptEntity{ai_ref.get()});
      }
    }
  } catch (...) {
    // If query fails, just continue with empty list
  }

  // Title
  {
    imm::div(context, mk(elem.ent()),
             ComponentConfig{}
                 .with_label("Round End")
                 .with_font(get_font_name(FontID::EQPro), 100.f)
                 .with_skip_tabbing(true)
                 .with_size(ComponentSize{pixels(400.f), pixels(100.f)})
                 .with_margin(Margin{.top = screen_pct(0.05f)}));
  }

  std::map<EntityID, int> rankings;
  if (RoundManager::get().active_round_type == RoundType::TagAndGo) {
    rankings = get_tag_and_go_rankings(round_players, round_ais);
  }

  // Render players in a grid layout similar to character creation
  size_t num_slots = round_players.size() + round_ais.size();
  if (num_slots > 0) {
    int fours =
        static_cast<int>(std::ceil(static_cast<float>(num_slots) / 4.f));

    auto player_group = imm::div(
        context, mk(elem.ent()),
        ComponentConfig{}
            .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
            .with_margin(Margin{.top = screen_pct(fours == 1 ? 0.3f : 0.15f),
                                .left = screen_pct(0.2f),
                                .right = screen_pct(0.1f)})
            .with_absolute_position()
            .with_debug_name("player_group"));

    for (int row_id = 0; row_id < fours; row_id++) {
      auto row = imm::div(
          context, mk(player_group.ent(), row_id),
          ComponentConfig{}
              .with_size(ComponentSize{percent(1.f), percent(0.5f, 0.4f)})
              .with_flex_direction(FlexDirection::Row)
              .with_debug_name("row"));
      size_t start = row_id * 4;
      for (size_t i = start; i < std::min(num_slots, start + 4); i++) {
        OptEntity car;
        if (i < round_players.size()) {
          car = round_players[i];
        } else {
          car = round_ais[i - round_players.size()];
        }

        std::optional<int> ranking;
        if (car.has_value() &&
            RoundManager::get().active_round_type == RoundType::TagAndGo) {
          auto it = rankings.find(car->id);
          if (it != rankings.end() && it->second <= 3) {
            ranking = it->second;
          }
        }

        round_end_player_column(row.ent(), context, i, round_players, round_ais,
                                ranking);
      }
    }
  }

  // Button group at bottom
  auto button_group =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("round_end_button_group"));

  {
    if (imm::button(context, mk(button_group.ent()),
                    ComponentConfig{}.with_label("continue"))) {
      navigation::to(GameStateManager::Screen::CharacterCreation);
    }
  }

  {
    if (imm::button(context, mk(button_group.ent()),
                    ComponentConfig{}.with_label("quit"))) {
      exit_game();
    }
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}
