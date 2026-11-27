#pragma once

#include "game_state_manager.h"
#include "input_mapping.h"
#include "ui/animation_control.h"
#include "ui/navigation.h"
#include <afterhours/src/plugins/e2e_testing/e2e_testing.h>
#include <afterhours/src/core/key_codes.h>
#include <magic_enum/magic_enum.hpp>

namespace e2e_commands {

using namespace afterhours;
using namespace afterhours::testing;

// Game e2e commands, registered with afterhours' typed register_command:
// argument-count and parse errors are produced by the library, and the
// handler sees only unconsumed commands with that name. (Previously two
// hand-rolled System<PendingE2ECommand> classes repeated the has_args /
// fail boilerplate per command.)
inline void register_app_commands(SystemManager &sm) {
  register_command<std::string>(
      sm, "goto_screen",
      [](PendingE2ECommand &cmd, const std::string &name) {
        std::optional<GameStateManager::Screen> screen;
        if (name == "Main" || name == "main") {
          screen = GameStateManager::Screen::Main;
        } else if (name == "Settings" || name == "settings") {
          screen = GameStateManager::Screen::Settings;
        } else if (name == "About" || name == "about") {
          screen = GameStateManager::Screen::About;
        } else if (name == "CharacterCreation" || name == "character_creation") {
          screen = GameStateManager::Screen::CharacterCreation;
        } else if (name == "RoundSettings" || name == "round_settings") {
          screen = GameStateManager::Screen::RoundSettings;
        } else if (name == "MapSelection" || name == "map_selection") {
          screen = GameStateManager::Screen::MapSelection;
        } else if (name == "RoundEnd" || name == "round_end") {
          screen = GameStateManager::Screen::RoundEnd;
        }

        if (!screen.has_value()) {
          cmd.fail("Unknown screen: " + name);
          return;
        }

        navigation::to(screen.value());
        cmd.consume();
      });

  register_command<std::string>(
      sm, "action", [](PendingE2ECommand &cmd, const std::string &name) {
        auto *ctx = EntityHelper::get_singleton_cmp<ui::UIContext<InputAction>>();
        if (!ctx) {
          cmd.fail("UIContext not found");
          return;
        }

        auto action = magic_enum::enum_cast<InputAction>(name);
        if (!action) {
          cmd.fail("Unknown action: " + name);
          return;
        }

        ctx->last_action = *action;
        cmd.consume();
      });
}

// disable_animations/enable_animations used to be handled here. afterhours
// ships them now, and registers its handler in register_builtin_handlers --
// which runs first and consumes the command, so ours never fired. Our own
// animations read the engine's flag via animation_control::disabled().

} // namespace e2e_commands
