#pragma once

#include "components.h"
#include "game_state_manager.h"
#include "input_mapping.h"
#include "query.h"
#include "round_settings.h"
#include "ui/animation_control.h"
#include "ui/navigation.h"
#include <afterhours/src/plugins/e2e_testing/e2e_testing.h>
#include <afterhours/src/core/key_codes.h>
#include <magic_enum/magic_enum.hpp>
#include <optional>

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

struct HandleAppActionCommand : System<PendingE2ECommand> {
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

// disable_animations/enable_animations used to be handled here. afterhours
// ships them now, and registers its handler in register_builtin_handlers --
// which runs first and consumes the command, so ours never fired. Our own
// animations read the engine's flag via animation_control::disabled().

namespace detail {

// e2e spawns exactly one car (make_player(0) at startup), so "the car" is
// unambiguous. Every per-round tracker the reset fix touches lives on it.
inline OptEntity first_car() {
    return EntityQuery({.ignore_temp_warning = true})
        .whereHasComponent<HasMultipleLives>()
        .gen_first();
}

// is_tagger is a bool but reads as 0/1, so one int-valued accessor pair covers
// every tracker a script needs to see.
inline std::optional<int> tracker_get(const Entity &car,
                                      const std::string &field) {
    if (field == "lives")
        return car.get<HasMultipleLives>().num_lives_remaining;
    if (field == "kills" && car.has<HasKillCountTracker>())
        return car.get<HasKillCountTracker>().kills;
    if (field == "hippos" && car.has<HasHippoCollection>())
        return car.get<HasHippoCollection>().hippos_collected;
    if (field == "health" && car.has<HasHealth>())
        return car.get<HasHealth>().amount;
    if (field == "is_tagger" && car.has<HasTagAndGoTracking>())
        return car.get<HasTagAndGoTracking>().is_tagger ? 1 : 0;
    return std::nullopt;
}

inline bool tracker_set(Entity &car, const std::string &field, int value) {
    if (field == "lives") {
        car.get<HasMultipleLives>().num_lives_remaining = value;
        return true;
    }
    if (field == "kills" && car.has<HasKillCountTracker>()) {
        car.get<HasKillCountTracker>().kills = value;
        return true;
    }
    if (field == "hippos" && car.has<HasHippoCollection>()) {
        car.get<HasHippoCollection>().hippos_collected = value;
        return true;
    }
    if (field == "health" && car.has<HasHealth>()) {
        car.get<HasHealth>().amount = value;
        return true;
    }
    if (field == "is_tagger" && car.has<HasTagAndGoTracking>()) {
        car.get<HasTagAndGoTracking>().is_tagger = value != 0;
        return true;
    }
    return false;
}

} // namespace detail

// Gameplay commands. The vocabulary was menu-only, so nothing could start a
// round -- these drive RoundManager/GameStateManager directly instead of
// clicking through Character Select. That is deliberate: a scripted round has
// to be reproducible, and physics plus AI are not.
//
// One system rather than six: the handlers are three lines each and share the
// tracker accessors above.
struct HandleGameplayCommand : System<PendingE2ECommand> {
    void for_each_with(Entity &, PendingE2ECommand &cmd, float) override {
        if (cmd.is_consumed())
            return;
        if (cmd.is("set_round_type"))
            return set_round_type(cmd);
        if (cmd.is("start_round"))
            return start_round(cmd);
        if (cmd.is("end_round"))
            return end_round(cmd);
        if (cmd.is("pause_round"))
            return pause_round(cmd);
        if (cmd.is("unpause_round"))
            return unpause_round(cmd);
        if (cmd.is("expect_state"))
            return expect_state(cmd);
        if (cmd.is("set_tracker"))
            return set_tracker(cmd);
        if (cmd.is("expect_tracker"))
            return expect_tracker(cmd);
    }

private:
    static void set_round_type(PendingE2ECommand &cmd) {
        if (!cmd.has_args(1)) {
            cmd.fail("set_round_type requires a round type name");
            return;
        }
        auto round_type = magic_enum::enum_cast<RoundType>(cmd.arg(0));
        if (!round_type) {
            cmd.fail("Unknown round type: " + cmd.arg(0));
            return;
        }
        RoundManager::get().set_active_round_type(
            static_cast<int>(*round_type));
        cmd.consume();
    }

    static void start_round(PendingE2ECommand &cmd) {
        GameStateManager::get().start_game();
        cmd.consume();
    }

    static void end_round(PendingE2ECommand &cmd) {
        GameStateManager::get().end_game();
        cmd.consume();
    }

    // pause_game/unpause_game are no-ops from the wrong state, so a script
    // that paused from the menu would sail past and then assert against a
    // pause card that was never drawn. Fail where the mistake is.
    static void pause_round(PendingE2ECommand &cmd) {
        if (!GameStateManager::get().is_game_active()) {
            cmd.fail("pause_round: no round is playing");
            return;
        }
        GameStateManager::get().pause_game();
        cmd.consume();
    }

    static void unpause_round(PendingE2ECommand &cmd) {
        if (!GameStateManager::get().is_paused()) {
            cmd.fail("unpause_round: not paused");
            return;
        }
        GameStateManager::get().unpause_game();
        cmd.consume();
    }

    static void expect_state(PendingE2ECommand &cmd) {
        if (!cmd.has_args(1)) {
            cmd.fail("expect_state requires Menu, Playing or Paused");
            return;
        }
        const auto actual = magic_enum::enum_name(
            GameStateManager::get().current_state);
        if (cmd.arg(0) != actual) {
            cmd.fail("expected game state " + cmd.arg(0) + " but was " +
                     std::string(actual));
            return;
        }
        cmd.consume();
    }

    static void set_tracker(PendingE2ECommand &cmd) {
        if (!cmd.has_args(2)) {
            cmd.fail("set_tracker requires <field> <value>");
            return;
        }
        auto car = detail::first_car();
        if (!car.valid()) {
            cmd.fail("set_tracker: no car found");
            return;
        }
        if (!detail::tracker_set(car.asE(), cmd.arg(0),
                                 cmd.arg_as<int>(1))) {
            cmd.fail("set_tracker: unknown field " + cmd.arg(0));
            return;
        }
        cmd.consume();
    }

    static void expect_tracker(PendingE2ECommand &cmd) {
        if (!cmd.has_args(2)) {
            cmd.fail("expect_tracker requires <field> <value>");
            return;
        }
        auto car = detail::first_car();
        if (!car.valid()) {
            cmd.fail("expect_tracker: no car found");
            return;
        }
        const auto actual = detail::tracker_get(car.asE(), cmd.arg(0));
        if (!actual) {
            cmd.fail("expect_tracker: unknown field " + cmd.arg(0));
            return;
        }
        const int expected = cmd.arg_as<int>(1);
        if (*actual != expected) {
            cmd.fail("expected " + cmd.arg(0) + " == " +
                     std::to_string(expected) + " but was " +
                     std::to_string(*actual));
            return;
        }
        cmd.consume();
    }
};

inline void register_app_commands(SystemManager &sm) {
    sm.register_update_system(std::make_unique<HandleGotoScreenCommand>());
    sm.register_update_system(std::make_unique<HandleAppActionCommand>());
    sm.register_update_system(std::make_unique<HandleGameplayCommand>());
}

}
