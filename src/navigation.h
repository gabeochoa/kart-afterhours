#pragma once

#include "std_include.h"

#include "library.h"
#include "game_state_manager.h"
#include "input_mapping.h"

using Screen = GameStateManager::Screen;

struct MenuNavigationStack : BaseComponent {
  std::vector<Screen> stack;
  bool ui_visible{true};
};

namespace navigation {

void to(Screen screen);
void back();

} // namespace navigation

struct NavigationSystem : System<> {
  input::PossibleInputCollector inpc;

  virtual void once(float) override;
};
