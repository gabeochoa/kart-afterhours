#include "reusable_components.h"
#include "../preload.h"
#include "../texture_library.h"
#include <afterhours/src/plugins/texture_manager.h>

using namespace afterhours;

namespace ui_reusable_components {

// Reusable player card component
ElementResult create_player_card(
    UIContext<InputAction> &context, Entity &parent, const std::string &label,
    raylib::Color bg_color, bool is_ai, std::optional<int> ranking,
    std::optional<std::string> stats_text, std::function<void()> on_next_color,
    std::function<void()> on_remove, bool show_add_ai,
    std::function<void()> on_add_ai,
    std::optional<AIDifficulty::Difficulty> ai_difficulty,
    std::function<void(AIDifficulty::Difficulty)> on_difficulty_change) {

  auto card = imm::div(context, mk(parent),
                       ComponentConfig{}
                           .with_size(ComponentSize{percent(1.f), percent(1.f)})
                           .with_margin(Margin{.top = percent(0.1f),
                                               .bottom = percent(0.1f),
                                               .left = percent(0.1f),
                                               .right = percent(0.1f)})
                           .with_color_usage(Theme::Usage::Custom)
                           .with_custom_color(bg_color)
                           .disable_rounded_corners());

  std::string player_label = label;
  if (is_ai) {
    player_label += " (AI)";
  }

  auto header_row =
      imm::div(context, mk(card.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
                   .with_flex_direction(FlexDirection::Row)
                   .with_debug_name("player_card_header"));

  bool show_next_color_icon = static_cast<bool>(on_next_color);
  bool show_remove_icon = is_ai && static_cast<bool>(on_remove);
  int header_icon_count =
      (show_next_color_icon ? 1 : 0) + (show_remove_icon ? 1 : 0);
  const float header_icon_width = 0.2f;
  float label_width =
      1.f - (header_icon_width * static_cast<float>(header_icon_count));
  if (label_width < 0.2f)
    label_width = 0.2f;

  imm::div(context, mk(header_row.ent()),
           ComponentConfig{}
               .with_size(ComponentSize{percent(label_width), percent(1.f)})
               .with_label(player_label)
               .with_color_usage(Theme::Usage::Custom)
               .with_custom_color(bg_color)
               .disable_rounded_corners()
               .with_debug_name("player_card_label"));

  if (stats_text.has_value()) {
    imm::div(context, mk(card.ent(), 1),
             ComponentConfig{}
                 .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
                 .with_label(stats_text.value())
                 .with_color_usage(Theme::Usage::Custom)
                 .with_custom_color(bg_color)
                 .disable_rounded_corners());
  }

  if (ranking.has_value() && ranking.value() <= 3) {
    std::string ranking_label = std::format("#{}", ranking.value());

    imm::div(context, mk(card.ent(), 2),
             ComponentConfig{}
                 .with_size(ComponentSize{percent(1.f), percent(0.3f, 0.4f)})
                 .with_label(ranking_label)
                 .with_font(get_font_name(FontID::EQPro), 120.f)
                 .with_color_usage(Theme::Usage::Custom)
                 .with_custom_color(bg_color)
                 .disable_rounded_corners()
                 .with_debug_name("player_card_ranking"));
  }

  if (show_next_color_icon) {
    raylib::Texture2D sheet = EntityHelper::get_singleton_cmp<
                                  afterhours::texture_manager::HasSpritesheet>()
                                  ->texture;
    auto src = afterhours::texture_manager::idx_to_sprite_frame(0, 6);

    auto icon_cell = imm::div(
        context, mk(header_row.ent()),
        ComponentConfig{}
            .with_size(ComponentSize{percent(header_icon_width), percent(1.f)})
            .with_padding(Padding{.top = percent(0.02f),
                                  .left = percent(0.02f),
                                  .bottom = percent(0.02f),
                                  .right = percent(0.02f)})
            .with_debug_name("next_color_cell"));

    if (imm::image_button(
            context, mk(icon_cell.ent()), sheet, src,
            ComponentConfig{}
                .with_size(ComponentSize{percent(1.f), percent(1.f)})
                .with_debug_name("next_color_icon")))
      on_next_color();
  }

  if (show_remove_icon) {
    auto &trash_tex = TextureLibrary::get().get("trashcan");
    raylib::Rectangle src{0.f, 0.f, static_cast<float>(trash_tex.width),
                          static_cast<float>(trash_tex.height)};

    auto icon_cell = imm::div(
        context, mk(header_row.ent()),
        ComponentConfig{}
            .with_size(ComponentSize{percent(header_icon_width), percent(1.f)})
            .with_padding(Padding{.top = percent(0.02f),
                                  .left = percent(0.02f),
                                  .bottom = percent(0.02f),
                                  .right = percent(0.02f)})
            .with_debug_name("remove_ai_cell"));

    if (imm::image_button(
            context, mk(icon_cell.ent()), trash_tex, src,
            ComponentConfig{}
                .with_size(ComponentSize{percent(1.f), percent(1.f)})
                .with_debug_name("remove_ai_icon"))) {
      on_remove();
    }
  }

  if (is_ai && ai_difficulty.has_value() && on_difficulty_change) {
    auto difficulty_options =
        std::vector<std::string>{"Easy", "Medium", "Hard", "Expert"};
    auto current_difficulty = static_cast<size_t>(ai_difficulty.value());

    if (auto result = imm::navigation_bar(
            context, mk(card.ent()), difficulty_options, current_difficulty,
            ComponentConfig{}
                .with_size(ComponentSize{percent(1.f), pixels(50.f)})
                .disable_rounded_corners()
                .with_debug_name("ai_difficulty_navigation_bar"))) {
      on_difficulty_change(
          static_cast<AIDifficulty::Difficulty>(current_difficulty));
    }
  }

  if (show_add_ai && on_add_ai) {
    auto &dollar_tex = TextureLibrary::get().get("dollar_sign");
    raylib::Rectangle src{0.f, 0.f, static_cast<float>(dollar_tex.width),
                          static_cast<float>(dollar_tex.height)};

    if (imm::image_button(
            context, mk(card.ent()), dollar_tex, src,
            ComponentConfig{}
                .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
                .with_padding(Padding{.top = percent(0.25f)})
                .disable_rounded_corners()
                .with_debug_name("add_ai_button"))) {
      on_add_ai();
    }
  }

  return {true, card.ent()};
}

// Reusable styled button component
ElementResult create_styled_button(UIContext<InputAction> &context,
                                   Entity &parent, const std::string &label,
                                   std::function<void()> on_click, int index) {

  if (imm::button(context, mk(parent, index),
                  ComponentConfig{}
                      .with_padding(Padding{.top = pixels(5.f),
                                            .left = pixels(0.f),
                                            .bottom = pixels(5.f),
                                            .right = pixels(0.f)})
                      .with_label(label))) {
    on_click();
    return {true, parent};
  }

  return {false, parent};
}

// Reusable volume slider component
ElementResult create_volume_slider(UIContext<InputAction> &context,
                                   Entity &parent, const std::string &label,
                                   float &volume,
                                   std::function<void(float)> on_change,
                                   int index) {

  auto volume_label = fmt::format("{}\n {:2.0f}", label, volume * 100.f);

  if (auto result =
          slider(context, mk(parent, index), volume,
                 ComponentConfig{}.with_label(std::move(volume_label)))) {
    volume = result.as<float>();
    on_change(volume);
    return {true, parent};
  }

  return {false, parent};
}

// Reusable screen container component
ElementResult create_screen_container(UIContext<InputAction> &context,
                                      Entity &parent,
                                      const std::string &debug_name) {

  return imm::div(
      context, mk(parent),
      ComponentConfig{}
          .with_font(get_font_name(FontID::EQPro), 75.f)
          .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
          .with_absolute_position()
          .with_debug_name(debug_name));
}

// Reusable control group component
ElementResult create_control_group(UIContext<InputAction> &context,
                                   Entity &parent,
                                   const std::string &debug_name) {

  return imm::div(
      context, mk(parent),
      ComponentConfig{}
          .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
          .with_padding(Padding{.top = screen_pct(0.4f),
                                .left = screen_pct(0.4f),
                                .bottom = pixels(0.f),
                                .right = pixels(0.f)})
          .with_absolute_position()
          .with_debug_name(debug_name));
}

ElementResult create_top_left_container(UIContext<InputAction> &context,
                                        Entity &parent,
                                        const std::string &debug_name,
                                        int index) {

  return imm::div(
      context, mk(parent, index),
      ComponentConfig{}
          .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
          .with_padding(Padding{.top = screen_pct(0.02f),
                                .left = screen_pct(0.02f),
                                .bottom = pixels(0.f),
                                .right = pixels(0.f)})
          .with_absolute_position()
          .with_debug_name(debug_name));
}

// Animation helper function
void apply_slide_mods(afterhours::Entity &ent, float slide_v) {
  if (!ent.has<afterhours::ui::UIComponent>())
    return;
  auto &mods = ent.addComponentIfMissing<afterhours::ui::HasUIModifiers>();
  auto rect_now = ent.get<afterhours::ui::UIComponent>().rect();
  float off_left = -(rect_now.x + rect_now.width + 20.0f);
  float tx = (1.0f - std::min(slide_v, 1.0f)) * off_left;
  mods.translate_x = tx;
  mods.translate_y = 0.0f;
  ent.addComponentIfMissing<afterhours::ui::HasOpacity>().value =
      std::clamp(slide_v, 0.0f, 1.0f);
}

} // namespace ui_reusable_components
