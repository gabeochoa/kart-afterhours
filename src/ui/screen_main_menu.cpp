#include "../navigation.h"
#include "../preload.h"
#include "../ui_systems.h"

using namespace afterhours;

Screen ScheduleMainMenuUI::main_screen(Entity &entity,
                                       UIContext<InputAction> &context) {
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("main_screen"));
  auto top_left =
      imm::div(context, mk(elem.ent(), 0),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_padding(Padding{.top = screen_pct(0.02f),
                                         .left = screen_pct(0.02f),
                                         .bottom = pixels(0.f),
                                         .right = pixels(0.f)})
                   .with_absolute_position()
                   .with_debug_name("main_top_left"));

  if (imm::button(context, mk(top_left.ent(), 0),
                  ComponentConfig{}
                      .with_padding(Padding{.top = pixels(5.f),
                                            .left = pixels(0.f),
                                            .bottom = pixels(5.f),
                                            .right = pixels(0.f)})
                      .with_label("play"))) {
    navigation::to(GameStateManager::Screen::CharacterCreation);
  }

  if (imm::button(context, mk(top_left.ent(), 1),
                  ComponentConfig{}
                      .with_padding(Padding{.top = pixels(5.f),
                                            .left = pixels(0.f),
                                            .bottom = pixels(5.f),
                                            .right = pixels(0.f)})
                      .with_label("about"))) {
    navigation::to(GameStateManager::Screen::About);
  }

  if (imm::button(context, mk(top_left.ent(), 2),
                  ComponentConfig{}
                      .with_padding(Padding{.top = pixels(5.f),
                                            .left = pixels(0.f),
                                            .bottom = pixels(5.f),
                                            .right = pixels(0.f)})
                      .with_label("settings"))) {
    navigation::to(GameStateManager::Screen::Settings);
  }

  if (imm::button(context, mk(top_left.ent(), 3),
                  ComponentConfig{}
                      .with_padding(Padding{.top = pixels(5.f),
                                            .left = pixels(0.f),
                                            .bottom = pixels(5.f),
                                            .right = pixels(0.f)})
                      .with_label("exit"))) {
    exit_game();
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}
