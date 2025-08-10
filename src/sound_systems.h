
#pragma once

#include "components.h"
#include "game_state_manager.h"
#include "input_mapping.h"
#include "sound_library.h"
#include "weapons.h"

// TODO this needs to be converted into a component with SoundAlias's
// in raylib a Sound can only be played once and hitting play again
// will restart it.
//
// we need to make aliases so that each one can play at the same time
struct CarRumble : System<Transform, CanShoot> {
  virtual void for_each_with(const Entity &, const Transform &,
                             const CanShoot &, float) const override {
    // SoundLibrary::get().play(SoundFile::Engine_Idle_Short);
  }
};

struct UISounds : System<> {
  input::PossibleInputCollector<InputAction> inpc;

  virtual bool should_run(float) override {
    return GameStateManager::get().is_menu_active();
  }

  virtual void once(float) override {
    inpc = input::get_input_collector<InputAction>();
  }

  template <typename TAction>
  void play_move_if_any(const TAction &actions_done) const {
    const bool is_move = actions_done.action == InputAction::WidgetLeft ||
                         actions_done.action == InputAction::WidgetRight ||
                         actions_done.action == InputAction::WidgetNext ||
                         actions_done.action == InputAction::WidgetBack;
    if (is_move) {
      SoundLibrary::get().play(SoundFile::UI_Move);
    }
  }

  virtual void for_each_with(Entity &, float) override {
    if (!inpc.has_value()) {
      return;
    }

    for (const auto &act : inpc.inputs_pressed()) {
      if (act.action == InputAction::WidgetPress) {
        SoundLibrary::get().play(SoundFile::UI_Select);
        continue;
      }
      play_move_if_any(act);
    }
  }
};
