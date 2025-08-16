#include "../navigation.h"
#include "../preload.h"
#include "../round_settings.h"
#include "../ui_systems.h"

using namespace afterhours;

void round_lives_settings(Entity &entity, UIContext<InputAction> &context) {
  auto &rl_settings = RoundManager::get().get_active_rt<RoundLivesSettings>();

  imm::div(context, mk(entity),
           ComponentConfig{}
               .with_label(
                   std::format("Num Lives: {}", rl_settings.num_starting_lives))
               .with_size(ComponentSize{percent(1.f), percent(0.2f)})
               .with_margin(Margin{.top = screen_pct(0.01f)}));
}

void round_kills_settings(Entity &entity, UIContext<InputAction> &context) {
  auto &rl_settings = RoundManager::get().get_active_rt<RoundKillsSettings>();

  imm::div(context, mk(entity),
           ComponentConfig{}
               .with_label(std::format("Round Length: {}",
                                       rl_settings.current_round_time))
               .with_size(ComponentSize{screen_pct(0.3f), screen_pct(0.06f)})
               .with_margin(Margin{.top = screen_pct(0.01f)}));

  {
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
          .with_label(std::format("Total Hippos: {}", rl_settings.total_hippos))
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

    if (imm::button(context, mk(settings_group.ent()),
                    ComponentConfig{}.with_label("select map"))) {
      navigation::to(GameStateManager::Screen::MapSelection);
    }

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

    if (imm::button(context, mk(settings_group.ent(), 2),
                    ComponentConfig{}
                        .with_padding(Padding{.top = pixels(5.f),
                                              .left = pixels(0.f),
                                              .bottom = pixels(5.f),
                                              .right = pixels(0.f)})
                        .with_label("back"))) {
      navigation::back();
    }
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}
