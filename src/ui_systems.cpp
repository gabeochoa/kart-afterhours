
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
    Entity &parent, UIContext<InputAction> &context, size_t index,
    size_t num_slots) {

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
  auto column = imm::div(context, mk(parent, (int)index),
                         ComponentConfig{
                             .size =
                                 ComponentSize{
                                     percent(1.f / num_cols, 0.1f),
                                     percent(1.f, 0.4f),
                                 },
                             .margin =
                                 Margin{
                                     .top = percent(0.05f),
                                     .bottom = percent(0.05f),
                                     .left = percent(0.05f),
                                     .right = percent(0.05f),
                                 },
                             // disable round
                             .rounded_corners = std::bitset<4>().reset(),
                             .color_usage = Theme::Usage::Custom,
                             .custom_color = bg_color,
                         });

  if (car.has_value()) {
    // Note: we arent using mk() here...
    imm::div(context, mk(column.ent()),
             ComponentConfig{
                 .size =
                     ComponentSize{
                         percent(1.f),
                         percent(0.2f, 0.4f),
                     },
                 // disable round
                 .rounded_corners = std::bitset<4>().reset(),
                 .label = std::format("{} {}", index, car->id),
                 .debug_name = std::format("player_car {} {} {} {}", index,
                                           car->id, players.size(), ais.size()),
                 .color_usage = Theme::Usage::Custom,
                 .custom_color = bg_color,
             });
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
            ComponentConfig{
                .size =
                    ComponentSize{
                        percent(1.f),
                        percent(0.2f, 0.4f),
                    },
                // disable round
                .rounded_corners = std::bitset<4>().reset(),
                .label = "Next Color",
                .skip_when_tabbing = true,
                .debug_name = std::format("next_color button {} ", index),
            });
        elem || player_right) {
      colorManager.release_and_get_next(car->id);
    }
  }

  // we are the last boi
  if (num_slots <= input::MAX_GAMEPAD_ID && is_last_slot) {
    if (imm::button(context, mk(column.ent()),
                    ComponentConfig{
                        .size =
                            ComponentSize{
                                percent(1.f),
                                pixels(50.f),
                            },
                        // disable round
                        .rounded_corners = std::bitset<4>().reset(),
                        .padding =
                            Padding{
                                .top = percent(0.25f),
                            },
                        .label = "Add AI",
                        .debug_name = "add_ai_button",
                    })) {
      make_ai();
    }
  }

  if (is_slot_ai && car.has_value()) {
    if (imm::button(context, mk(column.ent()),
                    ComponentConfig{
                        .size =
                            ComponentSize{
                                percent(1.f),
                                pixels(50.f),
                            },
                        // disable round
                        .rounded_corners = std::bitset<4>().reset(),
                        .padding =
                            Padding{
                                .top = percent(0.25f),
                            },
                        .label = "Remove AI",
                        .debug_name = "remove_ai_button",
                    })) {

      colorManager.release_only(car->id);
      car->cleanup = true;
    }
  }
}

ScheduleMainMenuUI::Screen
ScheduleMainMenuUI::character_creation(Entity &entity,
                                       UIContext<InputAction> &context) {
  Screen next_active_screen = active_screen;
  auto elem = imm::div(context, mk(entity));
  {
    elem.ent()
        .get<UIComponent>()
        .enable_font(get_font_name(FontID::EQPro), 75.f)
        .set_desired_width(screen_pct(1.f))
        .set_desired_height(screen_pct(1.f))
        .make_absolute();
    elem.ent().get<ui::UIComponentDebug>().set("character_creation");
  }

  {
    ComponentConfig _button_config;
    _button_config.padding = button_padding;
    _button_config.label = "go";
    if (imm::button(context, mk(elem.ent()), std::move(_button_config))
        //
    ) {
      next_active_screen = Screen::None;
    }
  }

  {
    ComponentConfig back_button_config;
    back_button_config.padding = button_padding;
    back_button_config.label = "back";
    if (imm::button(context, mk(elem.ent()), std::move(back_button_config))
        //
    ) {
      next_active_screen = Screen::Main;
    }
  }

  {
    ComponentConfig button_config;
    button_config.padding = button_padding;
    button_config.label = "round settings";

    if (imm::button(context, mk(elem.ent()), std::move(button_config))
        //
    ) {
      next_active_screen = Screen::RoundSettings;
    }
  }

  size_t num_slots = players.size() + ais.size() + 1;
  // 0-4 => 1, 5->8 -> 2
  int fours = static_cast<int>(std::ceil(static_cast<float>(num_slots) / 4.f));

  auto btn_group =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{
                   .size =
                       ComponentSize{
                           screen_pct(1.f),
                           screen_pct(1.f),
                       },
                   .margin =
                       Margin{
                           .top = screen_pct(fours == 1 ? 0.2f : 0.05f),
                           .left = screen_pct(0.05f),
                           .right = screen_pct(0.05f),
                       },
                   .is_absolute = true,
                   .debug_name = "btn_group",
               });

  for (int row_id = 0; row_id < fours; row_id++) {
    auto row = imm::div(context, mk(btn_group.ent(), row_id),
                        ComponentConfig{
                            .size =
                                ComponentSize{
                                    percent(1.f),
                                    percent(0.5f, 0.4f),
                                },
                            .flex_direction = FlexDirection::Row,
                            .debug_name = "row",
                        });
    size_t start = row_id * 4;
    for (size_t i = start; i < std::min(num_slots, start + 4); i++) {
      character_selector_column(row.ent(), context, i, num_slots);
    }
  }

  return next_active_screen;
}

void round_lives_settings(Entity &entity, UIContext<InputAction> &context) {
  auto &rl_settings = RoundManager::get().get_active_rt<RoundLivesSettings>();

  imm::div(
      context, mk(entity),
      ComponentConfig{
          .label = std::format("Num Lives: {}", rl_settings.num_starting_lives),
      });
}

void round_kills_settings(Entity &entity, UIContext<InputAction> &context) {
  auto &rl_settings = RoundManager::get().get_active_rt<RoundKillsSettings>();

  imm::div(context, mk(entity),
           ComponentConfig{
               .label = std::format("Round Length: {}",
                                    rl_settings.current_round_time),
           });

  {
    ComponentConfig resolution_config;
    resolution_config.label = "Round Length";

    // TODO replace with actual strings
    auto options = magic_enum::enum_names<RoundKillsSettings::TimeOptions>();
    auto option_index = magic_enum::enum_index(rl_settings.time_option).value();

    if (auto result = imm::dropdown(context, mk(entity), options, option_index,
                                    std::move(resolution_config));
        result) {
      rl_settings.set_time_option(result.as<int>());
    }
  }
}

void round_score_settings(Entity &entity, UIContext<InputAction> &context) {
  auto &rl_settings = RoundManager::get().get_active_rt<RoundScoreSettings>();

  imm::div(context, mk(entity),
           ComponentConfig{
               .label = std::format("Score Needed: {}",
                                    rl_settings.score_needed_to_win),
           });
}

ScheduleMainMenuUI::Screen
ScheduleMainMenuUI::round_settings(Entity &entity,
                                   UIContext<InputAction> &context) {
  Screen next_active_screen = active_screen;
  auto elem = imm::div(context, mk(entity));
  {
    elem.ent()
        .get<UIComponent>()
        .enable_font(get_font_name(FontID::EQPro), 75.f)
        .set_desired_width(screen_pct(1.f))
        .set_desired_height(screen_pct(1.f))
        .make_absolute();
    elem.ent().get<ui::UIComponentDebug>().set("round_settings");
  }

  auto settings_group = imm::div(context, mk(elem.ent()));
  {
    settings_group.ent()
        .get<UIComponent>()
        .set_desired_width(screen_pct(1.f))
        .set_desired_height(screen_pct(1.f))
        .set_desired_padding(button_group_padding)
        .make_absolute();
    settings_group.ent().get<ui::UIComponentDebug>().set("settings_group");
  }

  {
    ComponentConfig cmp_config;
    cmp_config.label = "Win Condition";

    size_t win_condition_index =
        enum_to_index(RoundManager::get().active_round_type);

    if (auto result =
            imm::pagination(context, mk(settings_group.ent()), RoundType_NAMES,
                            win_condition_index, std::move(cmp_config));
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
  default:
    log_error("You need to add a handler for UI settings for round type {}",
              (int)RoundManager::get().active_round_type);
    break;
  }

  {
    ComponentConfig back_button_config;
    back_button_config.padding = button_padding;
    back_button_config.label = "back";

    if (imm::button(context, mk(settings_group.ent()),
                    std::move(back_button_config))
        //
    ) {
      next_active_screen = Screen::CharacterCreation;
    }
  }
  return next_active_screen;
}

ScheduleMainMenuUI::Screen
ScheduleMainMenuUI::main_screen(Entity &entity,
                                UIContext<InputAction> &context) {
  Screen next_active_screen = active_screen;
  auto elem = imm::div(context, mk(entity));
  {
    elem.ent()
        .get<UIComponent>()
        .enable_font(get_font_name(FontID::EQPro), 75.f)
        .set_desired_width(screen_pct(1.f))
        .set_desired_height(screen_pct(1.f))
        .make_absolute();
    elem.ent().get<ui::UIComponentDebug>().set("main_screen");
  }

  auto btn_group = imm::div(context, mk(elem.ent()));
  {
    btn_group.ent()
        .get<UIComponent>()
        .set_desired_width(screen_pct(1.f))
        .set_desired_height(screen_pct(1.f))
        .set_desired_padding(button_group_padding)
        .make_absolute();
    btn_group.ent().get<ui::UIComponentDebug>().set("btn_group");
  }

  {
    ComponentConfig play_button_config;
    play_button_config.padding = button_padding;
    play_button_config.label = "play";

    if (imm::button(context, mk(btn_group.ent()), std::move(play_button_config))
        //
    ) {
      next_active_screen = Screen::CharacterCreation;
    }
  }

  {
    ComponentConfig about_button_config;
    about_button_config.padding = button_padding;
    about_button_config.label = "about";

    if (imm::button(context, mk(btn_group.ent()),
                    std::move(about_button_config))
        //
    ) {
      next_active_screen = Screen::About;
    }
  }

  {
    ComponentConfig settings_button_config;
    settings_button_config.padding = button_padding;
    settings_button_config.label = "settings";

    if (imm::button(context, mk(btn_group.ent()),
                    std::move(settings_button_config))
        //
    ) {
      next_active_screen = Screen::Settings;
    }
  }

  {
    ComponentConfig exit_button_config;
    exit_button_config.padding = button_padding;
    exit_button_config.label = "exit";

    if (imm::button(context, mk(btn_group.ent()), std::move(exit_button_config))
        //
    ) {
      exit_game();
    }
  }

  return next_active_screen;
}

ScheduleMainMenuUI::Screen
ScheduleMainMenuUI::settings_screen(Entity &entity,
                                    UIContext<InputAction> &context) {
  Screen next_active_screen = active_screen;
  auto elem = imm::div(context, mk(entity));
  {
    elem.ent()
        .get<UIComponent>()
        .enable_font(get_font_name(FontID::EQPro), 75.f)
        .set_desired_width(screen_pct(1.f))
        .set_desired_height(screen_pct(1.f))
        .make_absolute();
    elem.ent().get<ui::UIComponentDebug>().set("main_screen");
  }

  ComponentConfig control_group_config;
  control_group_config.size = {screen_pct(1.f), screen_pct(1.f)};
  control_group_config.padding = control_group_padding;
  control_group_config.is_absolute = true;
  control_group_config.debug_name = "control_group";

  auto control_group =
      imm::div(context, mk(elem.ent()), std::move(control_group_config));

  {
    float master_volume = Settings::get().get_master_volume();
    auto label = fmt::format("Master Volume\n {:2.0f}", master_volume * 100.f);

    ComponentConfig master_volume_config;
    master_volume_config.size = {pixels(300.f), pixels(50.f)};
    master_volume_config.label = std::move(label);

    if (auto result = slider(context, mk(control_group.ent()), master_volume,
                             std::move(master_volume_config));
        result) {
      master_volume = result.as<float>();
      Settings::get().update_master_volume(master_volume);
    }
  }

  {
    float music_volume = Settings::get().get_music_volume();
    auto label = fmt::format("Music Volume\n {:2.0f}", music_volume * 100.f);

    ComponentConfig music_volume_config;
    music_volume_config.size = {pixels(300.f), pixels(50.f)};
    music_volume_config.label = std::move(label);

    if (auto result = slider(context, mk(control_group.ent()), music_volume,
                             std::move(music_volume_config));
        result) {
      music_volume = result.as<float>();
      Settings::get().update_music_volume(music_volume);
    }
  }

  {
    float sfx_volume = Settings::get().get_sfx_volume();
    auto label = fmt::format("SFX Volume\n {:2.0f}", sfx_volume * 100.f);

    ComponentConfig sfx_volume_config;
    sfx_volume_config.size = {pixels(300.f), pixels(50.f)};
    sfx_volume_config.label = std::move(label);

    if (auto result = slider(context, mk(control_group.ent()), sfx_volume,
                             std::move(sfx_volume_config));
        result) {
      sfx_volume = result.as<float>();
      Settings::get().update_sfx_volume(sfx_volume);
    }
  }

  {
    ComponentConfig resolution_config;
    resolution_config.label = "Resolution";

    if (imm::dropdown(context, mk(control_group.ent()), resolution_strs,
                      resolution_index, std::move(resolution_config))) {
      resolution_provider->on_data_changed(resolution_index);
    }
  }

  if (imm::checkbox(context, mk(control_group.ent()),
                    Settings::get().get_fullscreen_enabled())) {
    Settings::get().toggle_fullscreen();
  }

  ComponentConfig back_button_config;
  back_button_config.padding = button_padding;
  back_button_config.label = "back";

  if (imm::button(context, mk(control_group.ent()),
                  std::move(back_button_config))
      //
  ) {
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

  auto elem = imm::div(context, mk(entity));
  {
    elem.ent()
        .get<UIComponent>()
        .enable_font(get_font_name(FontID::EQPro), 75.f)
        .set_desired_width(screen_pct(1.f))
        .set_desired_height(screen_pct(1.f))
        .make_absolute();
    elem.ent().get<ui::UIComponentDebug>().set("about_screen");
  }

  ComponentConfig about_screen_config;
  about_screen_config.size = {screen_pct(1.f), screen_pct(1.f)};
  about_screen_config.padding = control_group_padding;
  about_screen_config.is_absolute = true;
  about_screen_config.debug_name = "control_group";

  auto about_group =
      imm::div(context, mk(elem.ent()), std::move(about_screen_config));

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

  ComponentConfig back_button_config;
  back_button_config.padding = button_padding;
  back_button_config.label = "back";

  if (imm::button(context, mk(about_group.ent()), std::move(back_button_config))
      //
  ) {
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
    auto elem = imm::div(context, mk(entity));
    elem.ent()
        .get<UIComponent>()
        // TODO use config once it exists
        .set_flex_direction(FlexDirection::Row);

    // Max speed
    {
      auto max_speed_label =
          fmt::format("Max Speed\n {:.2f} m/s", Config::get().max_speed.data);
      auto pct = Config::get().max_speed.get_pct();

      ComponentConfig max_speed_slider_config;
      max_speed_slider_config.label = std::move(max_speed_label);
      max_speed_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(max_speed_slider_config));
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

      ComponentConfig breaking_acceleration_slider_config;
      breaking_acceleration_slider_config.label =
          std::move(max_breaking_acceleration_label);
      breaking_acceleration_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(breaking_acceleration_slider_config));
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

      ComponentConfig forward_acceleration_slider_config;
      forward_acceleration_slider_config.label =
          std::move(forward_acceleration_label);
      forward_acceleration_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(forward_acceleration_slider_config));
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

      ComponentConfig reverse_acceleration_slider_config;
      reverse_acceleration_slider_config.label =
          std::move(reverse_acceleration_label);
      reverse_acceleration_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(reverse_acceleration_slider_config));
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

      ComponentConfig boost_acceleration_slider_config;
      boost_acceleration_slider_config.label =
          std::move(boost_acceleration_label);
      boost_acceleration_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(boost_acceleration_slider_config));
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

      ComponentConfig boost_decay_slider_config;
      boost_decay_slider_config.label = std::move(boost_decay_label);
      boost_decay_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(boost_decay_slider_config));
          result) {
        Config::get().boost_decay_percent.set_pct(result.as<float>());
      }
    }
  }

  // Row 2
  {
    auto elem = imm::div(context, mk(entity));
    elem.ent()
        .get<UIComponent>()
        // TODO use config once it exists
        .set_flex_direction(FlexDirection::Row);

    // Skid Threshold
    {
      auto skid_threshold_label = fmt::format(
          "Skid \nThreshold \n {:.2f} %", Config::get().skid_threshold.data);
      auto pct = Config::get().skid_threshold.get_pct();

      ComponentConfig skid_threshold_slider_config;
      skid_threshold_slider_config.label = std::move(skid_threshold_label);
      skid_threshold_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(skid_threshold_slider_config));
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

      ComponentConfig steering_sensitivity_slider_config;
      steering_sensitivity_slider_config.label =
          std::move(steering_sensitivity_label);
      steering_sensitivity_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(steering_sensitivity_slider_config));
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

      ComponentConfig minimum_steering_radius_slider_config;
      minimum_steering_radius_slider_config.label =
          std::move(minimum_steering_radius_label);
      minimum_steering_radius_slider_config.skip_when_tabbing = true;

      if (auto result =
              slider(context, mk(elem.ent()), pct,
                     std::move(minimum_steering_radius_slider_config));
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

      ComponentConfig maximum_steering_radius_slider_config;
      maximum_steering_radius_slider_config.label =
          std::move(maximum_steering_radius_label);
      maximum_steering_radius_slider_config.skip_when_tabbing = true;

      if (auto result =
              slider(context, mk(elem.ent()), pct,
                     std::move(maximum_steering_radius_slider_config));
          result) {
        Config::get().maximum_steering_radius.set_pct(result.as<float>());
      }
    }

    // Collision Scalar
    {
      auto collision_scalar_label = fmt::format(
          "Collision \nScalar \n {:.4f}", Config::get().collision_scalar.data);
      auto pct = Config::get().collision_scalar.get_pct();

      ComponentConfig collision_scalar_slider_config;
      collision_scalar_slider_config.label = std::move(collision_scalar_label);
      collision_scalar_slider_config.skip_when_tabbing = true;

      if (auto result = slider(context, mk(elem.ent()), pct,
                               std::move(collision_scalar_slider_config));
          result) {
        Config::get().collision_scalar.set_pct(result.as<float>());
      }
    }
  }
}
