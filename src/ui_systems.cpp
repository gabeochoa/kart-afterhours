
#include "ui_systems.h"

#include "game_state_manager.h"
#include "input_mapping.h"
#include "makers.h" // make_ai()
#include "map_system.h"
#include "preload.h" // FontID
#include "round_settings.h"

// TODO the top left buttons should be have some top/left padding

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

void ScheduleMainMenuUI::navigate_back() {
  // If we're on the main screen, exit the game
  if (GameStateManager::get().active_screen == GameStateManager::Screen::Main ||
      navigation_stack.empty()) {
    exit_game();
    return;
  }

  Screen previous_screen = navigation_stack.back();
  navigation_stack.pop_back();
  GameStateManager::get().set_next_screen(previous_screen);
}

void ScheduleMainMenuUI::navigate_to_screen(Screen screen) {
  if (GameStateManager::get().active_screen != screen) {
    navigation_stack.push_back(GameStateManager::get().active_screen);
  }

  GameStateManager::get().set_next_screen(screen);
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

  if (navigation_stack.empty()) {
    if (GameStateManager::get().active_screen !=
        GameStateManager::Screen::Main) {
      navigation_stack.push_back(GameStateManager::Screen::Main);
    }
  }

  // character creator
  {
    players = EQ().whereHasComponent<PlayerID>().orderByPlayerID().gen();
    ais = EQ().whereHasComponent<AIControlled>().gen();
    inpc = input::get_input_collector<InputAction>();
  }
}

bool ScheduleMainMenuUI::should_run(float) {
  inpc = input::get_input_collector<InputAction>();
  if (GameStateManager::get().is_game_active()) {
    ui_visible = false;
  } else if (GameStateManager::get().is_menu_active()) {
    ui_visible = true;
  }

  const bool start_pressed =
      std::ranges::any_of(inpc.inputs_pressed(), [](const auto &actions_done) {
        return actions_done.action == InputAction::WidgetMod;
      });

  if (!ui_visible && start_pressed) {
    navigate_to_screen(GameStateManager::Screen::Main);
    ui_visible = true;
  } else if (ui_visible && start_pressed) {
    ui_visible = false;
  }

  // Handle escape key for back navigation
  const bool escape_pressed =
      std::ranges::any_of(inpc.inputs_pressed(), [](const auto &actions_done) {
        return actions_done.action == InputAction::MenuBack;
      });

  if (escape_pressed && ui_visible) {
    navigate_back();
  }

  return ui_visible;
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
                   .with_margin(Margin{.top = percent(0.05f),
                                       .bottom = percent(0.05f),
                                       .left = percent(0.05f),
                                       .right = percent(0.05f)})
                   .with_color_usage(Theme::Usage::Custom)
                   .with_custom_color(bg_color)
                   .disable_rounded_corners());

  if (car.has_value()) {
    // Note: we arent using mk() here...
    imm::div(
        context, mk(column.ent()),
        ComponentConfig{}
            .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
            .with_label(std::format("{} {}", index, car->id))
            .with_color_usage(Theme::Usage::Custom)
            .with_custom_color(bg_color)
            .disable_rounded_corners()
            .with_debug_name(std::format("player_car {} {} {} {}", index,
                                         car->id, players.size(), ais.size())));
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

      player_right |= actions_done.action == InputAction::WidgetRight;

      if (player_right) {
        break;
      }
    }
  }

  bool show_next_color_button =
      (is_last_slot && !is_last_slot_ai) ||
      (!is_last_slot && colorManager.any_available_colors());

  if (show_next_color_button) {
    if (auto elem = imm::button(
            context, mk(column.ent()),
            ComponentConfig{}
                .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
                .with_label("Next Color")
                .disable_rounded_corners()
                .with_skip_tabbing(true)
                .with_debug_name(std::format("next_color button {} ", index)));
        elem || player_right) {
      colorManager.release_and_get_next(car->id);
    }
  }

  // we are the last boi
  if (num_slots <= input::MAX_GAMEPAD_ID && is_last_slot) {
    // TODO make it more obvious this button is to add
    // - maybe maybe the color more see through?
    // - add just a dotted line or just outline?
    if (imm::button(context, mk(column.ent()),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.f), pixels(50.f)})
                        .with_padding(Padding{.top = percent(0.25f)})
                        .with_label("Add AI")
                        .disable_rounded_corners()
                        .with_debug_name("add_ai_button"))) {
      make_ai();
    }
  }

  if (is_slot_ai && car.has_value()) {
    if (imm::button(context, mk(column.ent()),
                    ComponentConfig{}
                        .with_size(ComponentSize{percent(1.f), pixels(50.f)})
                        .with_padding(Padding{.top = percent(0.25f)})
                        .with_label("Remove AI")
                        .disable_rounded_corners()
                        .with_debug_name("remove_ai_button"))) {

      colorManager.release_only(car->id);
      car->cleanup = true;
    }
  }
}

void ScheduleMainMenuUI::round_end_player_column(
    Entity &parent, UIContext<InputAction> &context, const size_t index,
    const std::vector<OptEntity> &round_players,
    const std::vector<OptEntity> &round_ais) {

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
                   .disable_rounded_corners());

  // Player info
  {
    std::string player_label = std::format("{} {}", index, car->id);
    if (is_slot_ai) {
      player_label += " (AI)";
    }

    imm::div(context, mk(column.ent(), 0),
             ComponentConfig{}
                 .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
                 .with_label(player_label)
                 .with_color_usage(Theme::Usage::Custom)
                 .with_custom_color(bg_color)
                 .disable_rounded_corners()
                 .with_debug_name(
                     std::format("round_end_player {} {} {} {}", index, car->id,
                                 round_players.size(), round_ais.size())));
  }

  // Round-specific stats
  render_round_end_stats(context, column.ent(), car, bg_color);
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
  case RoundType::Score:
    render_score_stats(context, parent, car, bg_color);
    break;
  case RoundType::CatAndMouse:
    render_cat_mouse_stats(context, parent, car, bg_color);
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

  std::string stats_text = std::format(
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
      std::format("Kills: {}", car->get<HasKillCountTracker>().kills);

  imm::div(context, mk(parent, 1),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
               .with_label(stats_text)
               .with_color_usage(Theme::Usage::Custom)
               .with_custom_color(bg_color)
               .disable_rounded_corners());
}

void ScheduleMainMenuUI::render_score_stats(UIContext<InputAction> &context,
                                            Entity &parent,
                                            const OptEntity &car,
                                            raylib::Color bg_color) {
  // TODO: Add score tracking component
  std::string stats_text = "Score: N/A";

  imm::div(context, mk(parent, 1),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
               .with_label(stats_text)
               .with_color_usage(Theme::Usage::Custom)
               .with_custom_color(bg_color)
               .disable_rounded_corners());
}

void ScheduleMainMenuUI::render_cat_mouse_stats(UIContext<InputAction> &context,
                                                Entity &parent,
                                                const OptEntity &car,
                                                raylib::Color bg_color) {
  if (!car->has<HasCatMouseTracking>()) {
    return;
  }

  const auto &tracking = car->get<HasCatMouseTracking>();
  std::string stats_text;

  if (tracking.is_cat) {
    stats_text = "Cat";
  } else {
    stats_text = std::format("Mouse: {:.1f}s", tracking.time_as_mouse);
  }

  imm::div(context, mk(parent, 1),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
               .with_label(stats_text)
               .with_color_usage(Theme::Usage::Custom)
               .with_custom_color(bg_color)
               .disable_rounded_corners());
}

void ScheduleMainMenuUI::render_unknown_stats(UIContext<InputAction> &context,
                                              Entity &parent,
                                              const OptEntity &car,
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

Screen ScheduleMainMenuUI::character_creation(Entity &entity,
                                              UIContext<InputAction> &context) {
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("character_creation"));

  {
    if (imm::button(context, mk(elem.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("select map"))) {
      GameStateManager::get().set_next_screen(
          GameStateManager::Screen::MapSelection);
    }
  }

  {
    if (imm::button(context, mk(elem.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("back"))) {
      GameStateManager::get().set_next_screen(GameStateManager::Screen::Main);
    }
  }

  {
    if (imm::button(context, mk(elem.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("round settings"))) {
      navigate_to_screen(GameStateManager::Screen::RoundSettings);
    }
  }

  // Settings Preview
  {
    auto round_lives_preview = [](Entity &entity,
                                  UIContext<InputAction> &context) {
      auto &rl_settings =
          RoundManager::get().get_active_rt<RoundLivesSettings>();

      imm::div(context, mk(entity),
               ComponentConfig{}.with_label(std::format(
                   "Num Lives: {}", rl_settings.num_starting_lives)));
    };

    auto round_kills_preview = [](Entity &entity,
                                  UIContext<InputAction> &context) {
      auto &rl_settings =
          RoundManager::get().get_active_rt<RoundKillsSettings>();

      imm::div(context, mk(entity),
               ComponentConfig{}.with_label(std::format(
                   "Round Length: {}", rl_settings.current_round_time)));
    };

    auto round_score_preview = [](Entity &entity,
                                  UIContext<InputAction> &context) {
      auto &rl_settings =
          RoundManager::get().get_active_rt<RoundScoreSettings>();

      imm::div(context, mk(entity),
               ComponentConfig{}.with_label(std::format(
                   "Score Needed: {}", rl_settings.score_needed_to_win)));
    };

    auto round_cat_mouse_preview = [](Entity &entity,
                                      UIContext<InputAction> &context) {
      auto &rl_settings =
          RoundManager::get().get_active_rt<RoundCatAndMouseSettings>();

      std::string time_display;
      switch (rl_settings.time_option) {
      case RoundCatAndMouseSettings::TimeOptions::Unlimited:
        time_display = "Unlimited";
        break;
      case RoundCatAndMouseSettings::TimeOptions::Seconds_30:
        time_display = "30s";
        break;
      case RoundCatAndMouseSettings::TimeOptions::Minutes_1:
        time_display = "1m";
        break;
      }

      imm::div(context, mk(entity),
               ComponentConfig{}.with_label(
                   std::format("Round Length: {}", time_display)));
    };

    {
      imm::div(
          context, mk(elem.ent()),
          ComponentConfig{}.with_label(std::format(
              "Win Condition: {}",
              magic_enum::enum_name(RoundManager::get().active_round_type))));
    }

    /*
     * TODO how to visualize this smaller? icons?
    // shared across all round types
    auto enabled_weapons = RoundManager::get().get_enabled_weapons();

    if (auto result =
            imm::checkbox_group(context, mk(settings_group.ent()),
                                enabled_weapons, WEAPON_STRING_LIST, {1, 3});
        result) {
      RoundManager::get().set_enabled_weapons(result.as<unsigned long>());
    }
    */

    switch (RoundManager::get().active_round_type) {
    case RoundType::Lives:
      round_lives_preview(elem.ent(), context);
      break;
    case RoundType::Kills:
      round_kills_preview(elem.ent(), context);
      break;
    case RoundType::Score:
      round_score_preview(elem.ent(), context);
      break;
    case RoundType::CatAndMouse:
      round_cat_mouse_preview(elem.ent(), context);
      break;
    default:
      log_error("You need to add a handler for UI settings for round type {}",
                (int)RoundManager::get().active_round_type);
      break;
    }
  }

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
           ComponentConfig{}.with_label(
               std::format("Num Lives: {}", rl_settings.num_starting_lives)));
}

void round_kills_settings(Entity &entity, UIContext<InputAction> &context) {
  auto &rl_settings = RoundManager::get().get_active_rt<RoundKillsSettings>();

  imm::div(context, mk(entity),
           ComponentConfig{}.with_label(std::format(
               "Round Length: {}", rl_settings.current_round_time)));

  {
    // TODO replace with actual strings
    auto options = magic_enum::enum_names<RoundKillsSettings::TimeOptions>();
    auto option_index = magic_enum::enum_index(rl_settings.time_option).value();

    if (auto result =
            imm::dropdown(context, mk(entity), options, option_index,
                          ComponentConfig{}.with_label("Round Length"));
        result) {
      rl_settings.set_time_option(result.as<int>());
    }
  }
}

void round_score_settings(Entity &entity, UIContext<InputAction> &context) {
  auto &rl_settings = RoundManager::get().get_active_rt<RoundScoreSettings>();

  imm::div(context, mk(entity),
           ComponentConfig{}.with_label(std::format(
               "Score Needed: {}", rl_settings.score_needed_to_win)));
}

void round_cat_mouse_settings(Entity &entity, UIContext<InputAction> &context) {
  auto &cm_settings =
      RoundManager::get().get_active_rt<RoundCatAndMouseSettings>();

  {
    auto options =
        magic_enum::enum_names<RoundCatAndMouseSettings::TimeOptions>();
    auto option_index = magic_enum::enum_index(cm_settings.time_option).value();

    if (auto result =
            imm::dropdown(context, mk(entity), options, option_index,
                          ComponentConfig{}.with_label("Round Length"));
        result) {
      cm_settings.set_time_option(result.as<int>());
    }
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

  auto settings_group =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_debug_name("settings_group")
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_padding(button_group_padding)
                   .with_absolute_position());

  {
    auto win_condition_div =
        imm::div(context, mk(settings_group.ent()),
                 ComponentConfig{}
                     .with_debug_name("win_condition_div")
                     .with_padding(Padding{.right = screen_pct(0.2f)}));

    if (auto result = imm::button_group(
            context, mk(win_condition_div.ent()), RoundType_NAMES,
            ComponentConfig{}
                .with_label("Win Condition")
                .with_flex_direction(FlexDirection::Row)
                .with_size(ComponentSize{pixels(100.f * RoundType_NAMES.size()),
                                         children(50.f)})
                .with_select_on_focus(true));
        result) {
      RoundManager::get().set_active_round_type(result.as<int>());
    }
  }

  // shared across all round types
  auto enabled_weapons = RoundManager::get().get_enabled_weapons();

  if (auto result =
          imm::checkbox_group(context, mk(settings_group.ent()),
                              enabled_weapons, WEAPON_STRING_LIST, {1, 3});
      result) {
    RoundManager::get().set_enabled_weapons(result.as<unsigned long>());
  }

  switch (RoundManager::get().active_round_type) {
  case RoundType::Lives:
    round_lives_settings(settings_group.ent(), context);
    break;
  case RoundType::Kills:
    round_kills_settings(settings_group.ent(), context);
    break;
  case RoundType::Score:
    round_score_settings(settings_group.ent(), context);
    break;
  case RoundType::CatAndMouse:
    round_cat_mouse_settings(settings_group.ent(), context);
    break;
  default:
    log_error("You need to add a handler for UI settings for round type {}",
              (int)RoundManager::get().active_round_type);
    break;
  }

  {
    if (imm::button(context, mk(settings_group.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("back"))) {
      navigate_back();
    }
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
                   .with_absolute_position()
                   .with_debug_name("map_selection"));

  {

    auto button_group =
        imm::div(context, mk(entity),
                 ComponentConfig{}
                     .with_font(get_font_name(FontID::EQPro), 75.f)
                     .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                     .with_absolute_position()
                     .with_debug_name("map_selection"));
    if (imm::button(
            context, mk(button_group.ent()),
            ComponentConfig{}.with_padding(button_padding).with_label("go"))) {
      MapManager::get().create_map();
      GameStateManager::get().start_game();
    }

    if (imm::button(context, mk(button_group.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("back"))) {
      GameStateManager::get().set_next_screen(
          GameStateManager::Screen::CharacterCreation);
    }
  }

  auto preview_box =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(1.0f), percent(0.4f)})
                   .with_margin(Margin{
                       .top = percent(0.1f),
                       .left = percent(0.1f),
                       .bottom = percent(0.1f),
                       .right = percent(0.1f),
                   })
                   .with_debug_name("preview_box")
                   .with_skip_tabbing(true));

  // Get current round type and compatible maps
  auto current_round_type = RoundManager::get().active_round_type;
  auto compatible_maps =
      MapManager::get().get_maps_for_round_type(current_round_type);
  auto selected_map_index = MapManager::get().get_selected_map();
  {

    // Find the currently selected map in the compatible maps list
    auto selected_map_it =
        std::find_if(compatible_maps.begin(), compatible_maps.end(),
                     [selected_map_index](const auto &pair) {
                       return pair.first == selected_map_index;
                     });

    // Display selected map info in preview box
    if (selected_map_it != compatible_maps.end()) {
      const auto &selected_map = selected_map_it->second;
      imm::div(context, mk(preview_box.ent()),
               ComponentConfig{}
                   .with_label(selected_map.display_name)
                   .with_size(ComponentSize{percent(1.f), percent(0.3f)})
                   .with_debug_name("map_title"));

      imm::div(context, mk(preview_box.ent()),
               ComponentConfig{}
                   .with_label(selected_map.description)
                   .with_size(ComponentSize{percent(1.f), percent(0.7f)})
                   .with_margin(Margin{.top = percent(0.3f)})
                   .with_debug_name("map_description"));
    }
  }

  // Grid of map options (bottom)
  auto map_grid =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(1.f), percent(0.5f)})
                   .with_margin(Margin{
                       .top = percent(0.1f),
                       .left = percent(0.1f),
                       .bottom = percent(0.1f),
                       .right = percent(0.1f),
                   })
                   .with_flex_direction(FlexDirection::Row)
                   .with_debug_name("map_grid"));

  for (size_t i = 0; i < compatible_maps.size(); i++) {
    const auto &map_pair = compatible_maps[i];
    const auto &map_config = map_pair.second;
    int map_index = map_pair.first;

    auto map_card =
        imm::div(context, mk(map_grid.ent(), static_cast<EntityID>(i)),
                 ComponentConfig{}
                     .with_debug_name("map_card")
                     .with_size(ComponentSize{
                         percent(1.f / compatible_maps.size()), percent(1.f)}));

    if (imm::button(context, mk(map_card.ent(), map_card.ent().id),
                    ComponentConfig{}
                        .with_label(map_config.display_name)
                        .with_size(ComponentSize{percent(1.f), percent(1.f)})
                        .with_margin(Margin{
                            .top = percent(0.1f),
                            .left = percent(0.1f),
                            .bottom = percent(0.1f),
                            .right = percent(0.1f),
                        }))) {
      MapManager::get().set_selected_map(map_index);
    }
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::main_screen(Entity &entity,
                                       UIContext<InputAction> &context) {
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("main_screen"));

  auto btn_group =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_padding(button_group_padding)
                   .with_absolute_position()
                   .with_debug_name("btn_group"));

  {
    if (imm::button(context, mk(btn_group.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("play"))) {
      navigate_to_screen(GameStateManager::Screen::CharacterCreation);
    }
  }

  {
    if (imm::button(context, mk(btn_group.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("about"))) {
      navigate_to_screen(GameStateManager::Screen::About);
    }
  }

  {
    if (imm::button(context, mk(btn_group.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("settings"))) {
      navigate_to_screen(GameStateManager::Screen::Settings);
    }
  }

  {
    if (imm::button(context, mk(btn_group.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("exit"))) {
      exit_game();
    }
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::settings_screen(Entity &entity,
                                           UIContext<InputAction> &context) {
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("settings_screen"));

  auto control_group =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_padding(button_group_padding)
                   .with_absolute_position()
                   .with_debug_name("control_group"));

  {
    float master_volume = Settings::get().get_master_volume();
    auto label = fmt::format("Master Volume\n {:2.0f}", master_volume * 100.f);

    if (auto result =
            slider(context, mk(control_group.ent()), master_volume,
                   ComponentConfig{}
                       .with_size(ComponentSize{pixels(300.f), pixels(50.f)})
                       .with_label(std::move(label)));
        result) {
      master_volume = result.as<float>();
      Settings::get().update_master_volume(master_volume);
    }
  }

  {
    float music_volume = Settings::get().get_music_volume();
    auto label = fmt::format("Music Volume\n {:2.0f}", music_volume * 100.f);

    if (auto result =
            slider(context, mk(control_group.ent()), music_volume,
                   ComponentConfig{}
                       .with_size(ComponentSize{pixels(300.f), pixels(50.f)})
                       .with_label(std::move(label)));
        result) {
      music_volume = result.as<float>();
      Settings::get().update_music_volume(music_volume);
    }
  }

  {
    float sfx_volume = Settings::get().get_sfx_volume();
    auto label = fmt::format("SFX Volume\n {:2.0f}", sfx_volume * 100.f);

    if (auto result =
            slider(context, mk(control_group.ent()), sfx_volume,
                   ComponentConfig{}
                       .with_size(ComponentSize{pixels(300.f), pixels(50.f)})
                       .with_label(std::move(label)));
        result) {
      sfx_volume = result.as<float>();
      Settings::get().update_sfx_volume(sfx_volume);
    }
  }

  {
    if (imm::dropdown(context, mk(control_group.ent()), resolution_strs,
                      resolution_index,
                      ComponentConfig{}.with_label("Resolution"))) {
      resolution_provider->on_data_changed(resolution_index);
    }
  }

  if (imm::checkbox(context, mk(control_group.ent()),
                    Settings::get().get_fullscreen_enabled(),
                    ComponentConfig{}.with_label("Fullscreen"))) {
    Settings::get().toggle_fullscreen();
  }

  if (imm::button(
          context, mk(control_group.ent()),
          ComponentConfig{}.with_padding(button_padding).with_label("back"))) {
    Settings::get().update_resolution(
        current_resolution_provider->current_resolution);
    // TODO do we want to write the settings, or should we have a save button?

    navigate_back();
  }
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
  window_manager::Resolution rez =
      current_resolution_provider->current_resolution;
  const auto width = static_cast<float>(rez.width);
  const auto height = static_cast<float>(rez.height);
  const auto scale = 5.f;
  auto x_pos = width * 0.2f;
  const auto num_icon = 3;
  const auto x_spacing = (width - x_pos * 2.f) / static_cast<float>(num_icon);

  for (int i = 0; i < num_icon; i++) {
    Rectangle frame = afterhours::texture_manager::idx_to_sprite_frame(i, 4);
    raylib::DrawTexturePro(sheet, frame,
                           Rectangle{
                               x_pos,
                               height * 0.2f,
                               frame.width * scale,
                               frame.height * scale,
                           },
                           vec2{frame.width / 2.f, frame.height / 2.f}, 0,
                           raylib::RAYWHITE);
    x_pos += x_spacing;
  }

  {
    if (imm::button(context, mk(control_group.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("back"))) {
      navigate_back();
    }
  }
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

  // Row 1
  {
    auto elem =
        imm::div(context, mk(entity),
                 ComponentConfig{}.with_flex_direction(FlexDirection::Row));

    // Max speed
    {
      auto max_speed_label =
          fmt::format("Max Speed\n {:.2f} m/s", Config::get().max_speed.data);
      auto pct = Config::get().max_speed.get_pct();

      if (auto result = slider(context, mk(elem.ent()), pct,
                               ComponentConfig{}
                                   .with_label(std::move(max_speed_label))
                                   .with_skip_tabbing(true));
          result) {
        Config::get().max_speed.set_pct(result.as<float>());
      }
    }

    // Breaking Acceleration
    {
      auto max_breaking_acceleration_label =
          fmt::format("Breaking \nPower \n -{:.2f} m/s^2",
                      Config::get().breaking_acceleration.data);
      auto pct = Config::get().breaking_acceleration.get_pct();

      if (auto result =
              slider(context, mk(elem.ent()), pct,
                     ComponentConfig{}
                         .with_label(std::move(max_breaking_acceleration_label))
                         .with_skip_tabbing(true));
          result) {
        Config::get().breaking_acceleration.set_pct(result.as<float>());
      }
    }

    // Forward Acceleration
    {
      auto forward_acceleration_label =
          fmt::format("Forward \nAcceleration \n {:.2f} m/s^2",
                      Config::get().forward_acceleration.data);
      auto pct = Config::get().forward_acceleration.get_pct();

      if (auto result =
              slider(context, mk(elem.ent()), pct,
                     ComponentConfig{}
                         .with_label(std::move(forward_acceleration_label))
                         .with_skip_tabbing(true));
          result) {
        Config::get().forward_acceleration.set_pct(result.as<float>());
      }
    }

    // Reverse Acceleration
    {
      auto reverse_acceleration_label =
          fmt::format("Reverse \nAcceleration \n {:.2f} m/s^2",
                      Config::get().reverse_acceleration.data);
      auto pct = Config::get().reverse_acceleration.get_pct();

      if (auto result =
              slider(context, mk(elem.ent()), pct,
                     ComponentConfig{}
                         .with_label(std::move(reverse_acceleration_label))
                         .with_skip_tabbing(true));
          result) {
        Config::get().reverse_acceleration.set_pct(result.as<float>());
      }
    }

    // Boost Acceleration
    {
      auto boost_acceleration_label =
          fmt::format("Boost \nAcceleration \n {:.2f} m/s^2",
                      Config::get().boost_acceleration.data);
      auto pct = Config::get().boost_acceleration.get_pct();

      if (auto result =
              slider(context, mk(elem.ent()), pct,
                     ComponentConfig{}
                         .with_label(std::move(boost_acceleration_label))
                         .with_skip_tabbing(true));
          result) {
        Config::get().boost_acceleration.set_pct(result.as<float>());
      }
    }

    // Boost Decay percentage
    {
      auto boost_decay_label =
          fmt::format("Boost \nDecay \n {:.2f} decay%/frame",
                      Config::get().boost_decay_percent.data);
      auto pct = Config::get().boost_decay_percent.get_pct();

      if (auto result = slider(context, mk(elem.ent()), pct,
                               ComponentConfig{}
                                   .with_label(std::move(boost_decay_label))
                                   .with_skip_tabbing(true));
          result) {
        Config::get().boost_decay_percent.set_pct(result.as<float>());
      }
    }
  }

  // Row 2
  {
    auto elem =
        imm::div(context, mk(entity),
                 ComponentConfig{}.with_flex_direction(FlexDirection::Row));

    // Skid Threshold
    {
      auto skid_threshold_label = fmt::format(
          "Skid \nThreshold \n {:.2f} %", Config::get().skid_threshold.data);
      auto pct = Config::get().skid_threshold.get_pct();

      if (auto result = slider(context, mk(elem.ent()), pct,
                               ComponentConfig{}
                                   .with_label(std::move(skid_threshold_label))
                                   .with_skip_tabbing(true));
          result) {
        Config::get().skid_threshold.set_pct(result.as<float>());
      }
    }

    // Steering Sensitivity
    {
      auto steering_sensitivity_label =
          fmt::format("Steering \nSensitivity \n {:.2f} %",
                      Config::get().steering_sensitivity.data);
      auto pct = Config::get().steering_sensitivity.get_pct();

      if (auto result =
              slider(context, mk(elem.ent()), pct,
                     ComponentConfig{}
                         .with_label(std::move(steering_sensitivity_label))
                         .with_skip_tabbing(true));
          result) {
        Config::get().steering_sensitivity.set_pct(result.as<float>());
      }
    }

    // Minimum Steering Radius
    {
      auto minimum_steering_radius_label =
          fmt::format("Min Steering \nSensitivity \n {:.2f} m",
                      Config::get().minimum_steering_radius.data);
      auto pct = Config::get().minimum_steering_radius.get_pct();

      if (auto result =
              slider(context, mk(elem.ent()), pct,
                     ComponentConfig{}
                         .with_label(std::move(minimum_steering_radius_label))
                         .with_skip_tabbing(true));
          result) {
        Config::get().minimum_steering_radius.set_pct(result.as<float>());
      }
    }

    // Maximum Steering Radius
    {
      auto maximum_steering_radius_label =
          fmt::format("Max Steering \nSensitivity \n {:.2f} m",
                      Config::get().maximum_steering_radius.data);
      auto pct = Config::get().maximum_steering_radius.get_pct();

      if (auto result =
              slider(context, mk(elem.ent()), pct,
                     ComponentConfig{}
                         .with_label(std::move(maximum_steering_radius_label))
                         .with_skip_tabbing(true));
          result) {
        Config::get().maximum_steering_radius.set_pct(result.as<float>());
      }
    }

    // Collision Scalar
    {
      auto collision_scalar_label = fmt::format(
          "Collision \nScalar \n {:.4f}", Config::get().collision_scalar.data);
      auto pct = Config::get().collision_scalar.get_pct();

      if (auto result =
              slider(context, mk(elem.ent()), pct,
                     ComponentConfig{}
                         .with_label(std::move(collision_scalar_label))
                         .with_skip_tabbing(true));
          result) {
        Config::get().collision_scalar.set_pct(result.as<float>());
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

  // Create pause menu UI
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("pause_screen"));

  auto control_group =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_padding(button_group_padding)
                   .with_absolute_position()
                   .with_debug_name("pause_control_group"));

  // Pause title
  {
    imm::div(context, mk(control_group.ent()),
             ComponentConfig{}
                 .with_label("paused")
                 .with_font(get_font_name(FontID::EQPro), 100.f)
                 .with_skip_tabbing(true)
                 .with_size(ComponentSize{pixels(400.f), pixels(100.f)}));
  }

  // Resume button
  {
    if (imm::button(context, mk(control_group.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("resume"))) {
      GameStateManager::get().unpause_game();
    }
  }

  {
    if (imm::button(context, mk(control_group.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("back to setup"))) {
      GameStateManager::get().end_game();
    }
  }

  {
    if (imm::button(context, mk(control_group.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("exit game"))) {
      exit_game();
    }
  }
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
        round_end_player_column(row.ent(), context, i, round_players,
                                round_ais);
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
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("continue"))) {
      navigate_to_screen(GameStateManager::Screen::CharacterCreation);
    }
  }

  {
    if (imm::button(context, mk(button_group.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("quit"))) {
      exit_game();
    }
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}
