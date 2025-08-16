#include "../navigation.h"
#include "../preload.h"
#include "../settings.h"
#include "../ui_systems.h"

using namespace afterhours;

Screen ScheduleMainMenuUI::settings_screen(Entity &entity,
                                           UIContext<InputAction> &context) {
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("settings_screen"));
  {
    auto top_left =
        imm::div(context, mk(elem.ent(), 0),
                 ComponentConfig{}
                     .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                     .with_padding(Padding{.top = screen_pct(0.02f),
                                           .left = screen_pct(0.02f),
                                           .bottom = pixels(0.f),
                                           .right = pixels(0.f)})
                     .with_absolute_position()
                     .with_debug_name("settings_top_left"));
    if (imm::button(context, mk(top_left.ent(), 0),
                    ComponentConfig{}
                        .with_padding(Padding{.top = pixels(5.f),
                                              .left = pixels(0.f),
                                              .bottom = pixels(5.f),
                                              .right = pixels(0.f)})
                        .with_label("back"))) {
      Settings::get().update_resolution(
          EntityHelper::get_singleton_cmp<
              window_manager::ProvidesCurrentResolution>()
              ->current_resolution);
      navigation::back();
    }
  }
  auto control_group =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_padding(Padding{.top = screen_pct(0.4f),
                                         .left = screen_pct(0.4f),
                                         .bottom = pixels(0.f),
                                         .right = pixels(0.f)})
                   .with_absolute_position()
                   .with_debug_name("control_group"));

  float master_volume = Settings::get().get_master_volume();
  if (auto result =
          slider(context, mk(control_group.ent(), 0), master_volume,
                 ComponentConfig{}.with_label(fmt::format(
                     "Master Volume\n {:2.0f}", master_volume * 100.f)))) {
    master_volume = result.as<float>();
    Settings::get().update_master_volume(master_volume);
  }

  float music_volume = Settings::get().get_music_volume();
  if (auto result =
          slider(context, mk(control_group.ent(), 1), music_volume,
                 ComponentConfig{}.with_label(fmt::format(
                     "Music Volume\n {:2.0f}", music_volume * 100.f)))) {
    music_volume = result.as<float>();
    Settings::get().update_music_volume(music_volume);
  }

  float sfx_volume = Settings::get().get_sfx_volume();
  if (auto result = slider(context, mk(control_group.ent(), 2), sfx_volume,
                           ComponentConfig{}.with_label(fmt::format(
                               "SFX Volume\n {:2.0f}", sfx_volume * 100.f)))) {
    sfx_volume = result.as<float>();
    Settings::get().update_sfx_volume(sfx_volume);
  }

  if (imm::dropdown(context, mk(control_group.ent(), 3), resolution_strs,
                    resolution_index,
                    ComponentConfig{}.with_label("Resolution"))) {
    resolution_provider->on_data_changed(resolution_index);
  }

  if (imm::checkbox(context, mk(control_group.ent(), 4),
                    Settings::get().get_fullscreen_enabled(),
                    ComponentConfig{}.with_label("Fullscreen"))) {
    Settings::get().toggle_fullscreen();
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}
