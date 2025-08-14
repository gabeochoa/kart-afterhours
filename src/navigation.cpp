#include "navigation.h"

#include "query.h"

static inline MenuNavigationStack &nav() {
  return *afterhours::EntityHelper::get_singleton_cmp<MenuNavigationStack>();
}

namespace navigation {

void to(Screen screen) {
	auto &gsm = GameStateManager::get();
  if (gsm.active_screen != screen) {
    nav().stack.push_back(gsm.active_screen);
  }
  gsm.set_next_screen(screen);
}

void back() {
	auto &gsm = GameStateManager::get();
	// If we're on the main screen, exit the game
  if (gsm.active_screen == GameStateManager::Screen::Main || nav().stack.empty()) {
    running = false;
    return;
  }
  Screen previous = nav().stack.back();
  nav().stack.pop_back();
  gsm.set_next_screen(previous);
}

} // namespace navigation

void NavigationSystem::once(float) {
	inpc = input::get_input_collector<InputAction>();

	// Initialize stack with Main on first run if needed
	auto &n = nav();
  if (n.stack.empty()) {
    if (GameStateManager::get().active_screen != GameStateManager::Screen::Main) {
      n.stack.push_back(GameStateManager::Screen::Main);
    }
  }

	// Baseline UI visibility from game state (then allow toggle)
	if (GameStateManager::get().is_game_active()) {
		n.ui_visible = false;
	} else if (GameStateManager::get().is_menu_active()) {
		n.ui_visible = true;
	}

	// Toggle UI visibility with WidgetMod (start button)
  const bool start_pressed = std::ranges::any_of(
      inpc.inputs_pressed(), [](const auto &a) { return a.action == InputAction::WidgetMod; });
  if (!n.ui_visible && start_pressed) {
    navigation::to(GameStateManager::Screen::Main);
    n.ui_visible = true;
  } else if (n.ui_visible && start_pressed) {
    n.ui_visible = false;
  }

	// Back navigation on escape
  const bool escape_pressed = std::ranges::any_of(
      inpc.inputs_pressed(), [](const auto &a) { return a.action == InputAction::MenuBack; });
  if (escape_pressed && n.ui_visible) {
    navigation::back();
  }
}