#include "sound_systems.h"
#include "../components.h"
#include "../game_state_manager.h"
#include "../input_mapping.h"
#include "../query.h"
#include "../library/sound_library.h"
#include <afterhours/src/plugins/sound_system.h>
#include "../weapons.h"
// rl.h does its own raylib lookup on every platform. The old #else branch
// included "../raylib.h", which does not exist, so it never compiled.
#include "../rl.h"
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

// Menu music on the upstream MusicLibrary (bus "music", on the
// "music" volume). The stream itself is pumped by
// sound_system::MusicUpdateSystem; this system only starts/stops it
// with a fade at menu <-> game transitions. The old version played
// through kart's own MusicLibrary with raylib calls in once() -- which
// runs exactly once, so the stream was never pumped, and the system
// was never registered at all (see register_sound_systems below).
struct BackgroundMusic : System<> {
  // State machine, not an is_playing() poll: a stubbed headless stream
  // never reports playing, so polling would replay (and re-fade) every
  // frame. `started` is only cleared once the fade-out has actually
  // stopped the stream (or it was never playing).
  bool started = false;
  bool fading_out = false;

  virtual void for_each_with(Entity &, float) override {
    // No IsAudioDeviceReady guard: headless, the music buffers are
    // stubs and plays are no-ops, but play() still records the play
    // for e2e assertions and the stream pump guards null buffers.
    auto &music_lib = sound_system::MusicLibrary::get();
    if (!music_lib.contains("menu_music"))
      return;
    if (GameStateManager::get().is_menu_active()) {
      if (fading_out) {
        // Back to the menu mid-fade-out: cancel the stop, fade back in.
        fading_out = false;
        sound_system::fade_to("menu_music", 1.f, 0.5f);
      } else if (!started) {
        // Fade in from silence rather than starting at full volume.
        music_lib.get("menu_music").looping = true;
        music_lib.set_fade_level("menu_music", 0.f);
        music_lib.play("menu_music");
        sound_system::fade_to("menu_music", 1.f, 0.5f);
        started = true;
      }
    } else if (started && !fading_out) {
      fading_out = true;
      sound_system::stop_with_fade("menu_music", 0.5f);
    } else if (fading_out && !music_lib.is_playing("menu_music")) {
      // Fade-out landed and the stream stopped (headless: it never
      // reported playing, so this clears on the next frame).
      started = false;
      fading_out = false;
    }
  }
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