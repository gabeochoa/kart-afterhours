

#include <afterhours/ah.h>
#include <fmt/format.h>

#include <afterhours/src/logging.h>

//

#include "../components.h"
#include "../config.h"
#include "../game.h"
#include "../game_state_manager.h"
#include "../map_system.h"
#include "../preload.h" // FontID
#include "../query.h"
#include "../round_settings.h"
#include "../settings.h"
#include "../strings.h"
#include "../texture_library.h"
#include "../translation_manager.h"
#include "animation_key.h"
#include "animation_slide_in.h"
#include "animation_ui_wiggle.h"
#include "navigation.h"

using namespace afterhours;

struct MapConfig;

using namespace afterhours::ui;
using namespace afterhours::ui::imm;
using Screen = GameStateManager::Screen;

struct SetupGameStylingDefaults
    : System<afterhours::ui::UIContext<InputAction>> {

  virtual void once(float) override {
    auto &styling_defaults = afterhours::ui::imm::UIStylingDefaults::get();

    // 80s Synthwave Theme
    styling_defaults
        .set_theme_color(afterhours::ui::Theme::Usage::Primary,
                         afterhours::Color{96, 0, 255, 255}) // Deep purple
        .set_theme_color(afterhours::ui::Theme::Usage::Secondary,
                         afterhours::Color{0, 224, 255, 255}) // Electric blue
        .set_theme_color(afterhours::ui::Theme::Usage::Accent,
                         afterhours::Color{255, 44, 156, 255}) // Hot pink
        .set_theme_color(afterhours::ui::Theme::Usage::Background,
                         afterhours::Color{23, 7, 26, 255}) // Deep purple-black
        .set_theme_color(
            afterhours::ui::Theme::Usage::Font,
            afterhours::Color{225, 225, 255, 255}) // Soft blue-white
        .set_theme_color(
            afterhours::ui::Theme::Usage::DarkFont,
            afterhours::Color{255, 44, 156,
                              255}); // Hot pink for dark backgrounds

    // Set the default font for all components based on current language
    styling_defaults.set_default_font(
        get_font_name(translation_manager::get_font_for_language()), 16.f);

    // Enable grid snapping for consistent 8pt grid spacing
    styling_defaults.set_grid_snapping(true);

    // Enable TV safe area validation
    styling_defaults.enable_tv_safe_validation();

    // Component-specific styling
    styling_defaults.set_component_config(
        ComponentType::Button,
        ComponentConfig{}
            .with_size(ComponentSize{w1280(200.f), h720(50.f)})
            .with_background(Theme::Usage::Primary));

    styling_defaults.set_component_config(
        ComponentType::Slider,
        ComponentConfig{}
            .with_size(ComponentSize{w1280(200.f), h720(50.f)})
            .with_background(Theme::Usage::Secondary));

    styling_defaults.set_component_config(
        ComponentType::Checkbox,
        ComponentConfig{}
            .with_size(ComponentSize{w1280(200.f), h720(50.f)})
            .with_background(Theme::Usage::Primary));

    styling_defaults.set_component_config(
        ComponentType::CheckboxNoLabel,
        ComponentConfig{}
            .with_size(ComponentSize{w1280(200.f), h720(50.f)})
            .with_background(Theme::Usage::Primary));

    styling_defaults.set_component_config(
        ComponentType::Dropdown,
        ComponentConfig{}
            .with_size(ComponentSize{w1280(200.f), h720(50.f)})
            .with_background(Theme::Usage::Primary));

    styling_defaults.set_component_config(
        ComponentType::NavigationBar,
        ComponentConfig{}
            .with_size(ComponentSize{w1280(200.f), h720(50.f)})
            .with_background(Theme::Usage::Primary));
  }
};

struct ScheduleDebugUI : System<afterhours::ui::UIContext<InputAction>> {
  bool enabled = false;
  float enableCooldown = 0.f;
  float enableCooldownReset = 0.2f;

  virtual bool should_run(float dt) override;
  virtual void for_each_with(Entity &entity,
                             afterhours::ui::UIContext<InputAction> &context,
                             float) override;
};

struct SchedulePauseUI : System<afterhours::ui::UIContext<InputAction>> {
  input::PossibleInputCollector inpc;

  void exit_game() { running = false; }

  virtual bool should_run(float) override;
  virtual void for_each_with(Entity &entity,
                             afterhours::ui::UIContext<InputAction> &context,
                             float) override;
};

struct ScheduleMainMenuUI : System<afterhours::ui::UIContext<InputAction>> {

  Screen get_active_screen() { return GameStateManager::get().active_screen; }

  void set_active_screen(Screen screen) {
    GameStateManager::get().set_screen(screen);
  }

  // settings cache stuff for now
  window_manager::ProvidesAvailableWindowResolutions *resolution_provider{
      nullptr}; // non owning ptr
                // eventually std::observer_ptr?
  window_manager::ProvidesCurrentResolution *current_resolution_provider{
      nullptr}; // non owning ptr
                // eventually std::observer_ptr?
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
  void render_lives_stats(UIContext<InputAction> &context, Entity &parent,
                          const OptEntity &car, raylib::Color bg_color);
  void render_kills_stats(UIContext<InputAction> &context, Entity &parent,
                          const OptEntity &car, raylib::Color bg_color);
  void render_score_stats(UIContext<InputAction> &context, Entity &parent,
                          const OptEntity &car, raylib::Color bg_color);
  void render_hippo_stats(UIContext<InputAction> &context, Entity &parent,
                          const OptEntity &car, raylib::Color bg_color);
  void render_tag_and_go_stats(UIContext<InputAction> &context, Entity &parent,
                               const OptEntity &car, raylib::Color bg_color);
  void render_unknown_stats(UIContext<InputAction> &context, Entity &parent,
                            const OptEntity &car, raylib::Color bg_color);
  void render_team_column(UIContext<InputAction> &context,
                          Entity &team_columns_container,
                          const std::string &team_name,
                          const std::vector<size_t> &team_players,
                          const size_t num_slots, int team_index);
  void render_team_results(UIContext<InputAction> &context, Entity &parent,
                           const std::vector<OptEntity> &round_players,
                           const std::vector<OptEntity> &round_ais);
  void render_team_column_results(UIContext<InputAction> &context,
                                  Entity &parent, const std::string &team_name,
                                  int team_id,
                                  const std::vector<OptEntity> &team_players,
                                  int team_score);
  void start_game_with_random_animation();

  Screen character_creation(Entity &entity, UIContext<InputAction> &context);
  Screen map_selection(Entity &entity, UIContext<InputAction> &context);
  Screen round_settings(Entity &entity, UIContext<InputAction> &context);
  Screen main_screen(Entity &entity, UIContext<InputAction> &context);
  Screen settings_screen(Entity &entity, UIContext<InputAction> &context);
  Screen about_screen(Entity &entity, UIContext<InputAction> &context);
  Screen round_end_screen(Entity &entity, UIContext<InputAction> &context);

  void render_map_preview(
      UIContext<InputAction> &context, Entity &preview_box,
      int effective_preview_index, int selected_map_index,
      const std::vector<std::pair<int, MapConfig>> &compatible_maps,
      bool overriding_preview, int prev_preview_index);

  void render_round_settings_preview(UIContext<InputAction> &context,
                                     Entity &parent);

  void exit_game() { running = false; }

  virtual void once(float) override;
  virtual bool should_run(float) override;
  virtual void for_each_with(Entity &entity, UIContext<InputAction> &context,
                             float) override;
};

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

// Reusable UI component functions
namespace ui_helpers {

struct PlayerCardData {
  Entity &parent;
  int index;
  const std::string &label;
  raylib::Color bg_color;
  bool is_ai = false;

  std::optional<int> ranking = std::nullopt;
  std::optional<std::string> stats_text = std::nullopt;

  std::function<void()> on_next_color = nullptr;
  std::function<void()> on_remove = nullptr;
  std::function<void()> on_add_ai = nullptr;
  std::function<void()> on_team_switch = nullptr;
  std::optional<AIDifficulty::Difficulty> ai_difficulty = std::nullopt;
  std::function<void(AIDifficulty::Difficulty)> on_difficulty_change = nullptr;
};

ElementResult player_card_cell(UIContext<InputAction> &context, Entity &parent,
                               const std::string &debug_name,
                               float width_percent = 0.125f) {
  return imm::div(
      context, mk(parent, std::hash<std::string>{}(debug_name + "_cell")),
      ComponentConfig{}
          .with_size(ComponentSize{percent(width_percent), percent(1.f)})
          .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                .left = imm::DefaultSpacing::tiny(),
                                .bottom = imm::DefaultSpacing::tiny(),
                                .right = imm::DefaultSpacing::tiny()})
          .with_debug_name(debug_name + "_cell"));
}

void maybe_button(UIContext<InputAction> &context, Entity &parent,
                  const std::string &label, const std::string &debug_name,
                  std::function<void()> action = nullptr,
                  float width_percent = 0.125f) {

  if (!action) {
    return;
  }

  auto button_cell =
      player_card_cell(context, parent, debug_name, width_percent);

  if (imm::button(context,
                  mk(button_cell.ent(),
                     std::hash<std::string>{}(debug_name + "_button")),
                  ComponentConfig{}
                      .with_size(ComponentSize{percent(1.f), percent(1.f)})
                      .with_label(label)
                      .with_debug_name(debug_name + "_button"))) {
    action();
  }
}

void maybe_image_button(UIContext<InputAction> &context, Entity &parent,
                        const std::string &debug_name, raylib::Texture2D sheet,
                        Rectangle spriteSheetSrc,
                        std::function<void()> action = nullptr,
                        float width_percent = 0.125f) {

  if (!action) {
    return;
  }
  auto button_cell =
      player_card_cell(context, parent, debug_name, width_percent);

  if (imm::image_button(
          context,
          mk(button_cell.ent(),
             std::hash<std::string>{}(debug_name + "_button")),
          sheet, spriteSheetSrc,
          ComponentConfig{}
              .with_size(ComponentSize{percent(1.f), percent(1.f)})
              .with_debug_name(debug_name))) {
    action();
  }
}

void maybe_difficulty_button(UIContext<InputAction> &context, Entity &parent,
                             PlayerCardData &data) {
  if (data.ai_difficulty.has_value() && data.on_difficulty_change) {
    auto difficulty_options = std::vector<std::string>{
        translation_manager::make_translatable_string(strings::i18n::easy)
            .get_text(),
        translation_manager::make_translatable_string(strings::i18n::medium)
            .get_text(),
        translation_manager::make_translatable_string(strings::i18n::hard)
            .get_text(),
        translation_manager::make_translatable_string(strings::i18n::expert)
            .get_text()};
    auto current_difficulty = static_cast<size_t>(data.ai_difficulty.value());

    auto difficulty_cell =
        player_card_cell(context, parent, "difficulty_cell", 0.7f);

    if (auto result = imm::navigation_bar(
            context, mk(difficulty_cell.ent()), difficulty_options,
            current_difficulty,
            ComponentConfig{}
                .with_size(ComponentSize{percent(1.f), percent(1.f)})
                .disable_rounded_corners()
                .with_debug_name("ai_difficulty_navigation_bar"))) {
      data.on_difficulty_change(
          static_cast<AIDifficulty::Difficulty>(current_difficulty));
    }
  }
}

void maybe_next_color_button(UIContext<InputAction> &context, Entity &parent,
                             PlayerCardData &data) {
  raylib::Texture2D sheet = EntityHelper::get_singleton_cmp<
                                afterhours::texture_manager::HasSpritesheet>()
                                ->texture;

  maybe_image_button(context, parent, "next_color", sheet,
                     afterhours::texture_manager::idx_to_sprite_frame(0, 6),
                     data.on_next_color);
}

void maybe_ai_buttons(UIContext<InputAction> &context, Entity &parent,
                      PlayerCardData &data) {

  auto bottom_row = imm::div(
      context, mk(parent),
      ComponentConfig{}
          .with_size(ComponentSize{percent(1.f, 1.f), percent(0.4f, 1.f)})
          .with_flex_direction(FlexDirection::Row)
          .with_debug_name("player_card_bottom_row"));
  if (data.is_ai) {
    maybe_difficulty_button(context, bottom_row.ent(), data);

    if (data.on_difficulty_change) {
      imm::div(
          context, mk(bottom_row.ent()),
          ComponentConfig{}
              .with_size(ComponentSize{percent(0.15f, 0.1f), percent(1.f)})
              .with_debug_name("spacer"));
    }

    auto &trash_tex = TextureLibrary::get().get("trashcan");
    raylib::Rectangle src{0.f, 0.f, static_cast<float>(trash_tex.width),
                          static_cast<float>(trash_tex.height)};
    maybe_image_button(context, bottom_row.ent(), "delete", trash_tex, src,
                       data.on_remove);
  }

  auto &dollar_tex = TextureLibrary::get().get("dollar_sign");
  raylib::Rectangle src{0.f, 0.f, static_cast<float>(dollar_tex.width),
                        static_cast<float>(dollar_tex.height)};
  maybe_image_button(context, bottom_row.ent(), "add_ai", dollar_tex, src,
                     data.on_add_ai, 1.f);
}

// Reusable player card component
ElementResult create_player_card(UIContext<InputAction> &context,
                                 Entity &parent, PlayerCardData &data) {

  auto card = imm::div(context, mk(parent),
                       ComponentConfig{}
                           .with_size(ComponentSize{percent(1.f), percent(1.f)})
                           .with_custom_background(data.bg_color)
                           .disable_rounded_corners());

  // Top row: ID [color] [team switch]
  auto top_row =
      imm::div(context, mk(card.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(1.0f), percent(0.4f)})
                   .with_margin(Margin{.top = imm::DefaultSpacing::tiny(),
                                       .left = imm::DefaultSpacing::tiny(),
                                       .bottom = imm::DefaultSpacing::tiny(),
                                       .right = imm::DefaultSpacing::tiny()})
                   .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                         .left = imm::DefaultSpacing::tiny(),
                                         .bottom = imm::DefaultSpacing::tiny(),
                                         .right = imm::DefaultSpacing::tiny()})
                   .with_flex_direction(FlexDirection::Row)
                   .with_debug_name("player_card_top_row"));

  // Player ID label
  imm::div(context, mk(top_row.ent()),
           ComponentConfig{}
               .with_size(ComponentSize{percent(0.2f), percent(1.f)})
               .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                     .left = imm::DefaultSpacing::tiny(),
                                     .bottom = imm::DefaultSpacing::tiny(),
                                     .right = imm::DefaultSpacing::tiny()})
               .with_label(data.label)
               .with_custom_background(data.bg_color)
               .disable_rounded_corners()
               .with_debug_name("player_id_label"));

  maybe_next_color_button(context, top_row.ent(), data);
  maybe_button(context, top_row.ent(), "<->", "team_switch",
               data.on_team_switch);
  maybe_ai_buttons(context, card.ent(), data);

  return {true, card.ent()};
}

// Reusable styled button component
ElementResult create_styled_button(UIContext<InputAction> &context,
                                   Entity &parent, const std::string &label,
                                   std::function<void()> on_click,
                                   int index = 0) {

  if (imm::button(
          context, mk(parent, index),
          ComponentConfig{}
              .with_label(label)
              .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                    .left = imm::DefaultSpacing::tiny(),
                                    .bottom = imm::DefaultSpacing::tiny(),
                                    .right = imm::DefaultSpacing::tiny()})
              .with_opacity(0.0f)
              .with_translate(-2000.0f, 0.0f))) {
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

  if (auto result = slider(context, mk(parent, index), volume,
                           ComponentConfig{}
                               .with_size(ComponentSize{pixels(400.f), pixels(40.f)})
                               .with_label(label)
                               .with_padding(
                                   Padding{.top = spacing_to_size(Spacing::xs),
                                           .left = pixels(0.f),
                                           .bottom = spacing_to_size(Spacing::xs),
                                           .right = pixels(0.f)}),
                           SliderHandleValueLabelPosition::OnHandle)) {
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
          .with_padding(Padding{.top = imm::DefaultSpacing::large(),
                                .left = imm::DefaultSpacing::large(),
                                .bottom = imm::DefaultSpacing::large(),
                                .right = imm::DefaultSpacing::large()})
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
          .with_padding(Padding{.top = imm::DefaultSpacing::medium(),
                                .left = imm::DefaultSpacing::medium(),
                                .bottom = pixels(0.f),
                                .right = pixels(0.f)})
          .with_absolute_position()
          .with_debug_name(debug_name));
}

} // namespace ui_helpers

using Screen = GameStateManager::Screen;

Padding button_group_padding = Padding{
    .top = imm::DefaultSpacing::large(),
    .left = imm::DefaultSpacing::large(),
    .bottom = imm::DefaultSpacing::large(),
    .right = imm::DefaultSpacing::large(),
};

Padding control_group_padding = Padding{
    .top = imm::DefaultSpacing::large(),
    .left = imm::DefaultSpacing::large(),
    .bottom = imm::DefaultSpacing::large(),
    .right = imm::DefaultSpacing::large(),
};

Padding button_padding = Padding{
    .top = imm::DefaultSpacing::tiny(),
    .left = imm::DefaultSpacing::tiny(),
    .bottom = imm::DefaultSpacing::tiny(),
    .right = imm::DefaultSpacing::tiny(),
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
    inpc = input::get_input_collector();
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

  bool team_mode = RoundManager::get().get_active_settings().team_mode_enabled;

  if (is_last_slot && (players.size() + ais.size()) >= input::MAX_GAMEPAD_ID) {
    return;
  }

  float card_width = 400.f;
  float card_height = 100.f;

  auto column = imm::div(context, mk(parent, (int)index),
                         ComponentConfig{}
                             .with_size(ComponentSize{ui::w1280(card_width),
                                                      ui::h720(card_height)})
                             .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                                   .left = imm::DefaultSpacing::tiny(),
                                                   .bottom = imm::DefaultSpacing::tiny(),
                                                   .right = imm::DefaultSpacing::tiny()})
                             .with_custom_background(bg_color)
                             .disable_rounded_corners());

  // Create player card using helper function
  std::string label = car.has_value() ? fmt::format("{} {}", index, car->id)
                                      : fmt::format("{} Empty", index);

  // Add team indicator to label in team mode
  if (team_mode && car.has_value() && car->has<TeamID>()) {
    int team_id = car->get<TeamID>().team_id;
    std::string team_letter = (team_id == 0) ? "A" : "B";
    label = fmt::format("{} {} ({})", index, car->id, team_letter);
  }

  bool player_right = false;
  if (index < players.size()) {
    for (const auto &actions_done : inpc.inputs_pressed()) {
      if (static_cast<size_t>(actions_done.id) != index) {
        continue;
      }

      if (actions_done.medium == input::DeviceMedium::GamepadAxis) {
        continue;
      }

      player_right |=
          action_matches(actions_done.action, InputAction::WidgetRight);

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

  // Team switching functionality for team mode
  std::function<void()> on_team_switch = nullptr;
  bool show_team_switch = false;

  if (team_mode && car.has_value()) {
    show_team_switch = true;

    // Initialize team assignment if not set
    if (!car->has<TeamID>()) {
      int initial_team = (index % 2 == 0) ? 0 : 1; // Alternate teams
      car->addComponent<TeamID>(initial_team);
    }

    on_team_switch = [&car]() {
      if (car.has_value() && car->has<TeamID>()) {
        // Toggle team assignment
        int current_team = car->get<TeamID>().team_id;
        int new_team = (current_team == 0) ? 1 : 0;
        car->get<TeamID>().team_id = new_team;
        log_info("Player {} switched to team {}", car->id, new_team);
      }
    };
  }

  ui_helpers::PlayerCardData data{
      .parent = column.ent(),
      .index = 0,
      .label = label,
      .bg_color = bg_color,
      .is_ai = is_slot_ai,
      .on_next_color = on_next_color,
      .on_remove = on_remove,
      .on_add_ai = show_add_ai ? on_add_ai : nullptr,
      .ai_difficulty = ai_difficulty,
      .on_difficulty_change = on_difficulty_change,
      .on_team_switch = show_team_switch ? on_team_switch : nullptr,
  };

  ui_helpers::create_player_card(context, column.ent(), data);
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
                   .with_margin(Spacing::xs)
                   .with_custom_background(bg_color)
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
      stats_text = translation_manager::translate_formatted(
          translation_manager::make_translatable_string(
              strings::i18n::lives_label)
              .set_param(translation_manager::i18nParam::number_count,
                         car->get<HasMultipleLives>().num_lives_remaining,
                         translation_manager::translation_param));
    }
    break;
  case RoundType::Kills:
    if (car->has<HasKillCountTracker>()) {
      stats_text = translation_manager::translate_formatted(
          translation_manager::make_translatable_string(
              strings::i18n::kills_label)
              .set_param(translation_manager::i18nParam::number_count,
                         car->get<HasKillCountTracker>().kills,
                         translation_manager::translation_param));
    }
    break;
  case RoundType::Hippo:
    if (car->has<HasHippoCollection>()) {
      stats_text = translation_manager::translate_formatted(
          translation_manager::make_translatable_string(
              strings::i18n::hippos_label)
              .set_param(translation_manager::i18nParam::number_count,
                         car->get<HasHippoCollection>().get_hippo_count(),
                         translation_manager::translation_param));
    } else {
      stats_text = translation_manager::make_translatable_string(
                       strings::i18n::hippos_zero)
                       .get_text();
    }
    break;
  case RoundType::TagAndGo:
    if (car->has<HasTagAndGoTracking>()) {
      stats_text = translation_manager::translate_formatted(
          translation_manager::make_translatable_string(
              strings::i18n::not_it_timer)
              .set_param(translation_manager::i18nParam::number_time,
                         car->get<HasTagAndGoTracking>().time_as_not_it,
                         translation_manager::translation_param));
    }
    break;
  default:
    stats_text =
        translation_manager::make_translatable_string(strings::i18n::unknown)
            .get_text();
    break;
  }

  std::optional<std::string> kills_text;
  if (car->has<HasKillCountTracker>()) {
    kills_text = translation_manager::translate_formatted(
        translation_manager::make_translatable_string(
            strings::i18n::kills_label)
            .set_param(translation_manager::i18nParam::number_count,
                       car->get<HasKillCountTracker>().kills,
                       translation_manager::translation_param));
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
      animated_stats = translation_manager::translate_formatted(
          translation_manager::make_translatable_string(
              strings::i18n::lives_label)
              .set_param(translation_manager::i18nParam::number_count, shown,
                         translation_manager::translation_param));
    }
    break;
  }
  case RoundType::Kills: {
    if (car->has<HasKillCountTracker>()) {
      int final_val = car->get<HasKillCountTracker>().kills;
      int shown = static_cast<int>(std::round(score_t * final_val));
      animated_stats = translation_manager::translate_formatted(
          translation_manager::make_translatable_string(
              strings::i18n::kills_label)
              .set_param(translation_manager::i18nParam::number_count, shown,
                         translation_manager::translation_param));
    }
    break;
  }
  case RoundType::Hippo: {
    int final_val = car->has<HasHippoCollection>()
                        ? car->get<HasHippoCollection>().get_hippo_count()
                        : 0;
    int shown = static_cast<int>(std::round(score_t * final_val));
    animated_stats = translation_manager::translate_formatted(
        translation_manager::make_translatable_string(
            strings::i18n::hippos_label)
            .set_param(translation_manager::i18nParam::number_count, shown,
                       translation_manager::translation_param));
    break;
  }
  case RoundType::TagAndGo: {
    if (car->has<HasTagAndGoTracking>()) {
      float final_val = car->get<HasTagAndGoTracking>().time_as_not_it;
      float shown = std::round(score_t * final_val * 10.0f) / 10.0f;
      animated_stats = translation_manager::translate_formatted(
          translation_manager::make_translatable_string(
              strings::i18n::not_it_timer)
              .set_param(translation_manager::i18nParam::number_time, shown,
                         translation_manager::translation_param));
    }
    break;
  }
  default: {
    break;
  }
  }

  // Combine round-specific stats with kill count
  std::optional<std::string> combined_stats;
  std::string final_stats_text =
      animated_stats.has_value()
          ? animated_stats.value()
          : (stats_text.has_value() ? stats_text.value() : "");

  if (kills_text.has_value()) {
    if (!final_stats_text.empty()) {
      combined_stats = final_stats_text + " | " + kills_text.value();
    } else {
      combined_stats = kills_text.value();
    }
  } else if (!final_stats_text.empty()) {
    combined_stats = final_stats_text;
  }

  ui_helpers::PlayerCardData data{
      .parent = column.ent(),
      .index = 0,
      .label = player_label,
      .bg_color = bg_color,
      .is_ai = is_slot_ai,
      .ranking = ranking,
      .stats_text = combined_stats,
  };

  ui_helpers::create_player_card(context, column.ent(), data);
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

void ScheduleMainMenuUI::render_team_column(
    UIContext<InputAction> &context, Entity &team_columns_container,
    const std::string &team_name, const std::vector<size_t> &team_players,
    const size_t num_slots, int team_index) {
  auto team_color =
      team_index == 0
          ? raylib::Color{100, 150, 255, 50}  // Light blue for Team A
          : raylib::Color{255, 150, 100, 50}; // Light orange for Team B

  auto column_container =
      imm::div(context, mk(team_columns_container, team_index),
               ComponentConfig{}
                   .with_size(ComponentSize{ui::w1280(400.f), ui::h720(700.f)})
                   .with_flex_direction(FlexDirection::Column)
                   .with_padding(Padding{.left = ui::w1280(20.f),
                                         .right = ui::w1280(20.f)})
                   .with_custom_background(team_color)
                   .disable_rounded_corners()
                   .with_debug_name(team_name + "_column"));

  imm::div(context, mk(column_container.ent(), team_index),
           ComponentConfig{}
               .with_size(ComponentSize{ui::w1280(400.f), ui::h720(100.f)})
               .with_label(team_name)
               .with_debug_name(team_name + "_header"));

  ////
  if (team_players.empty()) {
    imm::div(context, mk(column_container.ent(), team_index),
             ComponentConfig{}
                 .with_size(ComponentSize{ui::w1280(400.f), ui::h720(700.f)})
                 .with_label("No players")
                 .with_debug_name(team_name + "_empty"));
  } else {
    int cards_per_row = 1;
    size_t team_rows = team_players.size();

    for (size_t row_id = 0; row_id < team_rows; row_id++) {
      auto team_row = imm::div(
          context, mk(column_container.ent(), row_id),
          ComponentConfig{}
              .with_size(ComponentSize{ui::w1280(400.f),
                                       // Cap row height at 300/720 pixels
                                       // (about 42% of screen height)
                                       ui::h720(100.f)})
              .with_flex_direction(FlexDirection::Row)
              .with_debug_name(team_name + "_row"));

      // Render players for this row
      size_t start = row_id * cards_per_row;
      for (size_t i = start;
           i < std::min(team_players.size(), start + cards_per_row); i++) {
        character_selector_column(team_row.ent(), context, team_players[i],
                                  num_slots);
      }
    }
  }
}

Screen ScheduleMainMenuUI::character_creation(Entity &entity,
                                              UIContext<InputAction> &context) {
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("character_creation"));

  auto top_left = ui_helpers::create_top_left_container(
      context, elem.ent(), "character_top_left", 0);

  ui_helpers::create_styled_button(
      context, top_left.ent(),
      translation_manager::make_translatable_string(
          strings::i18n::round_settings)
          .get_text(),
      []() { navigation::to(GameStateManager::Screen::RoundSettings); }, 0);

  ui_helpers::create_styled_button(
      context, top_left.ent(),
      translation_manager::make_translatable_string(strings::i18n::back)
          .get_text(),
      []() { navigation::back(); }, 1);

  // Team mode toggle
  auto &active_settings = RoundManager::get().get_active_settings();
  if (imm::checkbox(context, mk(top_left.ent()),
                    active_settings.team_mode_enabled,
                    ComponentConfig{}
                        .with_label("Team Mode")
                        .with_margin(Margin{.top = screen_pct(0.01f)}))) {
    // Value already toggled by checkbox binding
    log_info("team mode toggled: {}", active_settings.team_mode_enabled);
  }

  size_t num_slots = players.size() + ais.size() + 1;
  bool team_mode = RoundManager::get().get_active_settings().team_mode_enabled;

  auto btn_group = imm::div(
      context, mk(elem.ent()),
      ComponentConfig{}
          .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
          .with_margin(Margin{.top = screen_pct(0.15f),
                              // Account for left column (20%) + padding
                              .left = screen_pct(0.20f),
                              .right = screen_pct(0.1f)})
          .with_absolute_position()
          .with_debug_name("btn_group"));

  if (team_mode) {
    // Team mode: Two rows (Team A on top, Team B on bottom)

    // Group players by team
    std::vector<size_t> team_a_players;
    std::vector<size_t> team_b_players;

    for (size_t i = 0; i < num_slots; i++) {
      OptEntity car;
      if (i < players.size()) {
        car = players[i];
      } else if (i < players.size() + ais.size()) {
        car = ais[i - players.size()];
      }

      // Determine which team this player belongs to
      bool is_team_a = true; // Default to team A
      if (car.has_value() && car->has<TeamID>()) {
        is_team_a = (car->get<TeamID>().team_id == 0);
      } else {
        // If no team assignment, alternate based on index
        is_team_a = (i % 2 == 0);
      }

      if (is_team_a) {
        team_a_players.push_back(i);
      } else {
        team_b_players.push_back(i);
      }
    }

    // Create centered container for team columns
    auto team_columns_container =
        imm::div(context, mk(btn_group.ent()),
                 ComponentConfig{}
                     .with_size(ComponentSize{percent(1.0f), percent(1.f)})
                     .with_flex_direction(FlexDirection::Row)
                     .with_debug_name("team_columns_container"));

    render_team_column(context, team_columns_container.ent(), "Team A",
                       team_a_players, num_slots, 0);
    render_team_column(context, team_columns_container.ent(), "Team B",
                       team_b_players, num_slots, 1);
  } else {
    // Individual mode: Original grid layout
    int fours =
        static_cast<int>(std::ceil(static_cast<float>(num_slots) / 4.f));

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
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

void ScheduleMainMenuUI::render_round_settings_preview(
    UIContext<InputAction> &context, Entity &parent) {
  imm::div(
      context, mk(parent),
      ComponentConfig{}.with_label(translation_manager::translate_formatted(
          translation_manager::make_translatable_string(
              strings::i18n::win_condition_label)
              .set_param(
                  translation_manager::i18nParam::weapon_name,
                  magic_enum::enum_name(RoundManager::get().active_round_type),
                  translation_manager::translation_param))));

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
    imm::div(
        context, mk(parent),
        ComponentConfig{}.with_label(translation_manager::translate_formatted(
            translation_manager::make_translatable_string(
                strings::i18n::num_lives_label)
                .set_param(translation_manager::i18nParam::number_count,
                           s.num_starting_lives,
                           translation_manager::translation_param))));
    break;
  }
  case RoundType::Kills: {
    auto &s = RoundManager::get().get_active_rt<RoundKillsSettings>();
    std::string time_display;
    switch (s.time_option) {
    case RoundSettings::TimeOptions::Unlimited:
      time_display = translation_manager::make_translatable_string(
                         strings::i18n::unlimited)
                         .get_text();
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
    imm::div(
        context, mk(parent),
        ComponentConfig{}.with_label(translation_manager::translate_formatted(
            translation_manager::make_translatable_string(
                strings::i18n::round_length_with_time)
                .set_param(translation_manager::i18nParam::weapon_name,
                           time_display,
                           translation_manager::translation_param))));
    break;
  }
  case RoundType::Hippo: {
    auto &s = RoundManager::get().get_active_rt<RoundHippoSettings>();
    imm::div(
        context, mk(parent),
        ComponentConfig{}.with_label(translation_manager::translate_formatted(
            translation_manager::make_translatable_string(
                strings::i18n::total_hippos_label)
                .set_param(translation_manager::i18nParam::number_count,
                           s.total_hippos,
                           translation_manager::translation_param))));
    break;
  }
  case RoundType::TagAndGo: {
    auto &s = RoundManager::get().get_active_rt<RoundTagAndGoSettings>();
    std::string time_display;
    switch (s.time_option) {
    case RoundSettings::TimeOptions::Unlimited:
      time_display = translation_manager::make_translatable_string(
                         strings::i18n::unlimited)
                         .get_text();
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
    imm::div(
        context, mk(parent),
        ComponentConfig{}.with_label(translation_manager::translate_formatted(
            translation_manager::make_translatable_string(
                strings::i18n::round_length_with_time)
                .set_param(translation_manager::i18nParam::weapon_name,
                           time_display,
                           translation_manager::translation_param))));
    break;
  }
  default:
    imm::div(context, mk(parent),
             ComponentConfig{}.with_label(
                 translation_manager::make_translatable_string(
                     strings::i18n::round_settings)
                     .get_text()));
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

bool ScheduleDebugUI::should_run(float dt) {
  enableCooldown -= dt;

  if (enableCooldown < 0) {
    enableCooldown = enableCooldownReset;
    input::PossibleInputCollector inpc = input::get_input_collector();

    bool debug_pressed =
        std::ranges::any_of(inpc.inputs(), [](const auto &actions_done) {
          return action_matches(actions_done.action,
                                InputAction::ToggleUIDebug);
        });
    if (debug_pressed) {
      enabled = !enabled;
    }
  }
  return enabled;
}

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
  inpc = input::get_input_collector();
  return GameStateManager::get().is_game_active() ||
         GameStateManager::get().is_paused();
}

void SchedulePauseUI::for_each_with(Entity &entity,
                                    UIContext<InputAction> &context, float) {
  const bool pause_pressed =
      std::ranges::any_of(inpc.inputs_pressed(), [](const auto &actions_done) {
        return action_matches(actions_done.action, InputAction::PauseButton);
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

  if (!GameStateManager::get().is_paused()) {
    return;
  }

  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("pause_screen"));

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
               .with_label(translation_manager::make_translatable_string(
                               strings::i18n::paused)
                               .get_text())
               .with_skip_tabbing(true)
               .with_size(ComponentSize{pixels(400.f), pixels(100.f)}));

  if (imm::button(context, mk(left_col.ent(), 1),
                  ComponentConfig{}
                      .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                            .left = imm::DefaultSpacing::tiny(),
                                            .bottom = imm::DefaultSpacing::tiny(),
                                            .right = imm::DefaultSpacing::tiny()})
                      .with_label(translation_manager::make_translatable_string(
                                      strings::i18n::resume)
                                      .get_text()))) {
    GameStateManager::get().unpause_game();
  }

  if (imm::button(context, mk(left_col.ent(), 2),
                  ComponentConfig{}
                      .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                            .left = imm::DefaultSpacing::tiny(),
                                            .bottom = imm::DefaultSpacing::tiny(),
                                            .right = imm::DefaultSpacing::tiny()})
                      .with_label(translation_manager::make_translatable_string(
                                      strings::i18n::back_to_setup)
                                      .get_text()))) {
    GameStateManager::get().end_game();
  }

  if (imm::button(context, mk(left_col.ent(), 3),
                  ComponentConfig{}
                      .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                            .left = imm::DefaultSpacing::tiny(),
                                            .bottom = imm::DefaultSpacing::tiny(),
                                            .right = imm::DefaultSpacing::tiny()})
                      .with_label(translation_manager::make_translatable_string(
                                      strings::i18n::exit_game)
                                      .get_text()))) {
    exit_game();
  }
}

void round_lives_settings(Entity &entity, UIContext<InputAction> &context) {
  auto &rl_settings = RoundManager::get().get_active_rt<RoundLivesSettings>();

  imm::div(context, mk(entity),
           ComponentConfig{}
               .with_label(translation_manager::translate_formatted(
                   translation_manager::make_translatable_string(
                       strings::i18n::num_lives_label)
                       .set_param(translation_manager::i18nParam::number_count,
                                  rl_settings.num_starting_lives,
                                  translation_manager::translation_param)))
               .with_size(ComponentSize{screen_pct(0.15f), screen_pct(0.06f)})
               .with_margin(Margin{.top = screen_pct(0.01f)})
               .with_debug_name("num_lives_text")
               .with_opacity(0.0f)
               .with_translate(-2000.0f, 0.0f));
}

void round_kills_settings(Entity &entity, UIContext<InputAction> &context) {
  auto &rl_settings = RoundManager::get().get_active_rt<RoundKillsSettings>();

  imm::div(context, mk(entity),
           ComponentConfig{}
               .with_label(translation_manager::translate_formatted(
                   translation_manager::make_translatable_string(
                       strings::i18n::round_length_with_time)
                       .set_param(translation_manager::i18nParam::number_time,
                                  rl_settings.current_round_time,
                                  translation_manager::translation_param)))
               .with_size(ComponentSize{screen_pct(0.15f), screen_pct(0.06f)})
               .with_margin(Margin{.top = screen_pct(0.01f)})
               .with_opacity(0.0f)
               .with_translate(-2000.0f, 0.0f));

  {
    // TODO replace with actual strings
    auto options = magic_enum::enum_names<RoundSettings::TimeOptions>();
    auto option_index = magic_enum::enum_index(rl_settings.time_option).value();

    if (auto result = imm::dropdown(
            context, mk(entity), options, option_index,
            ComponentConfig{}
                .with_size(ComponentSize{pixels(400.f), pixels(40.f)})
                .with_label(translation_manager::make_translatable_string(
                                strings::i18n::round_length)
                                .get_text())
                .with_opacity(0.0f)
                .with_translate(-2000.0f, 0.0f));
        result) {
      rl_settings.set_time_option(result.as<int>());
    }
  }
}

void round_hippo_settings(Entity &entity, UIContext<InputAction> &context) {
  auto &rl_settings = RoundManager::get().get_active_rt<RoundHippoSettings>();

  imm::div(context, mk(entity),
           ComponentConfig{}
               .with_label(translation_manager::translate_formatted(
                   translation_manager::make_translatable_string(
                       strings::i18n::total_hippos_label)
                       .set_param(translation_manager::i18nParam::number_count,
                                  rl_settings.total_hippos,
                                  translation_manager::translation_param)))
               .with_size(ComponentSize{screen_pct(0.15f), screen_pct(0.06f)}));
}

void round_tag_and_go_settings(Entity &entity,
                               UIContext<InputAction> &context) {
  auto &cm_settings =
      RoundManager::get().get_active_rt<RoundTagAndGoSettings>();

  {
    auto options = magic_enum::enum_names<RoundSettings::TimeOptions>();
    auto option_index = magic_enum::enum_index(cm_settings.time_option).value();

    if (auto result = imm::dropdown(
            context, mk(entity), options, option_index,
            ComponentConfig{}
                .with_size(ComponentSize{pixels(400.f), pixels(40.f)})
                .with_label(translation_manager::translate_formatted(
                    translation_manager::make_translatable_string(
                        strings::i18n::round_length)
                        .set_param(translation_manager::i18nParam::number_time,
                                   30, translation_manager::translation_param)))
                .with_opacity(0.0f)
                .with_translate(-2000.0f, 0.0f));
        result) {
      cm_settings.set_time_option(result.as<int>());
    }
  }

  if (imm::checkbox(
          context, mk(entity), cm_settings.allow_tag_backs,
          ComponentConfig{}
              .with_size(ComponentSize{pixels(400.f), pixels(40.f)})
              .with_label(translation_manager::make_translatable_string(
                              strings::i18n::allow_tag_backs)
                              .get_text())
              .with_opacity(0.0f)
              .with_translate(-2000.0f, 0.0f))) {
    // value already toggled by checkbox binding
  }
}

Screen ScheduleMainMenuUI::round_settings(Entity &entity,
                                          UIContext<InputAction> &context) {
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_debug_name("round_settings")
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position());

  auto top_left = ui_helpers::create_top_left_container(
      context, elem.ent(), "round_settings_top_left", 0);

  // Top-left controls scheduled first so "select map" gets initial focus
  {
    ui_helpers::create_styled_button(
        context, top_left.ent(),
        translation_manager::make_translatable_string(strings::i18n::select_map)
            .get_text(),
        []() { navigation::to(GameStateManager::Screen::MapSelection); }, 0);

    {
      auto win_condition_div =
          imm::div(context, mk(top_left.ent()),
                   ComponentConfig{}
                       .with_size(ComponentSize{percent(1.f), percent(0.2f)})
                       .with_debug_name("win_condition_div"));

      static size_t selected_round_type =
          static_cast<size_t>(RoundManager::get().active_round_type);

      if (auto result = imm::navigation_bar(
              context, mk(win_condition_div.ent()), RoundType_NAMES,
              selected_round_type,
              ComponentConfig{}.with_opacity(0.0f).with_translate(-2000.0f,
                                                                  0.0f));
          result) {
        RoundManager::get().set_active_round_type(
            static_cast<int>(selected_round_type));
      }
    }

    // shared across all round types
    auto enabled_weapons = RoundManager::get().get_enabled_weapons();

    if (auto result = imm::checkbox_group(
            context, mk(top_left.ent()), enabled_weapons, WEAPON_STRING_LIST,
            {1, 3},
            ComponentConfig{}
                .with_flex_direction(FlexDirection::Column)
                .with_margin(Margin{.top = screen_pct(0.01f)})
                .with_opacity(0.0f)
                .with_translate(-2000.0f, 0.0f));
        result) {
      auto mask = result.as<unsigned long>();
      log_info("weapon checkbox_group changed; mask={}", mask);
      RoundManager::get().set_enabled_weapons(mask);
    }

    switch (RoundManager::get().active_round_type) {
    case RoundType::Lives:
      round_lives_settings(top_left.ent(), context);
      break;
    case RoundType::Kills:
      round_kills_settings(top_left.ent(), context);
      break;
    case RoundType::Hippo:
      round_hippo_settings(top_left.ent(), context);
      break;
    case RoundType::TagAndGo:
      round_tag_and_go_settings(top_left.ent(), context);
      break;
    default:
      log_error("You need to add a handler for UI settings for round type {}",
                (int)RoundManager::get().active_round_type);
      break;
    }

    ui_helpers::create_styled_button(
        context, top_left.ent(),
        translation_manager::make_translatable_string(strings::i18n::back)
            .get_text(),
        []() { navigation::back(); }, 2);
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::map_selection(Entity &entity,
                                         UIContext<InputAction> &context) {
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
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
          auto mp = input::get_mouse_position();
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
          auto mp = input::get_mouse_position();
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

  // selected_map_it and maybe_shuffle were unused, removed
  bool overriding_preview = effective_preview_index != selected_map_index;
  render_map_preview(context, preview_box.ent(), effective_preview_index,
                     selected_map_index, compatible_maps, overriding_preview,
                     prev_preview_index);

  ui_helpers::create_styled_button(
      context, left_col.ent(),
      translation_manager::make_translatable_string(strings::i18n::back)
          .get_text(),
      []() { navigation::back(); }, 0);

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
      .on_step(
          1.0f,
          [](int) {
            auto opt = EntityQuery({.force_merge = true})
                           .whereHasComponent<sound_system::SoundEmitter>()
                           .gen_first();
            if (opt.valid()) {
              auto &ent = opt.asE();
              auto &req =
                  ent.addComponentIfMissing<sound_system::PlaySoundRequest>();
              req.policy = sound_system::PlaySoundRequest::Policy::Name;
              req.name = sound_file_to_str(SoundFile::UI_Move);
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
      context, top_left.ent(),
      translation_manager::make_translatable_string(strings::i18n::play)
          .get_text(),
      []() { navigation::to(GameStateManager::Screen::CharacterCreation); }, 0);

  // About button
  ui_helpers::create_styled_button(
      context, top_left.ent(),
      translation_manager::make_translatable_string(strings::i18n::about)
          .get_text(),
      []() { navigation::to(GameStateManager::Screen::About); }, 1);

  // Settings button
  ui_helpers::create_styled_button(
      context, top_left.ent(),
      translation_manager::make_translatable_string(strings::i18n::settings)
          .get_text(),
      []() { navigation::to(GameStateManager::Screen::Settings); }, 2);

  // Exit button
  ui_helpers::create_styled_button(
      context, top_left.ent(),
      translation_manager::make_translatable_string(strings::i18n::exit)
          .get_text(),
      [this]() { exit_game(); }, 3);

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::settings_screen(Entity &entity,
                                           UIContext<InputAction> &context) {
  auto elem =
      ui_helpers::create_screen_container(context, entity, "settings_screen");
  auto top_left = ui_helpers::create_top_left_container(context, elem.ent(),
                                                        "settings_top_left", 0);
  {
    ui_helpers::create_styled_button(
        context, top_left.ent(),
        translation_manager::make_translatable_string(strings::i18n::back)
            .get_text(),
        []() {
          Settings::update_resolution(
              EntityHelper::get_singleton_cmp<
                  window_manager::ProvidesCurrentResolution>()
                  ->current_resolution);
          navigation::back();
        },
        0);
  }

  // Master volume slider
  {
    float master_volume = Settings::get_master_volume();
    ui_helpers::create_volume_slider(
        context, top_left.ent(),
        translation_manager::make_translatable_string(
            strings::i18n::master_volume)
            .get_text(),
        master_volume,
        [](float volume) { Settings::update_master_volume(volume); }, 0);
  }

  // Music volume slider
  {
    float music_volume = Settings::get_music_volume();
    ui_helpers::create_volume_slider(
        context, top_left.ent(),
        translation_manager::make_translatable_string(
            strings::i18n::music_volume)
            .get_text(),
        music_volume,
        [](float volume) { Settings::update_music_volume(volume); }, 1);
  }

  // SFX volume slider
  {
    float sfx_volume = Settings::get_sfx_volume();
    ui_helpers::create_volume_slider(
        context, top_left.ent(),
        translation_manager::make_translatable_string(strings::i18n::sfx_volume)
            .get_text(),
        sfx_volume, [](float volume) { Settings::update_sfx_volume(volume); },
        2);
  }

  // Resolution dropdown
  {
    // TODO for some reason the dropdown button isnt wiggling
    if (imm::dropdown(
            context, mk(top_left.ent(), 3), resolution_strs, resolution_index,
            ComponentConfig{}
                .with_size(ComponentSize{pixels(400.f), pixels(40.f)})
                .with_label(translation_manager::make_translatable_string(
                                strings::i18n::resolution)
                                .get_text())
                .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                      .left = imm::DefaultSpacing::tiny(),
                                      .bottom = imm::DefaultSpacing::tiny(),
                                      .right = imm::DefaultSpacing::tiny()}))) {
      resolution_provider->on_data_changed(resolution_index);
    }
  }

  // Language dropdown
  {
    static std::vector<std::string> language_names =
        translation_manager::get_available_languages();
    static size_t language_dropdown_index =
        0; // Unique variable for language dropdown
    static translation_manager::Language last_language =
        translation_manager::Language::English;

    // Only update index when language actually changes
    auto current_lang = translation_manager::get_language();
    if (current_lang != last_language) {
      language_dropdown_index =
          translation_manager::get_language_index(current_lang);
      last_language = current_lang;
    }

    if (imm::dropdown(
            context, mk(top_left.ent(), 4), language_names,
            language_dropdown_index,
            ComponentConfig{}
                .with_size(ComponentSize{pixels(400.f), pixels(40.f)})
                .with_label(translation_manager::make_translatable_string(
                                strings::i18n::language)
                                .get_text())
                .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                      .left = imm::DefaultSpacing::tiny(),
                                      .bottom = imm::DefaultSpacing::tiny(),
                                      .right = imm::DefaultSpacing::tiny()}))) {

      auto new_language = translation_manager::Language::English;
      // Update language when selection changes
      switch (language_dropdown_index) {
      case 0:
        new_language = translation_manager::Language::English;
        break;
      case 1:
        new_language = translation_manager::Language::Korean;
        break;
      case 2:
        new_language = translation_manager::Language::Japanese;
        break;
      default:
        // This will cause a compilation error if we add a new language without
        // updating this switch
        static_assert(magic_enum::enum_count<translation_manager::Language>() ==
                          3,
                      "Add new language case to this switch statement");
        break;
      }

      translation_manager::set_language(new_language);
      Settings::set_language(new_language);
      Settings::write_save_file();

      auto &styling_defaults = afterhours::ui::imm::UIStylingDefaults::get();
      auto font_name =
          get_font_name(translation_manager::get_font_for_language());
      styling_defaults.set_default_font(font_name, 16.f);
    }
  }

  // Fullscreen checkbox
  if (imm::checkbox(
          context, mk(top_left.ent(), 5), Settings::get_fullscreen_enabled(),
          ComponentConfig{}
              .with_size(ComponentSize{pixels(400.f), pixels(40.f)})
              .with_label(translation_manager::make_translatable_string(
                              strings::i18n::fullscreen)
                              .get_text())
              .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                    .left = imm::DefaultSpacing::tiny(),
                                    .bottom = imm::DefaultSpacing::tiny(),
                                    .right = imm::DefaultSpacing::tiny()}))) {
    Settings::toggle_fullscreen();
  }

  // Post Processing checkbox
  if (imm::checkbox(
          context, mk(top_left.ent(), 6),
          Settings::get_post_processing_enabled(),
          ComponentConfig{}
              .with_size(ComponentSize{pixels(400.f), pixels(40.f)})
              .with_label(translation_manager::make_translatable_string(
                              strings::i18n::post_processing)
                              .get_text())
              .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                    .left = imm::DefaultSpacing::tiny(),
                                    .bottom = imm::DefaultSpacing::tiny(),
                                    .right = imm::DefaultSpacing::tiny()}))) {
    Settings::toggle_post_processing();
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
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("about_screen"));

  {
    auto top_left = ui_helpers::create_top_left_container(context, elem.ent(),
                                                          "about_top_left", 0);
    ui_helpers::create_styled_button(
        context, top_left.ent(),
        translation_manager::make_translatable_string(strings::i18n::back)
            .get_text(),
        []() { navigation::back(); }, 0);
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

void ScheduleMainMenuUI::render_team_column_results(
    UIContext<InputAction> &context, Entity &parent,
    const std::string &team_name, int team_id,
    const std::vector<OptEntity> &team_players, int team_score) {
  // Determine team color
  auto team_color =
      team_id == 0
          ? afterhours::ui::imm::ThemeDefaults::get().get_theme().from_usage(
                Theme::Usage::Primary) // Light blue for Team A
          : afterhours::ui::imm::ThemeDefaults::get().get_theme().from_usage(
                Theme::Usage::Accent); // Light orange for Team B

  // Create team column
  auto team_column =
      imm::div(context, mk(parent, team_id),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(0.5f), percent(1.f)})
                   .with_flex_direction(FlexDirection::Column)
                   .with_custom_background(team_color)
                   .disable_rounded_corners()
                   .with_debug_name(team_name + "_column"));

  // Team header
  imm::div(context, mk(team_column.ent()),
           ComponentConfig{}
               .with_label(team_name)
               .with_size(ComponentSize{percent(1.f), percent(0.15f)})
               .with_debug_name(team_name + "_header"));

  // Team score
  imm::div(context, mk(team_column.ent()),
           ComponentConfig{}
               .with_label("Score: " + std::to_string(team_score))
               .with_size(ComponentSize{percent(1.f), percent(0.1f)})
               .with_debug_name(team_name + "_score"));

  // Team players
  for (size_t i = 0; i < team_players.size(); i++) {
    const auto &player = team_players[i];
    if (player.has_value()) {
      std::string player_name = "Player" + std::to_string(i + 1);
      if (player->has<PlayerID>()) {
        player_name = "Player" + std::to_string(player->get<PlayerID>().id);
      } else if (player->has<AIControlled>()) {
        player_name = "AI" + std::to_string(i + 1);
      }

      imm::div(
          context, mk(team_column.ent(), i),
          ComponentConfig{}
              .with_label(player_name)
              .with_size(ComponentSize{percent(1.f), percent(0.1f)})
              .with_debug_name(team_name + "_player_" + std::to_string(i)));
    }
  }
}

void ScheduleMainMenuUI::render_team_results(
    UIContext<InputAction> &context, Entity &parent,
    const std::vector<OptEntity> &round_players,
    const std::vector<OptEntity> &round_ais) {
  // Group players by team
  std::map<int, std::vector<OptEntity>> team_groups;

  // Group players by team
  for (const auto &player : round_players) {
    if (player.has_value()) {
      int team_id = -1; // Default to no team
      if (player->has<TeamID>()) {
        team_id = player->get<TeamID>().team_id;
      }
      team_groups[team_id].push_back(player);
    }
  }

  // Group AIs by team
  for (const auto &ai : round_ais) {
    if (ai.has_value()) {
      int team_id = -1; // Default to no team
      if (ai->has<TeamID>()) {
        team_id = ai->get<TeamID>().team_id;
      }
      team_groups[team_id].push_back(ai);
    }
  }

  // Calculate team scores
  std::map<int, int> team_scores;
  for (const auto &[team_id, players] : team_groups) {
    int total_score = 0;
    for (const auto &player : players) {
      if (player.has_value()) {
        // Calculate score based on game mode
        if (RoundManager::get().active_round_type == RoundType::Hippo) {
          if (player->has<HasHippoCollection>()) {
            total_score += player->get<HasHippoCollection>().get_hippo_count();
          }
        } else if (RoundManager::get().active_round_type == RoundType::Kills) {
          if (player->has<HasKillCountTracker>()) {
            total_score += player->get<HasKillCountTracker>().kills;
          }
        } else if (RoundManager::get().active_round_type == RoundType::Lives) {
          if (player->has<HasMultipleLives>()) {
            total_score += player->get<HasMultipleLives>().num_lives_remaining;
          }
        } else if (RoundManager::get().active_round_type ==
                   RoundType::TagAndGo) {
          if (player->has<HasTagAndGoTracking>()) {
            total_score += static_cast<int>(
                player->get<HasTagAndGoTracking>().time_as_not_it);
          }
        }
      }
    }
    team_scores[team_id] = total_score;
  }

  // Create two-column layout
  auto team_container =
      imm::div(context, mk(parent),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(0.6f), screen_pct(0.6f)})
                   .with_margin(Margin{.top = screen_pct(0.2f),
                                       .left = screen_pct(0.2f)})
                   .with_flex_direction(FlexDirection::Row)
                   .with_absolute_position()
                   .with_debug_name("team_results_container"));

  render_team_column_results(context, team_container.ent(), "TEAM A", 0,
                             team_groups[0], team_scores[0]);
  render_team_column_results(context, team_container.ent(), "TEAM B", 1,
                             team_groups[1], team_scores[1]);
}

Screen ScheduleMainMenuUI::round_end_screen(Entity &entity,
                                            UIContext<InputAction> &context) {
  auto elem =
      ui_helpers::create_screen_container(context, entity, "round_end_screen");
  auto top_left = ui_helpers::create_top_left_container(
      context, elem.ent(), "round_end_top_left", 0);
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
                 .with_label(translation_manager::make_translatable_string(
                                 strings::i18n::round_end)
                                 .get_text())
                 .with_skip_tabbing(true)
                 .with_size(ComponentSize{percent(0.5f), percent(0.2f)})
                 .with_margin(Margin{
                     .top = screen_pct(0.05f),
                     .left = screen_pct(0.2f),
                 })
                 .with_debug_name("round_end_title"));
  }

  // Check if team mode is enabled
  auto &settings = RoundManager::get().get_active_settings();
  if (settings.team_mode_enabled) {
    // Render team results in two columns
    render_team_results(context, elem.ent(), round_players, round_ais);
  } else {
    // Render individual results in grid layout
    std::map<EntityID, int> rankings;
    if (RoundManager::get().active_round_type == RoundType::TagAndGo) {
      rankings = get_tag_and_go_rankings(round_players, round_ais);
    }

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

          round_end_player_column(row.ent(), context, i, round_players,
                                  round_ais, ranking);
        }
      }
    }
  }

  ui_helpers::create_styled_button(
      context, top_left.ent(),
      translation_manager::make_translatable_string(
          strings::i18n::continue_game)
          .get_text(),
      []() { navigation::to(GameStateManager::Screen::CharacterCreation); }, 0);
  ui_helpers::create_styled_button(
      context, top_left.ent(),
      translation_manager::make_translatable_string(strings::i18n::quit)
          .get_text(),
      [this]() { exit_game(); }, 1);

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

void register_ui_systems(afterhours::SystemManager &systems) {
  ui::register_before_ui_updates<InputAction>(systems);
  {
    systems.register_update_system(
        std::make_unique<SetupGameStylingDefaults>());

    afterhours::animation::register_update_systems<UIKey>(systems);
    afterhours::animation::register_update_systems<
        afterhours::animation::CompositeKey>(systems);
    systems.register_update_system(
        std::make_unique<ui_game::UpdateUIWiggle<InputAction>>());
    systems.register_update_system(std::make_unique<NavigationSystem>());
    systems.register_update_system(std::make_unique<ScheduleMainMenuUI>());
    systems.register_update_system(std::make_unique<SchedulePauseUI>());
    systems.register_update_system(std::make_unique<ScheduleDebugUI>());
  }
  ui::register_after_ui_updates<InputAction>(systems);
  systems.register_update_system(
      std::make_unique<ui_game::ApplyInitialSlideInMask<InputAction>>());
  systems.register_update_system(
      std::make_unique<ui_game::UpdateUISlideIn<InputAction>>());
}