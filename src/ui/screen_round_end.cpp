#include "../navigation.h"
#include "../preload.h"
#include "../round_settings.h"
#include "../ui_systems.h"
#include "animation_key.h"
#include <afterhours/src/plugins/animation.h>

using namespace afterhours;

Screen ScheduleMainMenuUI::round_end_screen(Entity &entity,
                                            UIContext<InputAction> &context) {
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("round_end_screen"));

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
  }

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
