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
#include <afterhours/src/plugins/ui/components.h>
#include <afterhours/src/plugins/ui/context.h>
#include <afterhours/src/plugins/ui/systems.h>

using namespace afterhours;

struct CarRumble : System<Transform, CanShoot> {
  virtual void for_each_with(const Entity &, const Transform &,
                             const CanShoot &, float) const override {
  }
};

struct UIClickSounds
    : ui::SystemWithUIContext<ui::HasClickListener> {
  ui::UIContext<InputAction> *context;

  virtual void once(float) override {
    context = EntityHelper::get_singleton_cmp<
        ui::UIContext<InputAction>>();
  }

  virtual void for_each_with(Entity &, ui::UIComponent &component,
                             ui::HasClickListener &hasClickListener,
                             float) override {
    if (should_play_click_sound(component, hasClickListener)) {
      play_click_sound();
    }

    process_derived_children(component);
  }

private:
  bool should_play_click_sound(const ui::UIComponent &component,
                               const ui::HasClickListener &hasClickListener) const {
    if (!GameStateManager::get().is_menu_active()) {
      return false;
    }
    if (!component.was_rendered_to_screen) {
      return false;
    }
    return hasClickListener.down;
  }

  void play_click_sound() {
    auto opt = EntityQuery({.force_merge = true})
                   .template whereHasComponent<sound_system::SoundEmitter>()
                   .gen_first();
    if (opt.valid()) {
      Entity &ent = opt.asE();
      sound_system::PlaySoundRequest &req = ent.addComponentIfMissing<sound_system::PlaySoundRequest>();
      req.policy = sound_system::PlaySoundRequest::Policy::Name;
      req.name = sound_file_to_str(SoundFile::UI_Select);
    }
  }

  void process_derived_children(ui::UIComponent &parent_component) {
    for (auto child_id : parent_component.children) {
      auto child_entity = EntityHelper::getEntityForID(child_id);
      if (!child_entity.has_value()) {
        continue;
      }

      Entity &child = child_entity.asE();
      if (!child.has<ui::UIComponent>() ||
          !child.has<ui::HasClickListener>()) {
        continue;
      }

      auto &child_component = child.get<ui::UIComponent>();
      auto &child_hasClickListener =
          child.get<ui::HasClickListener>();

      if (should_play_click_sound(child_component, child_hasClickListener)) {
        play_click_sound();
      }

      process_derived_children(child_component);
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
      raylib::PlayMusicStream(music);
      started = true;
    }

    auto &music = MusicLibrary::get().get("menu_music");
    if (raylib::IsMusicStreamPlaying(music)) {
      raylib::UpdateMusicStream(music);
    }
  }

  virtual void for_each_with(Entity &, float) override {}
};


struct UISoundBindingSystem : System<> {
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
      auto opt = EntityQuery({.force_merge = true})
                     .template whereHasComponent<sound_system::SoundEmitter>()
                     .gen_first();
      if (opt.valid()) {
        Entity &ent = opt.asE();
        sound_system::PlaySoundRequest &req = ent.addComponentIfMissing<sound_system::PlaySoundRequest>();
        req.policy = sound_system::PlaySoundRequest::Policy::Name;
        req.name = sound_file_to_str(SoundFile::UI_Move);
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

void register_sound_systems(SystemManager &systems) {
  systems.register_update_system(std::make_unique<BackgroundMusic>());
  systems.register_update_system(std::make_unique<UISoundBindingSystem>());
  systems.register_update_system(std::make_unique<UIClickSounds>());
  systems.register_render_system(std::make_unique<CarRumble>());
}