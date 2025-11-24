#include "sound_systems.h"
#include "components.h"
#include "game_state_manager.h"
#include "input_mapping.h"
#include "music_library.h"
#include "query.h"
#include "sound_library.h"
#include <afterhours/src/plugins/sound_system.h>
#include "weapons.h"
#if __APPLE__
#include "rl.h"
#else
#include "raylib.h"
#endif
// afterhours UI
#include <afterhours/src/plugins/ui/components.h>
#include <afterhours/src/plugins/ui/context.h>
#include <afterhours/src/plugins/ui/systems.h>

// TODO this needs to be converted into a component with SoundAlias's
// in raylib a Sound can only be played once and hitting play again
// will restart it.
//
// we need to make aliases so that each one can play at the same time
struct CarRumble : afterhours::System<Transform, CanShoot> {
  virtual void for_each_with(const afterhours::Entity &, const Transform &,
                             const CanShoot &, float) const override {
    // SoundLibrary::get().play(SoundFile::Engine_Idle_Short);
  }
};

struct UIClickSounds
    : afterhours::ui::SystemWithUIContext<afterhours::ui::HasClickListener> {
  afterhours::ui::UIContext<InputAction> *context;

  virtual void once(float) override {
    context = afterhours::EntityHelper::get_singleton_cmp<
        afterhours::ui::UIContext<InputAction>>();
    // this->include_derived_children = true;
  }

  virtual void for_each_with(afterhours::Entity &, afterhours::ui::UIComponent &component,
                             afterhours::ui::HasClickListener &hasClickListener,
                             float) override {
    if (!GameStateManager::get().is_menu_active()) {
      return;
    }
    if (!component.was_rendered_to_screen) {
      return;
    }

    // If this element registered a click this frame, play the UI_Select sound
    if (hasClickListener.down) {
      auto opt = afterhours::EntityQuery({.force_merge = true})
                     .template whereHasComponent<afterhours::sound_system::SoundEmitter>()
                     .gen_first();
      if (opt.valid()) {
        afterhours::Entity &ent = opt.asE();
        afterhours::sound_system::PlaySoundRequest &req = ent.addComponentIfMissing<afterhours::sound_system::PlaySoundRequest>();
        req.policy = afterhours::sound_system::PlaySoundRequest::Policy::Name;
        req.name = sound_file_to_str(SoundFile::UI_Select);
      }
    }

    process_derived_children(component);
  }

private:
  void process_derived_children(afterhours::ui::UIComponent &parent_component) {
    for (auto child_id : parent_component.children) {
      auto child_entity = afterhours::EntityHelper::getEntityForID(child_id);
      if (!child_entity.has_value()) {
        continue;
      }

      afterhours::Entity &child = child_entity.asE();
      if (!child.has<afterhours::ui::UIComponent>() ||
          !child.has<afterhours::ui::HasClickListener>()) {
        continue;
      }

      auto &child_component = child.get<afterhours::ui::UIComponent>();
      auto &child_hasClickListener =
          child.get<afterhours::ui::HasClickListener>();

      if (!GameStateManager::get().is_menu_active()) {
        continue;
      }
      if (!child_component.was_rendered_to_screen) {
        continue;
      }

      // If this element registered a click this frame, play the UI_Select sound
      if (child_hasClickListener.down) {
        auto opt = afterhours::EntityQuery({.force_merge = true})
                       .template whereHasComponent<afterhours::sound_system::SoundEmitter>()
                       .gen_first();
        if (opt.valid()) {
          afterhours::Entity &ent = opt.asE();
          afterhours::sound_system::PlaySoundRequest &req = ent.addComponentIfMissing<afterhours::sound_system::PlaySoundRequest>();
          req.policy = afterhours::sound_system::PlaySoundRequest::Policy::Name;
          req.name = sound_file_to_str(SoundFile::UI_Select);
        }
      }

      process_derived_children(child_component);
    }
  }
};

struct BackgroundMusic : afterhours::System<> {
  bool started = false;

  virtual void once(float) override {
    if (!raylib::IsAudioDeviceReady())
      return;

    if (!started && GameStateManager::get().is_menu_active()) {
      auto &music = MusicLibrary::get().get("menu_music");
      music.looping = true;
      raylib::PlayMusicStream(music);
      started = true;
    }

    auto &music = MusicLibrary::get().get("menu_music");
    if (raylib::IsMusicStreamPlaying(music)) {
      raylib::UpdateMusicStream(music);
    }
  }

  virtual void for_each_with(afterhours::Entity &, float) override {}
};


// Binds UI input to sound requests instead of direct plays
struct UISoundBindingSystem : afterhours::System<> {
  input::PossibleInputCollector inpc;

  virtual bool should_run(float) override {
    return GameStateManager::get().is_menu_active();
  }

  virtual void once(float) override { inpc = input::get_input_collector(); }

  template <typename TAction>
  static void enqueue_move_if_any(const TAction &actions_done) {
    const bool is_move =
        action_matches(actions_done.action, InputAction::WidgetLeft) ||
        action_matches(actions_done.action, InputAction::WidgetRight) ||
        action_matches(actions_done.action, InputAction::WidgetNext) ||
        action_matches(actions_done.action, InputAction::WidgetBack);
    if (is_move) {
      auto opt = afterhours::EntityQuery({.force_merge = true})
                     .template whereHasComponent<afterhours::sound_system::SoundEmitter>()
                     .gen_first();
      if (opt.valid()) {
        afterhours::Entity &ent = opt.asE();
        afterhours::sound_system::PlaySoundRequest &req = ent.addComponentIfMissing<afterhours::sound_system::PlaySoundRequest>();
        req.policy = afterhours::sound_system::PlaySoundRequest::Policy::Name;
        req.name = sound_file_to_str(SoundFile::UI_Move);
      }
    }
  }

  virtual void for_each_with(afterhours::Entity &, float) override {
    if (!inpc.has_value()) {
      return;
    }
    for (const auto &act : inpc.inputs_pressed()) {
      enqueue_move_if_any(act);
    }
  }
};

void register_sound_systems(afterhours::SystemManager &systems) {
  systems.register_update_system(std::make_unique<BackgroundMusic>());
  systems.register_update_system(std::make_unique<UISoundBindingSystem>());
  systems.register_update_system(std::make_unique<UIClickSounds>());
  systems.register_render_system(std::make_unique<CarRumble>());
}