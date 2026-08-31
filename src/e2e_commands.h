#pragma once

#include "components.h"
#include "game_state_manager.h"
#include "input_mapping.h"
#include "makers.h"
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

// Karts in creation order. Indexed rather than keyed on PlayerID because AI
// karts do not carry one, and a script that adds both needs to address either.
// Entities destroyed this frame are still in the list with cleanup set, and a
// script that just killed one must not see it.
inline std::vector<RefEntity> cars() {
    return EntityQuery({.ignore_temp_warning = true})
        .whereHasComponent<HasMultipleLives>()
        .whereLambda([](const Entity &e) { return !e.cleanup; })
        .gen();
}

inline OptEntity nth_car(size_t n) {
    auto all = cars();
    return n < all.size() ? OptEntity{all[n]} : OptEntity{};
}

// Startup spawns make_player(0) before any script runs, so scripts that never
// add a kart still mean "the car" when they say set_tracker.
inline OptEntity first_car() { return nth_car(0); }

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
    // Whole seconds is all a script needs: it only ever pushes this far enough
    // into the past to clear the tag cooldown.
    if (field == "last_tag_time" && car.has<HasTagAndGoTracking>())
        return static_cast<int>(car.get<HasTagAndGoTracking>().last_tag_time);
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
    if (field == "last_tag_time" && car.has<HasTagAndGoTracking>()) {
        car.get<HasTagAndGoTracking>().last_tag_time =
            static_cast<float>(value);
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
            return tracker_write(cmd, 0);
        if (cmd.is("set_tracker_at"))
            return tracker_write(cmd, 1);
        if (cmd.is("expect_tracker"))
            return tracker_read(cmd, 0);
        if (cmd.is("expect_tracker_at"))
            return tracker_read(cmd, 1);
        if (cmd.is("add_ai"))
            return add_ai(cmd);
        if (cmd.is("add_player"))
            return add_player(cmd);
        if (cmd.is("remove_ai"))
            return remove_ai(cmd);
        if (cmd.is("remove_player"))
            return remove_player(cmd);
        if (cmd.is("expect_car_count"))
            return expect_car_count(cmd);
        if (cmd.is("warp_car"))
            return warp_car(cmd);
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

    // The `_at` forms take a leading kart index; base is how many args that
    // costs, so both spellings share one body.
    static OptEntity tracker_car(PendingE2ECommand &cmd, size_t base) {
        if (!cmd.has_args(base + 2)) {
            cmd.fail(cmd.name + " requires " +
                     (base ? "<n> <field> <value>" : "<field> <value>"));
            return {};
        }
        const size_t idx = base ? static_cast<size_t>(cmd.arg_as<int>(0)) : 0;
        auto car = detail::nth_car(idx);
        if (!car.valid())
            cmd.fail(cmd.name + ": no car at index " + std::to_string(idx));
        return car;
    }

    static void tracker_write(PendingE2ECommand &cmd, size_t base) {
        auto car = tracker_car(cmd, base);
        if (!car.valid())
            return;
        if (!detail::tracker_set(car.asE(), cmd.arg(base),
                                 cmd.arg_as<int>(base + 1))) {
            cmd.fail(cmd.name + ": unknown field " + cmd.arg(base));
            return;
        }
        cmd.consume();
    }

    static void tracker_read(PendingE2ECommand &cmd, size_t base) {
        auto car = tracker_car(cmd, base);
        if (!car.valid())
            return;
        const auto actual = detail::tracker_get(car.asE(), cmd.arg(base));
        if (!actual) {
            cmd.fail(cmd.name + ": unknown field " + cmd.arg(base));
            return;
        }
        const int expected = cmd.arg_as<int>(base + 1);
        if (*actual != expected) {
            cmd.fail("expected " + cmd.arg(base) + " == " +
                     std::to_string(expected) + " but was " +
                     std::to_string(*actual));
            return;
        }
        cmd.consume();
    }

    // Karts added mid-round miss reset_car_trackers and are invisible to
    // MatchKartsToPlayers, which only reconciles from the menu -- so a script
    // that spawns one while Playing gets a kart in an undefined state.
    static bool require_menu(PendingE2ECommand &cmd) {
        if (GameStateManager::get().is_menu_active())
            return true;
        cmd.fail(cmd.name + ": only valid from the menu");
        return false;
    }

    static void add_ai(PendingE2ECommand &cmd) {
        if (!require_menu(cmd))
            return;
        make_ai();
        cmd.consume();
    }

    static void add_player(PendingE2ECommand &cmd) {
        if (!cmd.has_args(1)) {
            cmd.fail("add_player requires <gamepad_id>");
            return;
        }
        const int id = cmd.arg_as<int>(0);
        if (!require_menu(cmd))
            return;
        // Three players is one too many: MatchKartsToPlayers' "a player left"
        // branch fires at size() > count() + 1, and with no gamepad connected
        // count() is 1, so it would delete the ones it just found.
        auto players = EntityQuery({.force_merge = true})
                           .whereHasComponent<PlayerID>()
                           .gen();
        for (Entity &p : players) {
            if (p.get<PlayerID>().id == id) {
                cmd.fail("add_player: player " + std::to_string(id) +
                         " already exists");
                return;
            }
        }
        if (players.size() >= 2) {
            cmd.fail("add_player: two players is the most the menu will keep");
            return;
        }
        make_player(static_cast<input::GamepadID>(id));
        cmd.consume();
    }

    static void remove_ai(PendingE2ECommand &cmd) {
        auto ais = EntityQuery({.force_merge = true})
                       .whereHasComponent<AIControlled>()
                       .gen();
        if (ais.empty()) {
            cmd.fail("remove_ai: no AI karts");
            return;
        }
        for (Entity &ai : ais)
            ai.cleanup = true;
        cmd.consume();
    }

    static void remove_player(PendingE2ECommand &cmd) {
        if (!cmd.has_args(1)) {
            cmd.fail("remove_player requires <gamepad_id>");
            return;
        }
        const int id = cmd.arg_as<int>(0);
        for (Entity &p : EntityQuery({.force_merge = true})
                             .whereHasComponent<PlayerID>()
                             .gen()) {
            if (p.get<PlayerID>().id != id)
                continue;
            p.cleanup = true;
            cmd.consume();
            return;
        }
        cmd.fail("remove_player: no player " + std::to_string(id));
    }

    static void expect_car_count(PendingE2ECommand &cmd) {
        if (!cmd.has_args(1)) {
            cmd.fail("expect_car_count requires <n>");
            return;
        }
        const size_t actual = detail::cars().size();
        const size_t expected = static_cast<size_t>(cmd.arg_as<int>(0));
        if (actual != expected) {
            cmd.fail("expected " + std::to_string(expected) + " karts but was " +
                     std::to_string(actual));
            return;
        }
        cmd.consume();
    }

    // Spawn points are a fifth of the arena apart, so nothing ever touches on
    // its own. Collision-driven rules need the karts put on top of each other.
    static void warp_car(PendingE2ECommand &cmd) {
        if (!cmd.has_args(3)) {
            cmd.fail("warp_car requires <n> <x> <y>");
            return;
        }
        auto car = detail::nth_car(static_cast<size_t>(cmd.arg_as<int>(0)));
        if (!car.valid()) {
            cmd.fail("warp_car: no car at index " + cmd.arg(0));
            return;
        }
        Transform &transform = car.asE().get<Transform>();
        transform.position = vec2{cmd.arg_as<float>(1), cmd.arg_as<float>(2)};
        transform.velocity = vec2{0.f, 0.f};
        cmd.consume();
    }
};

inline void register_app_commands(SystemManager &sm) {
    sm.register_update_system(std::make_unique<HandleGotoScreenCommand>());
    sm.register_update_system(std::make_unique<HandleAppActionCommand>());
    sm.register_update_system(std::make_unique<HandleGameplayCommand>());
}

}
