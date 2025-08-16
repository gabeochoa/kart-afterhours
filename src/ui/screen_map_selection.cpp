#include "../map_system.h"
#include "../navigation.h"
#include "../preload.h"
#include "../ui_systems.h"
#include "animation_key.h"
#include "reusable_components.h"
#include <afterhours/src/plugins/animation.h>

using namespace afterhours;

Screen ScheduleMainMenuUI::map_selection(Entity &entity,
                                         UIContext<InputAction> &context) {
  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_font(get_font_name(FontID::EQPro), 75.f)
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_flex_direction(FlexDirection::Row)
                   .with_absolute_position()
                   .with_debug_name("map_selection"));

  auto left_col =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(0.2f), percent(1.0f)})
                   .with_padding(Padding{.top = screen_pct(0.02f),
                                         .left = screen_pct(0.02f)})
                   .with_flex_direction(FlexDirection::Column)
                   .with_debug_name("map_selection_left"));

  auto preview_box =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(0.8f), percent(1.0f)})
                   .with_margin(Margin{.top = percent(0.05f),
                                       .bottom = percent(0.05f),
                                       .right = percent(0.05f)})
                   .with_opacity(0.0f)
                   .with_debug_name("preview_box")
                   .with_skip_tabbing(true));

  auto current_round_type = RoundManager::get().active_round_type;
  auto compatible_maps =
      MapManager::get().get_maps_for_round_type(current_round_type);
  auto selected_map_index = MapManager::get().get_selected_map();
  static int prev_preview_index = -2;
  static int last_effective_preview_index = -2;

  constexpr int NO_PREVIEW_INDEX = -1000;
  int hovered_preview_index = NO_PREVIEW_INDEX;
  int focused_preview_index = NO_PREVIEW_INDEX;
  static int persisted_hovered_preview_index = NO_PREVIEW_INDEX;

  {
    auto round_preview = imm::div(
        context, mk(left_col.ent(), 1),
        ComponentConfig{}
            .with_debug_name("round_settings_preview")
            .with_size(ComponentSize{percent(1.f), percent(0.3f, 0.5f)})
            .with_margin(Margin{.top = screen_pct(0.008f)}));

    render_round_settings_preview(context, round_preview.ent());
  }

  auto map_list =
      imm::div(context, mk(left_col.ent(), 2),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(1.f), percent(0.5f)})
                   .with_margin(Margin{.top = screen_pct(0.01f)})
                   .with_flex_direction(FlexDirection::Row)
                   .with_debug_name("map_list"));

  auto map_grid_button_size =
      ComponentSize{percent(0.48f), screen_pct(100.f / 720.f)};

  {
    float inner_margin = 0.01f;
    auto random_btn = imm::button(
        context,
        mk(map_list.ent(), static_cast<EntityID>(compatible_maps.size())),
        ComponentConfig{}
            .with_label("?")
            .with_size(map_grid_button_size)
            .with_margin(Margin{.top = percent(inner_margin),
                                .bottom = percent(inner_margin),
                                .left = percent(inner_margin),
                                .right = percent(inner_margin)})
            .with_flex_direction(FlexDirection::Row)
            .with_opacity(0.0f)
            .with_translate(-2000.0f, 0.0f)
            .with_debug_name("map_card_random"));

    size_t random_index = compatible_maps.size();
    afterhours::animation::one_shot(
        UIKey::MapCard, random_index,
        ui_anims::make_map_card_slide(random_index));

    static int random_card_anim_state = 0;
    float slide_v = 0.0f;
    if (auto mv =
            afterhours::animation::get_value(UIKey::MapCard, random_index);
        mv.has_value()) {
      slide_v = std::clamp(mv.value(), 0.0f, 1.0f);
      random_card_anim_state = 1;
    } else {
      if (random_card_anim_state == 1) {
        random_card_anim_state = 2;
        slide_v = 1.0f;
      } else if (random_card_anim_state == 2) {
        slide_v = 1.0f;
      } else {
        slide_v = 0.0f;
      }
    }

    auto opt_ent = afterhours::EntityHelper::getEntityForID(random_btn.id());
    if (opt_ent) {
      ui_reusable_components::apply_slide_mods(opt_ent.asE(), slide_v);
    }

    if (random_btn) {
      start_game_with_random_animation();
    }

    auto random_btn_id = random_btn.id();
    if (context.is_hot(random_btn_id)) {
      hovered_preview_index = MapManager::RANDOM_MAP_INDEX;
      persisted_hovered_preview_index = hovered_preview_index;
    }
    if (context.has_focus(random_btn_id)) {
      focused_preview_index = MapManager::RANDOM_MAP_INDEX;
    }
    {
      auto opt_ent = afterhours::EntityHelper::getEntityForID(random_btn_id);
      if (opt_ent) {
        auto &ent = opt_ent.asE();
        if (ent.has<afterhours::ui::UIComponent>()) {
          auto rect = ent.get<afterhours::ui::UIComponent>().rect();
          auto mp = context.mouse_pos;
          if (mp.x >= rect.x && mp.x <= rect.x + rect.width && mp.y >= rect.y &&
              mp.y <= rect.y + rect.height) {
            hovered_preview_index = MapManager::RANDOM_MAP_INDEX;
            persisted_hovered_preview_index = hovered_preview_index;
          }
        }
      }
    }
  }

  static int map_card_anim_state[256] = {0};
  for (size_t i = 0; i < compatible_maps.size(); i++) {
    const auto &map_pair = compatible_maps[i];
    const auto &map_config = map_pair.second;
    int map_index = map_pair.first;

    afterhours::animation::one_shot(UIKey::MapCard, i,
                                    ui_anims::make_map_card_slide(i));

    float pulse_v =
        afterhours::animation::get_value(UIKey::MapCardPulse, i).value_or(0.0f);
    float inner_margin_base = 0.02f;
    float inner_margin_scale = 0.004f;
    float inner_margin = inner_margin_base - (inner_margin_scale * pulse_v);

    float slide_v = 0.0f;
    if (auto mv = afterhours::animation::get_value(UIKey::MapCard, i);
        mv.has_value()) {
      slide_v = std::clamp(mv.value(), 0.0f, 1.0f);
      map_card_anim_state[i] = 1;
    } else {
      if (map_card_anim_state[i] == 1) {
        map_card_anim_state[i] = 2;
        slide_v = 1.0f;
      } else if (map_card_anim_state[i] == 2) {
        slide_v = 1.0f;
      } else {
        slide_v = 0.0f;
      }
    }

    auto map_btn =
        imm::button(context, mk(map_list.ent(), static_cast<EntityID>(i)),
                    ComponentConfig{}
                        .with_label(map_config.display_name)
                        .with_size(map_grid_button_size)
                        .with_margin(Margin{.top = percent(inner_margin),
                                            .bottom = percent(inner_margin),
                                            .left = percent(inner_margin),
                                            .right = percent(inner_margin)})
                        .with_flex_direction(FlexDirection::Row)
                        .with_opacity(0.0f)
                        .with_translate(-2000.0f, 0.0f)
                        .with_debug_name("map_card"));
    if (map_btn) {
      MapManager::get().set_selected_map(map_index);
      MapManager::get().create_map();
      GameStateManager::get().start_game();
    }

    auto btn_id = map_btn.id();
    {
      auto opt_ent = afterhours::EntityHelper::getEntityForID(btn_id);
      if (opt_ent) {
        auto &ent = opt_ent.asE();
        auto &mods =
            ent.addComponentIfMissing<afterhours::ui::HasUIModifiers>();
        (void)mods;
        ui_reusable_components::apply_slide_mods(ent, slide_v);
      }
    }

    if (context.is_hot(btn_id)) {
      hovered_preview_index = map_index;
      persisted_hovered_preview_index = hovered_preview_index;
    }
    if (context.has_focus(btn_id)) {
      focused_preview_index = map_index;
    }
    {
      auto opt_ent = afterhours::EntityHelper::getEntityForID(btn_id);
      if (opt_ent) {
        auto &ent = opt_ent.asE();
        if (ent.has<afterhours::ui::UIComponent>()) {
          auto rect = ent.get<afterhours::ui::UIComponent>().rect();
          float mod_x = rect.x;
          float mod_y = rect.y;
          float comp_tx = 0.0f;
          float comp_ty = 0.0f;
          if (ent.has<afterhours::ui::HasUIModifiers>()) {
            const auto &mods = ent.get<afterhours::ui::HasUIModifiers>();
            comp_tx = mods.translate_x;
            comp_ty = mods.translate_y;
            auto mod_rect = mods.apply_modifier(rect);
            mod_x = mod_rect.x;
            mod_y = mod_rect.y;
          }

          auto mp = context.mouse_pos;
          if (mp.x >= rect.x && mp.x <= rect.x + rect.width && mp.y >= rect.y &&
              mp.y <= rect.y + rect.height) {
            hovered_preview_index = map_index;
            persisted_hovered_preview_index = hovered_preview_index;
          }
        }
      }
    }
  }

  int effective_preview_index = selected_map_index;
  if (hovered_preview_index != NO_PREVIEW_INDEX) {
    effective_preview_index = hovered_preview_index;
  } else if (persisted_hovered_preview_index != NO_PREVIEW_INDEX) {
    effective_preview_index = persisted_hovered_preview_index;
  } else if (focused_preview_index != NO_PREVIEW_INDEX) {
    effective_preview_index = focused_preview_index;
  }

  if (effective_preview_index >= 0 && last_effective_preview_index < 0) {
    afterhours::animation::anim(UIKey::MapPreviewFade)
        .from(0.0f)
        .to(1.0f, 0.2f, afterhours::animation::EasingType::EaseOutQuad);
  } else if (effective_preview_index >= 0 &&
             last_effective_preview_index >= 0 &&
             effective_preview_index != last_effective_preview_index) {
    prev_preview_index = last_effective_preview_index;
    afterhours::animation::anim(UIKey::MapPreviewFade)
        .from(0.0f)
        .to(1.0f, 0.12f, afterhours::animation::EasingType::EaseOutQuad);
  }
  last_effective_preview_index = effective_preview_index;

  auto selected_map_it =
      std::find_if(compatible_maps.begin(), compatible_maps.end(),
                   [effective_preview_index](const auto &pair) {
                     return pair.first == effective_preview_index;
                   });

  auto maybe_shuffle =
      afterhours::animation::manager<UIKey>().get_value(UIKey::MapShuffle);
  bool overriding_preview = effective_preview_index != selected_map_index;
  render_map_preview(context, preview_box.ent(), effective_preview_index,
                     selected_map_index, compatible_maps, overriding_preview,
                     prev_preview_index);

  if (imm::button(context, mk(left_col.ent()),
                  ComponentConfig{}
                      .with_padding(Padding{.top = pixels(5.f),
                                            .left = pixels(0.f),
                                            .bottom = pixels(5.f),
                                            .right = pixels(0.f)})
                      .with_label("back"))) {
    navigation::back();
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}
