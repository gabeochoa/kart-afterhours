#pragma once

#include "../game_state_manager.h"

using Screen = GameStateManager::Screen;

struct MenuNavigationStack : afterhours::BaseComponent {
  std::vector<Screen> stack;
  bool ui_visible{true};
};

namespace navigation {

void to(Screen screen);
void back();

} // namespace navigation

struct NavigationSystem : afterhours::System<> {
  input::PossibleInputCollector inpc;

  virtual void once(float) override;
};
