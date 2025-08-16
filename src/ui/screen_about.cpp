#include "../navigation.h"
#include "../preload.h"
#include "../ui_systems.h"

using namespace afterhours;

Screen ScheduleMainMenuUI::about_screen(Entity &entity,
                                        UIContext<InputAction> &context) {
  if (!current_resolution_provider)
    return GameStateManager::get().active_screen;

  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("about_screen"));

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
                     .with_debug_name("about_top_left"));
    if (imm::button(context, mk(top_left.ent(), 0),
                    ComponentConfig{}
                        .with_padding(Padding{.top = pixels(5.f),
                                              .left = pixels(0.f),
                                              .bottom = pixels(5.f),
                                              .right = pixels(0.f)})
                        .with_label("back"))) {
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

  raylib::Texture2D sheet = EntityHelper::get_singleton_cmp<
                                afterhours::texture_manager::HasSpritesheet>()
                                ->texture;
  const auto scale = 5.f;

  const std::array<afterhours::texture_manager::Rectangle, 3> about_frames{
      afterhours::texture_manager::idx_to_sprite_frame(0, 4),
      afterhours::texture_manager::idx_to_sprite_frame(1, 4),
      afterhours::texture_manager::idx_to_sprite_frame(2, 4),
  };

  imm::icon_row(context, mk(control_group.ent()), sheet, about_frames, scale,
                ComponentConfig{}
                    .with_size(ComponentSize{percent(1.f), percent(0.4f)})
                    .with_margin(Margin{.top = percent(0.1f)})
                    .with_debug_name("about_icons"));

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}
