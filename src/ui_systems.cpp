#include "ui_systems.h"
#include "car_affectors.h"
#include "components.h"
#include "components_weapons.h"
#include "game_state_manager.h"
#include "input_mapping.h"
#include "intro.h"
#include "log.h"
#include "makers.h"
#include "map_system.h"
#include "navigation.h"
#include "preload.h" // FontID
#include "texture_library.h"
#include "ui/animation_key.h"
#include "math_util.h"
#include "max_health.h"
#include "music_library.h"
#include "query.h"
#include "resources.h"
#include "rl.h"
#include "round_settings.h"
#include "settings.h"
#include "shader_library.h"
#include "sound_library.h"
#include "sound_systems.h"
#include "systems.h"
#include "systems_ai.h"
#include "systems_roundtypes.h"
#include "ui/animation_button_wiggle.h"
#include "ui/animation_key.h"
#include "ui/animation_slide_in.h"
#include "utils.h"
#include "weapons.h"
#include <fmt/format.h>
#include <afterhours/src/plugins/animation.h>
#include <afterhours/src/plugins/ui/immediate/imm_components.h>

using namespace afterhours;

#if 1
static inline void apply_slide_mods(afterhours::Entity &ent, float slide_v) {
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
#endif

// Reusable UI component functions
namespace ui_helpers {

// Reusable player card component
ElementResult create_player_card(
    UIContext<InputAction> &context, Entity &parent, const std::string &label,
    raylib::Color bg_color, bool is_ai = false,
    std::optional<int> ranking = std::nullopt,
    std::optional<std::string> stats_text = std::nullopt,
    std::function<void()> on_next_color = nullptr,
    std::function<void()> on_remove = nullptr, bool show_add_ai = false,
    std::function<void()> on_add_ai = nullptr,
    std::optional<AIDifficulty::Difficulty> ai_difficulty = std::nullopt,
    std::function<void(AIDifficulty::Difficulty)> on_difficulty_change =
        nullptr) {

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

  // Stats text (if provided)
  if (stats_text.has_value()) {
    imm::div(context, mk(card.ent(), 1),
             ComponentConfig{}
                 .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
                 .with_label(stats_text.value())
                 .with_color_usage(Theme::Usage::Custom)
                 .with_custom_color(bg_color)
                 .disable_rounded_corners());
  }

  // Ranking (for top 3)
  if (ranking.has_value() && ranking.value() <= 3) {
    std::string rank_emoji;
    switch (ranking.value()) {
    case 1:
      rank_emoji = "🥇";
      break;
    case 2:
      rank_emoji = "🥈";
      break;
    case 3:
      rank_emoji = "🥉";
      break;
    default:
      rank_emoji = "🏅";
      break;
    }

    imm::div(context, mk(card.ent(), 2),
             ComponentConfig{}
                 .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
                 .with_label(rank_emoji)
                 .with_color_usage(Theme::Usage::Custom)
                 .with_custom_color(bg_color)
                 .disable_rounded_corners());
  }

  // Action buttons row
  if (show_next_color_icon || show_remove_icon || show_add_ai) {
    auto action_row =
        imm::div(context, mk(card.ent(), 3),
                 ComponentConfig{}
                     .with_size(ComponentSize{percent(1.f), percent(0.2f, 0.4f)})
                     .with_flex_direction(FlexDirection::Row)
                     .with_debug_name("player_card_actions"));

    // Next color button
    if (show_next_color_icon) {
      auto next_color_btn = imm::button(
          context, mk(action_row.ent()),
          ComponentConfig{}
              .with_size(ComponentSize{percent(header_icon_width), percent(1.f)})
              .with_label("🎨")
              .with_color_usage(Theme::Usage::Primary)
              .with_debug_name("next_color_btn"));

      if (next_color_btn.as<bool>()) {
        on_next_color();
      }
    }

    // Remove AI button
    if (show_remove_icon) {
      auto remove_btn = imm::button(
          context, mk(action_row.ent()),
          ComponentConfig{}
              .with_size(ComponentSize{percent(header_icon_width), percent(1.f)})
              .with_label("🗑️")
              .with_color_usage(Theme::Usage::Error)
              .with_debug_name("remove_ai_btn"));

      if (remove_btn.as<bool>()) {
        on_remove();
      }
    }

    // Add AI button
    if (show_add_ai) {
      auto add_ai_btn = imm::button(
          context, mk(action_row.ent()),
          ComponentConfig{}
              .with_size(ComponentSize{percent(header_icon_width), percent(1.f)})
              .with_label("🤖")
              .with_color_usage(Theme::Usage::Secondary)
              .with_debug_name("add_ai_btn"));

      if (add_ai_btn.as<bool>()) {
        on_add_ai();
      }
    }

    // AI difficulty selector (if applicable)
    if (ai_difficulty.has_value() && on_difficulty_change) {
      auto difficulty_btn = imm::button(
          context, mk(action_row.ent()),
          ComponentConfig{}
              .with_size(ComponentSize{percent(header_icon_width), percent(1.f)})
              .with_label("⚙️")
              .with_color_usage(Theme::Usage::Secondary)
              .with_debug_name("difficulty_btn"));

      if (difficulty_btn.as<bool>()) {
        // Cycle through difficulties
        AIDifficulty::Difficulty next_diff;
        switch (ai_difficulty.value()) {
        case AIDifficulty::Difficulty::Easy:
          next_diff = AIDifficulty::Difficulty::Medium;
          break;
        case AIDifficulty::Difficulty::Medium:
          next_diff = AIDifficulty::Difficulty::Hard;
          break;
        case AIDifficulty::Difficulty::Hard:
          next_diff = AIDifficulty::Difficulty::Easy;
          break;
        default:
          next_diff = AIDifficulty::Difficulty::Medium;
          break;
        }
        on_difficulty_change(next_diff);
      }
    }
  }

  return card;
}

ElementResult create_top_left_container(UIContext<InputAction> &context, Entity &parent, const std::string &debug_name, int index) {
  return imm::div(context, mk(parent, index),
                  ComponentConfig{}
                      .with_size(ComponentSize{screen_pct(0.2f), screen_pct(0.3f)})
                      .with_padding(Padding{.top = screen_pct(0.02f), .left = screen_pct(0.02f)})
                      .with_flex_direction(FlexDirection::Column)
                      .with_debug_name(debug_name));
}

ElementResult create_screen_container(UIContext<InputAction> &context, Entity &entity, const std::string &debug_name) {
  return imm::div(context, mk(entity),
                  ComponentConfig{}
                      .with_font(get_font_name(FontID::EQPro), 75.f)
                      .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                      .with_absolute_position()
                      .with_debug_name(debug_name));
}

ElementResult create_control_group(UIContext<InputAction> &context, Entity &parent, const std::string &debug_name) {
  return imm::div(context, mk(parent),
                  ComponentConfig{}
                      .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                      .with_padding(Padding{.top = screen_pct(0.4f), .left = screen_pct(0.4f), .bottom = pixels(0.f), .right = pixels(0.f)})
                      .with_absolute_position()
                      .with_debug_name(debug_name));
}

ElementResult create_styled_button(UIContext<InputAction> &context, Entity &parent, const std::string &label, std::function<void()> on_click, int index) {
  auto btn = imm::button(context, mk(parent, index),
                         ComponentConfig{}
                             .with_label(label)
                             .with_size(ComponentSize{screen_pct(200.f / 1280.f), screen_pct(50.f / 720.f)})
                             .with_margin(Margin{.top = screen_pct(0.01f)}));
  
  if (btn.as<bool>()) {
    on_click();
  }
  
  return btn;
}

ElementResult create_volume_slider(UIContext<InputAction> &context, Entity &parent, const std::string &label, float volume, std::function<void(float)> on_change, int index) {
  if (auto result = slider(context, mk(parent, index), volume,
                          ComponentConfig{}
                              .with_size(ComponentSize{screen_pct(200.f / 1280.f), screen_pct(50.f / 720.f)})
                              .with_label(label)
                              .with_margin(Margin{.top = screen_pct(0.01f)}))) {
    on_change(result.as<float>());
  }
  
  return ElementResult{parent};
}

ElementResult slider(UIContext<InputAction> &context, Entity &parent, float value, ComponentConfig config) {
  // Simple slider implementation - this should be replaced with proper slider component
  auto btn = imm::button(context, mk(parent),
                         config.with_label(fmt::format("{:.2f}", value)));
  
  if (btn.as<bool>()) {
    // Increment value on click for now
    value = std::min(1.0f, value + 0.1f);
  }
  
  return ElementResult{parent};
}

} // namespace ui_helpers

using Screen = GameStateManager::Screen;

constexpr static vec2 button_size = vec2{100, 50};

Padding button_group_padding = Padding{
    .top = screen_pct(0.4f),
    .left = screen_pct(0.4f),
    .bottom = pixels(0.f),
    .right = pixels(0.f),
};

Padding control_group_padding = Padding{
    .top = screen_pct(0.4f),
    .left = screen_pct(0.4f),
    .bottom = pixels(0.f),
    .right = pixels(0.f),
};

Padding button_padding = Padding{
    .top = pixels(button_size.y / 10.f),
    .left = pixels(0.f),
    .bottom = pixels(button_size.y / 10.f),
    .right = pixels(0.f),
};
