
#include "ui_systems.h"

#include "input_mapping.h"
#include "makers.h"  // make_ai()
#include "preload.h" // FontID
#include "round_settings.h"
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

  if (active_screen == Screen::Settings) {
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
  inpc = input::get_input_collector<InputAction>();
  if (active_screen == Screen::None) {
    ui_visible = false;
  }

  const bool start_pressed =
      std::ranges::any_of(inpc.inputs_pressed(), [](const auto &actions_done) {
        return actions_done.action == InputAction::WidgetMod;
      });

  if (!ui_visible && start_pressed) {
    active_screen = Screen::Main;
    ui_visible = true;
  } else if (ui_visible && start_pressed) {
    ui_visible = false;
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

  auto bg_color = car.has_value() ? car->get<HasColor>().color()
                                  : colorManager.get_next_NO_STORE(index);

  const auto num_cols = std::min(4.f, static_cast<float>(num_slots));
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

ScheduleMainMenuUI::Screen
ScheduleMainMenuUI::character_creation(Entity &entity,
                                       UIContext<InputAction> &context) {
  Screen next_active_screen = active_screen;
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute(true)
                   .with_debug_name("character_creation"));

  {
    if (imm::button(
            context, mk(elem.ent()),
            ComponentConfig{}.with_padding(button_padding).with_label("go"))) {
      next_active_screen = Screen::None;
    }
  }

  {
    if (imm::button(context, mk(elem.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("back"))) {
      next_active_screen = Screen::Main;
    }
  }

  {
    if (imm::button(context, mk(elem.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("round settings"))) {
      next_active_screen = Screen::RoundSettings;
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
    case RoundType::Tag:
      // TODO currently no settings
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
          .with_absolute(true)
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

  return next_active_screen;
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

ScheduleMainMenuUI::Screen
ScheduleMainMenuUI::round_settings(Entity &entity,
                                   UIContext<InputAction> &context) {
  Screen next_active_screen = active_screen;
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_debug_name("round_settings")
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute(true));

  auto settings_group =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_debug_name("settings_group")
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_padding(button_group_padding)
                   .with_absolute(true));

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
  case RoundType::Tag:
    // TODO currently no settings
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
      next_active_screen = Screen::CharacterCreation;
    }
  }
  return next_active_screen;
}

ScheduleMainMenuUI::Screen
ScheduleMainMenuUI::main_screen(Entity &entity,
                                UIContext<InputAction> &context) {
  Screen next_active_screen = active_screen;
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute(true)
                   .with_debug_name("main_screen"));

  auto btn_group =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_padding(button_group_padding)
                   .with_absolute(true)
                   .with_debug_name("btn_group"));

  {
    if (imm::button(context, mk(btn_group.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("play"))) {
      next_active_screen = Screen::CharacterCreation;
    }
  }

  {
    if (imm::button(context, mk(btn_group.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("about"))) {
      next_active_screen = Screen::About;
    }
  }

  {
    if (imm::button(context, mk(btn_group.ent()),
                    ComponentConfig{}
                        .with_padding(button_padding)
                        .with_label("settings"))) {
      next_active_screen = Screen::Settings;
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

  return next_active_screen;
}

ScheduleMainMenuUI::Screen
ScheduleMainMenuUI::settings_screen(Entity &entity,
                                    UIContext<InputAction> &context) {
  Screen next_active_screen = active_screen;
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute(true)
                   .with_debug_name("main_screen"));

  auto control_group =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_padding(control_group_padding)
                   .with_absolute(true)
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
                    Settings::get().get_fullscreen_enabled())) {
    Settings::get().toggle_fullscreen();
  }

  if (imm::button(
          context, mk(control_group.ent()),
          ComponentConfig{}.with_padding(button_padding).with_label("back"))) {
    Settings::get().update_resolution(
        current_resolution_provider->current_resolution);
    // TODO do we want to write the settings, or should we have a save button?

    next_active_screen = Screen::Main;
  }
  return next_active_screen;
}

ScheduleMainMenuUI::Screen
ScheduleMainMenuUI::about_screen(Entity &entity,
                                 UIContext<InputAction> &context) {
  Screen next_active_screen = active_screen;
  if (!current_resolution_provider)
    return next_active_screen;

  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute(true)
                   .with_debug_name("about_screen"));

  auto about_group =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_padding(control_group_padding)
                   .with_absolute(true)
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

  if (imm::button(
          context, mk(about_group.ent()),
          ComponentConfig{}.with_padding(button_padding).with_label("back"))) {
    next_active_screen = Screen::Main;
  }
  return next_active_screen;
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
