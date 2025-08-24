#pragma once

#include "../std_include.h"
#include "../components.h"
#include "../input_mapping.h"
#include "../rl.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/ui.h>

using namespace afterhours;

namespace ui_reusable_components {

// Reusable player card component
afterhours::ui::imm::ElementResult create_player_card(
    afterhours::ui::UIContext<InputAction> &context, afterhours::Entity &parent, const std::string &label,
    raylib::Color bg_color, bool is_ai = false,
    std::optional<int> ranking = std::nullopt,
    std::optional<std::string> stats_text = std::nullopt,
    std::function<void()> on_next_color = nullptr,
    std::function<void()> on_remove = nullptr, bool show_add_ai = false,
    std::function<void()> on_add_ai = nullptr,
    std::optional<AIDifficulty::Difficulty> ai_difficulty = std::nullopt,
    std::function<void(AIDifficulty::Difficulty)> on_difficulty_change =
        nullptr);

// Reusable styled button component
afterhours::ui::imm::ElementResult create_styled_button(afterhours::ui::UIContext<InputAction> &context,
                                   afterhours::Entity &parent, const std::string &label,
                                   std::function<void()> on_click,
                                   int index = 0);

// Reusable volume slider component
afterhours::ui::imm::ElementResult create_volume_slider(afterhours::ui::UIContext<InputAction> &context,
                                   afterhours::Entity &parent, const std::string &label,
                                   float &volume,
                                   std::function<void(float)> on_change,
                                   int index = 0);

// Reusable screen container component
afterhours::ui::imm::ElementResult create_screen_container(afterhours::ui::UIContext<InputAction> &context,
                                      afterhours::Entity &parent,
                                      const std::string &debug_name);

// Reusable control group component
afterhours::ui::imm::ElementResult create_control_group(afterhours::ui::UIContext<InputAction> &context,
                                   afterhours::Entity &parent,
                                   const std::string &debug_name);

afterhours::ui::imm::ElementResult create_top_left_container(afterhours::ui::UIContext<InputAction> &context,
                                        afterhours::Entity &parent,
                                        const std::string &debug_name,
                                        int index);

// Animation helper function
void apply_slide_mods(afterhours::Entity &ent, float slide_v);

} // namespace ui_reusable_components
