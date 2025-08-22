#include "../preload.h"
#include "../ui_systems.h"

using namespace afterhours;

bool SchedulePauseUI::should_run(float) {
  inpc = input::get_input_collector<InputAction>();
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
                   .with_font(get_font_name(FontID::EQPro), 75.f)
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
               .with_label("paused")
               .with_font(get_font_name(FontID::EQPro), 100.f)
               .with_skip_tabbing(true)
               .with_size(ComponentSize{pixels(400.f), pixels(100.f)}));

  if (imm::button(context, mk(left_col.ent(), 1),
                  ComponentConfig{}
                      .with_padding(Padding{.top = pixels(5.f),
                                            .left = pixels(0.f),
                                            .bottom = pixels(5.f),
                                            .right = pixels(0.f)})
                      .with_label("resume"))) {
    GameStateManager::get().unpause_game();
  }

  if (imm::button(context, mk(left_col.ent(), 2),
                  ComponentConfig{}
                      .with_padding(Padding{.top = pixels(5.f),
                                            .left = pixels(0.f),
                                            .bottom = pixels(5.f),
                                            .right = pixels(0.f)})
                      .with_label("back to setup"))) {
    GameStateManager::get().end_game();
  }

  if (imm::button(context, mk(left_col.ent(), 3),
                  ComponentConfig{}
                      .with_padding(Padding{.top = pixels(5.f),
                                            .left = pixels(0.f),
                                            .bottom = pixels(5.f),
                                            .right = pixels(0.f)})
                      .with_label("exit game"))) {
    exit_game();
  }
}
