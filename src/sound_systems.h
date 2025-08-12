
#pragma once

#include "components.h"
#include "game_state_manager.h"
#include "input_mapping.h"
#include "music_library.h"
#include "sound_library.h"
#include "weapons.h"
// afterhours UI
#include <afterhours/src/plugins/ui/components.h>
#include <afterhours/src/plugins/ui/context.h>
#include <afterhours/src/plugins/ui/systems.h>

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

struct UIMoveSounds : System<> {
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
      play_move_if_any(act);
    }
  }
};

struct UIClickSounds
    : afterhours::ui::SystemWithUIContext<afterhours::ui::HasClickListener> {
  afterhours::ui::UIContext<InputAction> *context = nullptr;

  virtual void once(float) override {
    context = afterhours::EntityHelper::get_singleton_cmp<
        afterhours::ui::UIContext<InputAction>>();
    this->include_derived_children = true;
  }

  virtual void
  for_each_with_derived(Entity &entity, afterhours::ui::UIComponent &component,
                        afterhours::ui::HasClickListener &hasClickListener,
                        float) {
    if (!GameStateManager::get().is_menu_active()) {
      return;
    }
    if (!component.was_rendered_to_screen) {
      return;
    }

    // If this element registered a click this frame, play the UI_Select sound
    if (hasClickListener.down) {
      SoundLibrary::get().play(SoundFile::UI_Select);
    }
  }
};

struct BackgroundMusic : System<> {
  bool started = false;

  virtual void once(float) override {
    if (!raylib::IsAudioDeviceReady())
      return;

    if (!started && GameStateManager::get().is_menu_active()) {
      auto &music = MusicLibrary::get().get("menu_music");
      music.looping = true;
      // TODO when we change the music, we should change this back to 0
      raylib::SeekMusicStream(music, 3.0f);
      raylib::PlayMusicStream(music);
      started = true;
    }

    auto &music = MusicLibrary::get().get("menu_music");
    raylib::UpdateMusicStream(music);
  }

  virtual void for_each_with(Entity &, float) override {}
};
