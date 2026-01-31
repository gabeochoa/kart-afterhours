#pragma once

#include "game_state_manager.h"
#include "input_mapping.h"
#include "ui/navigation.h"
#include <afterhours/src/plugins/e2e_testing/e2e_testing.h>
#include <magic_enum/magic_enum.hpp>

namespace e2e_commands {

using namespace afterhours;
using namespace afterhours::testing;

struct HandleGotoScreenCommand : System<PendingE2ECommand> {
    void for_each_with(Entity &, PendingE2ECommand &cmd, float) override {
        if (cmd.is_consumed() || !cmd.is("goto_screen"))
            return;
        if (!cmd.has_args(1)) {
            cmd.fail("goto_screen requires screen name");
            return;
        }

        const std::string &name = cmd.arg(0);
        
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
    }
};

struct HandleActionCommand : System<PendingE2ECommand> {
    void for_each_with(Entity &, PendingE2ECommand &cmd, float) override {
        if (cmd.is_consumed() || !cmd.is("action"))
            return;
        if (!cmd.has_args(1)) {
            cmd.fail("action requires action name");
            return;
        }

        auto *ctx = EntityHelper::get_singleton_cmp<ui::UIContext<InputAction>>();
        if (!ctx) {
            cmd.fail("UIContext not found");
            return;
        }

        auto action = magic_enum::enum_cast<InputAction>(cmd.arg(0));
        if (!action) {
            cmd.fail("Unknown action: " + cmd.arg(0));
            return;
        }

        ctx->last_action = *action;
        cmd.consume();
    }
};

inline void register_app_commands(SystemManager &sm) {
    sm.register_update_system(std::make_unique<HandleGotoScreenCommand>());
    sm.register_update_system(std::make_unique<HandleActionCommand>());
}

}
