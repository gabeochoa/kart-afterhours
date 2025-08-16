#include "../makers.h"
#include "../navigation.h"
#include "../preload.h"
#include "../ui_systems.h"

using namespace afterhours;

Screen ScheduleMainMenuUI::character_creation(Entity &entity,
                                              UIContext<InputAction> &context) {
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("character_creation"));

  auto top_left =
      imm::div(context, mk(elem.ent(), 0),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_padding(Padding{.top = screen_pct(0.02f),
                                         .left = screen_pct(0.02f),
                                         .bottom = pixels(0.f),
                                         .right = pixels(0.f)})
                   .with_absolute_position()
                   .with_debug_name("character_top_left"));

  if (imm::button(context, mk(top_left.ent(), 0),
                  ComponentConfig{}
                      .with_padding(Padding{.top = pixels(5.f),
                                            .left = pixels(0.f),
                                            .bottom = pixels(5.f),
                                            .right = pixels(0.f)})
                      .with_label("round settings"))) {
    navigation::to(GameStateManager::Screen::RoundSettings);
  }

  if (imm::button(context, mk(top_left.ent(), 1),
                  ComponentConfig{}
                      .with_padding(Padding{.top = pixels(5.f),
                                            .left = pixels(0.f),
                                            .bottom = pixels(5.f),
                                            .right = pixels(0.f)})
                      .with_label("back"))) {
    navigation::back();
  }

  size_t num_slots = players.size() + ais.size() + 1;
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
