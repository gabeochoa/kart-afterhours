
#pragma once

#include "components.h"
#include "game_state_manager.h"
#include "input_mapping.h"
#include "music_library.h"
#include "sound_library.h"
#include "weapons.h"
#include "query.h"
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
      auto opt = EntityQuery({.force_merge = true})
                     .whereHasComponent<SoundEmitter>()
                     .gen_first();
      if (opt.valid()) {
        auto &ent = opt.asE();
        auto &req = ent.addComponentIfMissing<PlaySoundRequest>();
        req = PlaySoundRequest(SoundFile::UI_Move);
      }
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
      auto opt = EntityQuery({.force_merge = true})
                     .whereHasComponent<SoundEmitter>()
                     .gen_first();
      if (opt.valid()) {
        auto &ent = opt.asE();
        auto &req = ent.addComponentIfMissing<PlaySoundRequest>();
        req = PlaySoundRequest(SoundFile::UI_Select);
      }
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

// Centralized playback that supports aliasing/prefix policies
struct SoundPlaybackSystem : System<PlaySoundRequest> {
  virtual void for_each_with(Entity &entity, PlaySoundRequest &req, float) override {
    // Fetch audio singleton emitter (for alias settings)
    SoundEmitter *emitter = EntityHelper::get_singleton_cmp<SoundEmitter>();
    // Dispatch by policy
    switch (req.policy) {
    case PlaySoundRequest::Policy::Enum: {
      const char *name = sound_file_to_str(req.file);
      play_with_alias_or_name(emitter, name, req.prefer_alias);
    } break;
    case PlaySoundRequest::Policy::Name: {
      play_with_alias_or_name(emitter, req.name.c_str(), req.prefer_alias);
    } break;
    case PlaySoundRequest::Policy::PrefixRandom: {
      SoundLibrary::get().play_random_match(req.prefix);
    } break;
    case PlaySoundRequest::Policy::PrefixFirstAvailable: {
      SoundLibrary::get().play_first_available_match(req.prefix);
    } break;
    case PlaySoundRequest::Policy::PrefixIfNonePlaying: {
      SoundLibrary::get().play_if_none_playing(req.prefix);
    } break;
    }

    // One-shot request, remove after processing
    entity.removeComponent<PlaySoundRequest>();
  }

  static void ensure_alias_names(SoundEmitter *emitter, const std::string &base,
                                 int copies) {
    if (!emitter)
      return;
    if (emitter->alias_names_by_base.contains(base))
      return;
    std::vector<std::string> names;
    names.reserve(static_cast<size_t>(copies));
    for (int i = 0; i < copies; ++i) {
      names.push_back(base + std::string("_a") + std::to_string(i));
    }
    emitter->alias_names_by_base[base] = std::move(names);
    emitter->next_alias_index_by_base[base] = 0;
  }

  static bool library_contains(const std::string &name) {
    // There is no public contains on SoundLibrary, but Library has contains
    // through impl; expose via play/get try-catch alternative
    try {
      (void)SoundLibrary::get().get(name);
      return true;
    } catch (...) {
      return false;
    }
  }

  static void play_with_alias_or_name(SoundEmitter *emitter, const char *name,
                                      bool prefer_alias) {
    const std::string base{name};
 
    if (prefer_alias && emitter) {
      ensure_alias_names(emitter, base, emitter->default_alias_copies);
      auto &names = emitter->alias_names_by_base[base];
      auto &idx = emitter->next_alias_index_by_base[base];
 
      // Find a non-playing alias if possible
      for (size_t i = 0; i < names.size(); ++i) {
        size_t try_index = (idx + i) % names.size();
        const std::string &alias_name = names[try_index];
        if (library_contains(alias_name)) {
          auto &s = SoundLibrary::get().get(alias_name);
          if (!raylib::IsSoundPlaying(s)) {
            raylib::PlaySound(s);
            idx = (try_index + 1) % names.size();
            return;
          }
        }
      }
      // All busy or no aliases loaded; try multi-play for concurrency
      try {
        auto &s = SoundLibrary::get().get(base);
        raylib::PlaySoundMulti(s);
      } catch (...) {
        // Fallback
        SoundLibrary::get().play(name);
      }
      idx = (idx + 1) % names.size();
      return;
    }
 
    // No alias preference or no emitter: try multi-play for concurrency
    try {
      auto &s = SoundLibrary::get().get(base);
      raylib::PlaySoundMulti(s);
    } catch (...) {
      SoundLibrary::get().play(name);
    }
  }
};

// Binds UI input to sound requests instead of direct plays
struct UISoundBindingSystem : System<> {
  input::PossibleInputCollector<InputAction> inpc;

  virtual bool should_run(float) override {
    return GameStateManager::get().is_menu_active();
  }

  virtual void once(float) override { inpc = input::get_input_collector<InputAction>(); }

  template <typename TAction>
  static void enqueue_move_if_any(const TAction &actions_done) {
    const bool is_move = actions_done.action == InputAction::WidgetLeft ||
                         actions_done.action == InputAction::WidgetRight ||
                         actions_done.action == InputAction::WidgetNext ||
                         actions_done.action == InputAction::WidgetBack;
    if (is_move) {
      auto opt = EntityQuery({.force_merge = true}).whereHasComponent<SoundEmitter>().gen_first();
      if (opt.valid()) {
        auto &ent = opt.asE();
        auto &req = ent.addComponentIfMissing<PlaySoundRequest>();
        req = PlaySoundRequest(SoundFile::UI_Move);
      }
    }
  }

  virtual void for_each_with(Entity &, float) override {
    if (!inpc.has_value()) {
      return;
    }
    for (const auto &act : inpc.inputs_pressed()) {
      enqueue_move_if_any(act);
    }
  }
};
