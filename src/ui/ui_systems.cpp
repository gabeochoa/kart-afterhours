

#include <afterhours/ah.h>
#include <fmt/format.h>

#include <afterhours/src/logging.h>
#include <afterhours/src/graphics.h>

//

#include "../components.h"
#include "../config.h"
#include "../e2e_integration.h"
#include "../game.h"
#include "../game_state_manager.h"
#include "../map_system.h"
#include "../preload.h" // FontID
#include "../query.h"
#include "../round_settings.h"
#include "../settings.h"
#include "../strings.h"
#include "../library/texture_library.h"
#include "../translation_manager.h"
#include "animation_control.h"
#include "animation_key.h"
#include "animation_slide_in.h"
#include "animation_ui_wiggle.h"
#include "navigation.h"

using namespace afterhours;

struct MapConfig;

using namespace afterhours::ui;
using namespace afterhours::ui::imm;
using Screen = GameStateManager::Screen;

struct SetupGameStylingDefaults
    : System<afterhours::ui::UIContext<InputAction>> {

  virtual void once(float) override {
    auto &styling_defaults = afterhours::ui::imm::UIStylingDefaults::get();

    // Memphis '93. Deep purple ground, pastel-bright accents, shapes as often
    // outlined as filled. See docs/ui-mock.html.
    styling_defaults
        .set_theme_color(afterhours::ui::Theme::Usage::Primary,
                         afterhours::Color{224, 107, 221, 255}) // orchid
        .set_theme_color(afterhours::ui::Theme::Usage::Secondary,
                         afterhours::Color{91, 168, 240, 255}) // sky
        .set_theme_color(afterhours::ui::Theme::Usage::Accent,
                         afterhours::Color{240, 232, 92, 255}) // butter
        .set_theme_color(afterhours::ui::Theme::Usage::Background,
                         afterhours::Color{46, 27, 105, 255}) // deep purple
        .set_theme_color(afterhours::ui::Theme::Usage::Font,
                         afterhours::Color{255, 255, 255, 255})
        .set_theme_color(afterhours::ui::Theme::Usage::DarkFont,
                         afterhours::Color{18, 10, 43, 255}); // ink

    // Set the default font for all components based on current language
    styling_defaults.set_default_font(
        get_font_name(translation_manager::get_font_for_language()), 16.f);

    // Enable grid snapping for consistent 8pt grid spacing
    styling_defaults.set_grid_snapping(true);

    // Enable TV safe area validation
    styling_defaults.enable_tv_safe_validation();

    // Component-specific styling
    styling_defaults.set_component_config(
        ComponentType::Button,
        ComponentConfig{}
            .with_size(ComponentSize{w1280(200.f), h720(50.f)})
            .with_background(Theme::Usage::Primary));

    styling_defaults.set_component_config(
        ComponentType::Slider,
        ComponentConfig{}
            .with_size(ComponentSize{w1280(200.f), h720(50.f)})
            .with_background(Theme::Usage::Secondary));

    styling_defaults.set_component_config(
        ComponentType::Checkbox,
        ComponentConfig{}
            .with_size(ComponentSize{w1280(200.f), h720(50.f)})
            .with_background(Theme::Usage::Primary));

    styling_defaults.set_component_config(
        ComponentType::CheckboxNoLabel,
        ComponentConfig{}
            .with_size(ComponentSize{w1280(200.f), h720(50.f)})
            .with_background(Theme::Usage::Primary));

    styling_defaults.set_component_config(
        ComponentType::Dropdown,
        ComponentConfig{}
            .with_size(ComponentSize{w1280(200.f), h720(50.f)})
            .with_background(Theme::Usage::Primary));

    styling_defaults.set_component_config(
        ComponentType::NavigationBar,
        ComponentConfig{}
            .with_size(ComponentSize{w1280(200.f), h720(50.f)})
            .with_background(Theme::Usage::Primary));
  }
};

struct ScheduleDebugUI : System<afterhours::ui::UIContext<InputAction>> {
  bool enabled = false;
  float enableCooldown = 0.f;
  float enableCooldownReset = 0.2f;

  virtual bool should_run(float dt) override;
  virtual void for_each_with(Entity &entity,
                             afterhours::ui::UIContext<InputAction> &context,
                             float) override;
};

struct SchedulePauseUI : System<afterhours::ui::UIContext<InputAction>> {
  input::PossibleInputCollector inpc;

  void exit_game() {
    if (!e2e_integration::is_enabled())
      running = false;
  }

  virtual bool should_run(float) override;
  virtual void for_each_with(Entity &entity,
                             afterhours::ui::UIContext<InputAction> &context,
                             float) override;
};

struct ScheduleMainMenuUI : System<afterhours::ui::UIContext<InputAction>> {

  Screen get_active_screen() { return GameStateManager::get().active_screen; }

  void set_active_screen(Screen screen) {
    GameStateManager::get().set_screen(screen);
  }

  // settings cache stuff for now
  window_manager::ProvidesAvailableWindowResolutions *resolution_provider{
      nullptr}; // non owning ptr
                // eventually std::observer_ptr?
  window_manager::ProvidesCurrentResolution *current_resolution_provider{
      nullptr}; // non owning ptr
                // eventually std::observer_ptr?
  std::vector<std::string> resolution_strs;
  size_t resolution_index{0};

  // character creators
  std::vector<RefEntity> players;
  std::vector<RefEntity> ais;
  input::PossibleInputCollector inpc;

  void update_resolution_cache();
  void round_end_player_column(Entity &parent, UIContext<InputAction> &context,
                               const size_t index,
                               const std::vector<OptEntity> &round_players,
                               const std::vector<OptEntity> &round_ais,
                               std::optional<int> ranking = std::nullopt);
  std::map<EntityID, int>
  get_tag_and_go_rankings(const std::vector<OptEntity> &round_players,
                          const std::vector<OptEntity> &round_ais);
  void render_lives_stats(UIContext<InputAction> &context, Entity &parent,
                          const OptEntity &car, raylib::Color bg_color);
  void render_kills_stats(UIContext<InputAction> &context, Entity &parent,
                          const OptEntity &car, raylib::Color bg_color);
  void render_score_stats(UIContext<InputAction> &context, Entity &parent,
                          const OptEntity &car, raylib::Color bg_color);
  void render_hippo_stats(UIContext<InputAction> &context, Entity &parent,
                          const OptEntity &car, raylib::Color bg_color);
  void render_tag_and_go_stats(UIContext<InputAction> &context, Entity &parent,
                               const OptEntity &car, raylib::Color bg_color);
  void render_unknown_stats(UIContext<InputAction> &context, Entity &parent,
                            const OptEntity &car, raylib::Color bg_color);
  void render_team_results(UIContext<InputAction> &context, Entity &parent,
                           const std::vector<OptEntity> &round_players,
                           const std::vector<OptEntity> &round_ais);
  void render_team_column_results(UIContext<InputAction> &context,
                                  Entity &parent, const std::string &team_name,
                                  int team_id,
                                  const std::vector<OptEntity> &team_players,
                                  int team_score);

  Screen character_creation(Entity &entity, UIContext<InputAction> &context);
  Screen map_selection(Entity &entity, UIContext<InputAction> &context);
  Screen round_settings(Entity &entity, UIContext<InputAction> &context);
  Screen main_screen(Entity &entity, UIContext<InputAction> &context);
  Screen settings_screen(Entity &entity, UIContext<InputAction> &context);
  Screen about_screen(Entity &entity, UIContext<InputAction> &context);
  Screen round_end_screen(Entity &entity, UIContext<InputAction> &context);

  void exit_game() {
    if (!e2e_integration::is_enabled())
      running = false;
  }

  virtual void once(float) override;
  virtual bool should_run(float) override;
  virtual void for_each_with(Entity &entity, UIContext<InputAction> &context,
                             float) override;
};

namespace ui_helpers {

struct PlayerCardData {
  Entity &parent;
  int index;
  const std::string &label;
  raylib::Color bg_color;
  bool is_ai = false;

  std::optional<int> ranking = std::nullopt;
  std::optional<std::string> stats_text = std::nullopt;

  std::function<void()> on_next_color = nullptr;
  std::function<void()> on_remove = nullptr;
  std::function<void()> on_add_ai = nullptr;
  std::function<void()> on_team_switch = nullptr;
  std::optional<AIDifficulty::Difficulty> ai_difficulty = std::nullopt;
  std::function<void(AIDifficulty::Difficulty)> on_difficulty_change = nullptr;
};

ElementResult player_card_cell(UIContext<InputAction> &context, Entity &parent,
                               const std::string &debug_name,
                               float width_percent = 0.125f) {
  return imm::div(
      context, mk(parent, std::hash<std::string>{}(debug_name + "_cell")),
      ComponentConfig{}
          .with_size(ComponentSize{percent(width_percent), percent(1.f)})
          .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                .left = imm::DefaultSpacing::tiny(),
                                .bottom = imm::DefaultSpacing::tiny(),
                                .right = imm::DefaultSpacing::tiny()})
          .with_debug_name(debug_name + "_cell"));
}

void maybe_button(UIContext<InputAction> &context, Entity &parent,
                  const std::string &label, const std::string &debug_name,
                  std::function<void()> action = nullptr,
                  float width_percent = 0.125f) {

  if (!action) {
    return;
  }

  auto button_cell =
      player_card_cell(context, parent, debug_name, width_percent);

  if (imm::button(context,
                  mk(button_cell.ent(),
                     std::hash<std::string>{}(debug_name + "_button")),
                  ComponentConfig{}
                      .with_size(ComponentSize{percent(1.f), percent(1.f)})
                      .with_label(label)
                      .with_debug_name(debug_name + "_button"))) {
    action();
  }
}

void maybe_image_button(UIContext<InputAction> &context, Entity &parent,
                        const std::string &debug_name, raylib::Texture2D sheet,
                        Rectangle spriteSheetSrc,
                        std::function<void()> action = nullptr,
                        float width_percent = 0.125f) {

  if (!action) {
    return;
  }
  auto button_cell =
      player_card_cell(context, parent, debug_name, width_percent);

  if (imm::image_button(
          context,
          mk(button_cell.ent(),
             std::hash<std::string>{}(debug_name + "_button")),
          sheet, spriteSheetSrc,
          ComponentConfig{}
              .with_size(ComponentSize{percent(1.f), percent(1.f)})
              .with_debug_name(debug_name))) {
    action();
  }
}

void maybe_difficulty_button(UIContext<InputAction> &context, Entity &parent,
                             PlayerCardData &data) {
  if (data.ai_difficulty.has_value() && data.on_difficulty_change) {
    auto difficulty_options = std::vector<std::string>{
        translation_manager::make_translatable_string(strings::i18n::easy)
            .get_text(),
        translation_manager::make_translatable_string(strings::i18n::medium)
            .get_text(),
        translation_manager::make_translatable_string(strings::i18n::hard)
            .get_text(),
        translation_manager::make_translatable_string(strings::i18n::expert)
            .get_text()};
    auto current_difficulty = static_cast<size_t>(data.ai_difficulty.value());

    auto difficulty_cell =
        player_card_cell(context, parent, "difficulty_cell", 0.7f);

    if (auto result = imm::navigation_bar(
            context, mk(difficulty_cell.ent()), difficulty_options,
            current_difficulty,
            ComponentConfig{}
                .with_size(ComponentSize{percent(1.f), percent(1.f)})
                .disable_rounded_corners()
                .with_debug_name("ai_difficulty_navigation_bar"))) {
      data.on_difficulty_change(
          static_cast<AIDifficulty::Difficulty>(current_difficulty));
    }
  }
}

void maybe_next_color_button(UIContext<InputAction> &context, Entity &parent,
                             PlayerCardData &data) {
  raylib::Texture2D sheet = EntityHelper::get_singleton_cmp<
                                afterhours::texture_manager::HasSpritesheet>()
                                ->texture;

  maybe_image_button(context, parent, "next_color", sheet,
                     afterhours::texture_manager::idx_to_sprite_frame(0, 6),
                     data.on_next_color);
}

void maybe_ai_buttons(UIContext<InputAction> &context, Entity &parent,
                      PlayerCardData &data) {

  auto bottom_row = imm::hstack(
      context, mk(parent),
      ComponentConfig{}
          .with_size(ComponentSize{percent(1.f, 1.f), percent(0.4f, 1.f)})
          .with_debug_name("player_card_bottom_row"));
  if (data.is_ai) {
    maybe_difficulty_button(context, bottom_row.ent(), data);

    if (data.on_difficulty_change) {
      imm::spacer(
          context, mk(bottom_row.ent()),
          ComponentConfig{}
              .with_size(ComponentSize{percent(0.15f, 0.1f), percent(1.f)}));
    }

    auto &trash_tex = TextureLibrary::get().get("trashcan");
    raylib::Rectangle src{0.f, 0.f, static_cast<float>(trash_tex.width),
                          static_cast<float>(trash_tex.height)};
    maybe_image_button(context, bottom_row.ent(), "delete", trash_tex, src,
                       data.on_remove);
  }

  auto &dollar_tex = TextureLibrary::get().get("dollar_sign");
  raylib::Rectangle src{0.f, 0.f, static_cast<float>(dollar_tex.width),
                        static_cast<float>(dollar_tex.height)};
  maybe_image_button(context, bottom_row.ent(), "add_ai", dollar_tex, src,
                     data.on_add_ai, 1.f);
}

// Reusable player card component
ElementResult create_player_card(UIContext<InputAction> &context,
                                 Entity &parent, PlayerCardData &data) {

  auto card = imm::div(context, mk(parent),
                       ComponentConfig{}
                           .with_size(ComponentSize{percent(1.f), percent(1.f)})
                           .with_custom_background(data.bg_color)
                           .disable_rounded_corners());

  // Top row: ID [color] [team switch]
  auto top_row =
      imm::hstack(context, mk(card.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(1.0f), percent(0.4f)})
                   .with_margin(Margin{.top = imm::DefaultSpacing::tiny(),
                                       .left = imm::DefaultSpacing::tiny(),
                                       .bottom = imm::DefaultSpacing::tiny(),
                                       .right = imm::DefaultSpacing::tiny()})
                   .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                         .left = imm::DefaultSpacing::tiny(),
                                         .bottom = imm::DefaultSpacing::tiny(),
                                         .right = imm::DefaultSpacing::tiny()})
                   .with_debug_name("player_card_top_row"));

  // Player ID label
  imm::div(context, mk(top_row.ent()),
           ComponentConfig{}
               .with_size(ComponentSize{percent(0.2f), percent(1.f)})
               .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                     .left = imm::DefaultSpacing::tiny(),
                                     .bottom = imm::DefaultSpacing::tiny(),
                                     .right = imm::DefaultSpacing::tiny()})
               .with_label(data.label)
               .with_custom_background(data.bg_color)
               .disable_rounded_corners()
               .with_debug_name("player_id_label"));

  maybe_next_color_button(context, top_row.ent(), data);
  maybe_button(context, top_row.ent(), "<->", "team_switch",
               data.on_team_switch);
  maybe_ai_buttons(context, card.ent(), data);

  return {true, card.ent()};
}

// Reusable styled button component
ElementResult create_styled_button(UIContext<InputAction> &context,
                                   Entity &parent, const std::string &label,
                                   std::function<void()> on_click,
                                   int index = 0,
                                   const std::string &debug_name = "") {

  auto config = ComponentConfig{}
      .with_label(label)
      .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                            .left = imm::DefaultSpacing::tiny(),
                            .bottom = imm::DefaultSpacing::tiny(),
                            .right = imm::DefaultSpacing::tiny()});

  // A stable debug name lets e2e target the button by id instead of by its
  // visible text, so renaming copy stops breaking the suite.
  if (!debug_name.empty())
    config = config.with_debug_name(debug_name);

  animation_control::apply_slide_in(config);

  if (imm::button(context, mk(parent, index), config)) {
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
                                   int index = 0) {

  if (auto result = slider(context, mk(parent, index), volume,
                           ComponentConfig{}
                               .with_size(ComponentSize{pixels(400.f), pixels(40.f)})
                               .with_label(label)
                               .with_padding(
                                   Padding{.top = spacing_to_size(Spacing::xs),
                                           .left = pixels(0.f),
                                           .bottom = spacing_to_size(Spacing::xs),
                                           .right = pixels(0.f)}),
                           SliderHandleValueLabelPosition::OnHandle)) {
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
          .with_padding(Padding{.top = imm::DefaultSpacing::large(),
                                .left = imm::DefaultSpacing::large(),
                                .bottom = imm::DefaultSpacing::large(),
                                .right = imm::DefaultSpacing::large()})
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
          .with_padding(Padding{.top = imm::DefaultSpacing::medium(),
                                .left = imm::DefaultSpacing::medium(),
                                .bottom = pixels(0.f),
                                .right = pixels(0.f)})
          .with_absolute_position()
          .with_debug_name(debug_name));
}

} // namespace ui_helpers

using Screen = GameStateManager::Screen;

Padding button_group_padding = Padding{
    .top = imm::DefaultSpacing::large(),
    .left = imm::DefaultSpacing::large(),
    .bottom = imm::DefaultSpacing::large(),
    .right = imm::DefaultSpacing::large(),
};

Padding control_group_padding = Padding{
    .top = imm::DefaultSpacing::large(),
    .left = imm::DefaultSpacing::large(),
    .bottom = imm::DefaultSpacing::large(),
    .right = imm::DefaultSpacing::large(),
};

Padding button_padding = Padding{
    .top = imm::DefaultSpacing::tiny(),
    .left = imm::DefaultSpacing::tiny(),
    .bottom = imm::DefaultSpacing::tiny(),
    .right = imm::DefaultSpacing::tiny(),
};

void ScheduleMainMenuUI::update_resolution_cache() {
  if (afterhours::graphics::is_headless()) {
    if (resolution_strs.empty()) {
      resolution_strs.push_back(fmt::format("{}x{}", Settings::get_screen_width(), Settings::get_screen_height()));
      resolution_index = 0;
    }
    return;
  }

  resolution_provider = EntityHelper::get_singleton_cmp<
      window_manager::ProvidesAvailableWindowResolutions>();

  resolution_strs.clear();

  std::vector<std::string> temp;
  std::ranges::transform(resolution_provider->fetch_data(),
                         std::back_inserter(temp),
                         [](const auto &rez) { return std::string(rez); });
  resolution_strs = std::move(temp);
  resolution_index = resolution_provider->current_index();
}

void ScheduleMainMenuUI::once(float) {

  current_resolution_provider = EntityHelper::get_singleton_cmp<
      window_manager::ProvidesCurrentResolution>();

  if (GameStateManager::get().active_screen ==
      GameStateManager::Screen::Settings) {
    update_resolution_cache();
  }

  // character creator

  {
    players = EQ().whereHasComponent<PlayerID>().orderByPlayerID().gen();
    ais = EQ().whereHasComponent<AIControlled>().gen();
    inpc = input::get_input_collector();
  }
}

bool ScheduleMainMenuUI::should_run(float) {
  // Visibility managed by NavigationSystem; render if menu active and UI
  // visible
  auto *nav = EntityHelper::get_singleton_cmp<MenuNavigationStack>();
  return GameStateManager::get().is_menu_active() &&
         (nav ? nav->ui_visible : true);
}

// ----------------------------------------------------------------------------
// Character select ("WHO'S DRIVING") -- see docs/ui-mock.html section 02.
//
// Eight slots are always on screen in a 4-wide grid. A filled slot shows the
// kart the player will actually drive, tinted to their paint, plus the device
// they're on, the eight-colour palette, and (for bots) a difficulty stepper.
// An open slot is a dotted outline you can click to drop a bot in.
//
// Determinism: nothing here reads a clock, rand(), or an unordered container.
// Bot names come from a fixed table indexed by seat, and the device chip falls
// back to KEYBOARD under headless (is_gamepad_available is always false), so
// the screenshot baselines stay byte-stable.
// ----------------------------------------------------------------------------
namespace character_select {

using afterhours::Color;

constexpr Color ink{18, 10, 43, 255};
constexpr Color panel_bg{27, 16, 64, 255};
constexpr Color well_bg{21, 11, 51, 255};
constexpr Color sky{91, 168, 240, 255};
constexpr Color mint{79, 214, 166, 255};
constexpr Color butter{240, 232, 92, 255};
constexpr Color muted{179, 166, 214, 255};
constexpr Color open_edge{92, 74, 148, 255};
constexpr Color open_text{143, 131, 184, 255};
constexpr Color team_a{91, 168, 240, 255};
constexpr Color team_b{255, 164, 60, 255};

constexpr std::array<const char *, input::MAX_GAMEPAD_ID> bot_names = {
    "ROBO-DAVE", "PIXEL-8",   "NEON-NAN", "CHROME-JO",
    "GLITCH-KO", "TURBO-MAX", "VECTOR-VI", "STATIC-SU"};

// One grid position. `car` is null for an open slot.
struct Slot {
  Entity *car{nullptr};
  size_t seat{0}; // player number, or bot number for AI
  bool is_ai{false};
};

// imm::button routes its config through UIStylingDefaults::apply_overrides,
// which only forwards a fixed list of fields -- border, shadow, opacity,
// translate, custom hover background and the custom-draw hooks are all
// silently dropped on the way through. Marking a config internal skips that
// merge; the only thing we then have to put back by hand is the default font,
// since font name/size are the other thing the merge was doing for us.
inline ComponentConfig &keep_visuals(ComponentConfig &config, float font_px) {
  return config
      .with_font(get_font_name(translation_manager::get_font_for_language()),
                 pixels(font_px))
      .with_internal(true);
}

inline Padding card_padding() {
  return Padding{.top = ui::h720(8.f),
                 .left = ui::w1280(10.f),
                 .bottom = ui::h720(8.f),
                 .right = ui::w1280(10.f)};
}

inline std::string driver_name(const Slot &slot) {
  if (slot.is_ai)
    return bot_names[slot.seat % bot_names.size()];
  return fmt::format("PLAYER {}", slot.seat + 1);
}

inline std::string device_chip(const Slot &slot) {
  if (slot.is_ai)
    return "CPU";
  const auto id = slot.car->has<PlayerID>()
                      ? slot.car->get<PlayerID>().id
                      : static_cast<input::GamepadID>(0);
  if (input::is_gamepad_available(id))
    return fmt::format("PAD {}", id + 1);
  return "KEYBOARD";
}

// The kart the player will drive, drawn in their paint. imm::sprite always
// tints white (see docs/afterhours_gaps.md), so the tinted draw goes through
// the custom-draw escape hatch instead.
inline void kart_portrait(UIContext<InputAction> &context, Entity &card,
                          int slot_index, Color tint,
                          const std::function<void()> &on_click) {
  auto *sheet_cmp = EntityHelper::get_singleton_cmp<
      afterhours::texture_manager::HasSpritesheet>();
  const std::string dbg = fmt::format("slot_{}_kart", slot_index);

  auto config = ComponentConfig{}
                    .with_size(ComponentSize{percent(1.f), expand()})
                    .with_custom_background(well_bg)
                    .with_custom_hover_bg(panel_bg)
                    .with_corner_radius(10.f)
                    .with_debug_name(dbg);
  keep_visuals(config, 12.f);

  if (sheet_cmp) {
    const auto sheet = sheet_cmp->texture;
    const auto src = afterhours::texture_manager::idx_to_sprite_frame(0, 1);
    config.with_on_draw_fg([sheet, src, tint](RectangleType r) {
      const float side = std::min(r.width, r.height) * 0.86f;
      const RectangleType dest{r.x + (r.width - side) * 0.5f,
                               r.y + (r.height - side) * 0.5f, side, side};
      raylib::DrawTexturePro(sheet, src, dest, raylib::Vector2{0.f, 0.f}, 0.f,
                             tint);
    });
  }

  if (imm::button(context, mk(card, 1), config))
    on_click();
}

// Eight swatches. The player's own colour gets a white ring; a colour another
// driver already holds is dimmed and inert.
inline void paint_palette(UIContext<InputAction> &context, Entity &card,
                          int slot_index, ManagesAvailableColors &colors,
                          EntityID car_id) {
  const size_t current =
      colors.users.contains(car_id) ? colors.users.at(car_id) : 0;

  auto row = imm::hstack(context, mk(card, 4),
                         ComponentConfig{}
                             .with_size(ComponentSize{percent(1.f), ui::h720(16.f)})
                             .with_gap(ui::w1280(3.f))
                             .with_transparent_bg()
                             .with_no_wrap()
                             .with_debug_name(
                                 fmt::format("slot_{}_paint", slot_index)));

  for (size_t k = 0; k < ManagesAvailableColors::colors.size(); k++) {
    const bool is_current = (k == current);
    const bool taken_by_other = colors.used[k] && !is_current;

    auto swatch =
        ComponentConfig{}
            .with_size(ComponentSize{ui::w1280(15.f), percent(1.f)})
            .with_padding(Padding{})
            .with_custom_background(ManagesAvailableColors::colors[k])
            .with_custom_hover_bg(ManagesAvailableColors::colors[k])
            .with_border(is_current ? afterhours::Color{255, 255, 255, 255}
                                    : ink,
                         2.f)
            .with_corner_radius(4.f)
            .with_opacity(taken_by_other ? 0.28f : 1.f)
            .with_skip_grid_snap()
            // 64 swatches on screen would swamp the tab order; the kart button
            // is the tabbable way to change colour.
            .with_skip_tabbing(true)
            .with_debug_name(
                fmt::format("slot_{}_swatch_{}", slot_index, k));
    keep_visuals(swatch, 10.f);

    if (imm::button(context, mk(row.ent(), static_cast<int>(k)), swatch) &&
        !taken_by_other && !is_current) {
      colors.release_only(car_id);
      colors.used[k] = true;
      colors.users[car_id] = k;
    }
  }
}

inline void difficulty_stepper(UIContext<InputAction> &context, Entity &card,
                               int slot_index, Entity &car) {
  const std::array<std::string, 4> options{
      translation_manager::make_translatable_string(strings::i18n::easy)
          .get_text(),
      translation_manager::make_translatable_string(strings::i18n::medium)
          .get_text(),
      translation_manager::make_translatable_string(strings::i18n::hard)
          .get_text(),
      translation_manager::make_translatable_string(strings::i18n::expert)
          .get_text()};

  auto &difficulty = car.addComponentIfMissing<AIDifficulty>().difficulty;
  size_t index = static_cast<size_t>(difficulty);

  if (imm::stepper(context, mk(card, 5), options, index,
                   ComponentConfig{}
                       .with_size(ComponentSize{percent(1.f), ui::h720(24.f)})
                       .with_custom_background(well_bg)
                       .with_custom_text_color(butter)
                       .with_corner_radius(10.f)
                       .with_font_size(12.f)
                       .with_debug_name(
                           fmt::format("slot_{}_difficulty", slot_index)))) {
    difficulty = static_cast<AIDifficulty::Difficulty>(index);
  }
}

inline void open_card(UIContext<InputAction> &context, Entity &cell,
                      int slot_index) {
  auto config =
      ComponentConfig{}
          .with_size(ComponentSize{percent(1.f), percent(1.f)})
          .with_label("+\nPRESS START")
          .with_custom_background(well_bg)
          .with_custom_hover_bg(panel_bg)
          .with_custom_text_color(open_text)
          .with_border(open_edge, 3.f, afterhours::ui::BorderStyle::Dotted)
          .with_corner_radius(14.f)
          .with_alignment(TextAlignment::Center)
          .with_debug_name(fmt::format("slot_{}_open", slot_index));
  keep_visuals(config, 13.f);

  if (imm::button(context, mk(cell), config))
    make_ai();
}

inline void filled_card(UIContext<InputAction> &context, Entity &cell,
                        int slot_index, const Slot &slot, bool team_mode) {
  Entity &car = *slot.car;
  const Color paint = car.get<HasColor>().color();
  ManagesAvailableColors &colors =
      *EntityHelper::get_singleton_cmp<ManagesAvailableColors>();

  auto card = imm::vstack(
      context, mk(cell),
      ComponentConfig{}
          .with_size(ComponentSize{percent(1.f), percent(1.f)})
          .with_custom_background(panel_bg)
          .with_border(mint, 3.f)
          .with_corner_radius(14.f)
          .with_padding(card_padding())
          .with_debug_name(fmt::format("slot_{}_card", slot_index)));

  // Top strip: team badge on the left, READY stamp on the right.
  auto strip = imm::hstack(
      context, mk(card.ent(), 0),
      ComponentConfig{}
          .with_size(ComponentSize{percent(1.f), ui::h720(16.f)})
          .with_transparent_bg()
          .with_no_wrap()
          .with_debug_name(fmt::format("slot_{}_strip", slot_index)));

  if (team_mode) {
    const int team_id = car.get<TeamID>().team_id;
    auto badge = ComponentConfig{}
                     .with_size(ComponentSize{ui::w1280(66.f), percent(1.f)})
                     .with_padding(Padding{})
                     .with_label(team_id == 0 ? "TEAM A" : "TEAM B")
                     .with_custom_background(team_id == 0 ? team_a : team_b)
                     .with_custom_text_color(ink)
                     .with_corner_radius(8.f)
                     // The team is in the debug name, not just the label:
                     // assert_ui splits args on spaces so "TEAM A" is not a
                     // value an e2e script can match on.
                     .with_debug_name(fmt::format(
                         "slot_{}_team_{}", slot_index, team_id == 0 ? "a"
                                                                     : "b"));
    keep_visuals(badge, 11.f);
    if (imm::button(context, mk(strip.ent(), 0), badge))
      car.get<TeamID>().team_id = (team_id == 0) ? 1 : 0;
  }

  imm::div(context, mk(strip.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{expand(), percent(1.f)})
               .with_transparent_bg()
               .with_skip_tabbing(true)
               .with_debug_name(fmt::format("slot_{}_strip_gap", slot_index)));

  // Every occupied slot is ready -- the game has no per-driver ready gate yet.
  imm::div(context, mk(strip.ent(), 2),
           ComponentConfig{}
               .with_size(ComponentSize{ui::w1280(52.f), percent(1.f)})
               .with_label("READY")
               .with_custom_background(mint)
               .with_custom_text_color(ink)
               .with_corner_radius(8.f)
               .with_font_size(10.f)
               .with_skip_tabbing(true)
               .with_debug_name(fmt::format("slot_{}_ready", slot_index)));

  kart_portrait(context, card.ent(), slot_index, paint,
                [&colors, id = car.id]() { colors.release_and_get_next(id); });

  imm::div(context, mk(card.ent(), 2),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), ui::h720(20.f)})
               .with_label(driver_name(slot))
               .with_transparent_bg()
               .with_custom_text_color(butter)
               .with_font_size(14.f)
               .with_skip_tabbing(true)
               .with_debug_name(fmt::format("slot_{}_name", slot_index)));

  imm::div(context, mk(card.ent(), 3),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), ui::h720(17.f)})
               .with_label(device_chip(slot))
               .with_custom_background(well_bg)
               .with_custom_text_color(muted)
               .with_corner_radius(8.f)
               .with_font_size(11.f)
               .with_skip_tabbing(true)
               .with_debug_name(fmt::format("slot_{}_device", slot_index)));

  paint_palette(context, card.ent(), slot_index, colors, car.id);

  if (slot.is_ai)
    difficulty_stepper(context, card.ent(), slot_index, car);
  else
    imm::div(context, mk(card.ent(), 5),
             ComponentConfig{}
                 .with_size(ComponentSize{percent(1.f), ui::h720(24.f)})
                 .with_transparent_bg()
                 .with_skip_tabbing(true)
                 .with_debug_name(
                     fmt::format("slot_{}_difficulty_gap", slot_index)));

  // The trashcan is gone; removing a bot is the same button that spawns it,
  // held on the card itself so the row doesn't need a second control column.
  if (slot.is_ai) {
    auto remove = ComponentConfig{}
                      .with_size(ComponentSize{percent(1.f), ui::h720(18.f)})
                      .with_padding(Padding{})
                      .with_label("REMOVE")
                      .with_transparent_bg()
                      .with_custom_text_color(muted)
                      .with_debug_name(fmt::format("slot_{}_remove", slot_index));
    keep_visuals(remove, 11.f);
    if (imm::button(context, mk(card.ent(), 6), remove)) {
      colors.release_only(car.id);
      car.cleanup = true;
    }
  }
}

inline void slot_card(UIContext<InputAction> &context, Entity &cell,
                      int slot_index, const Slot &slot, bool team_mode) {
  if (slot.car)
    filled_card(context, cell, slot_index, slot, team_mode);
  else
    open_card(context, cell, slot_index);
}

} // namespace character_select

// ----------------------------------------------------------------------------
// Round rules ("HOW DO WE WIN") -- see docs/ui-mock.html section 03.
//
// Tabs replaced a </> stepper: cycling meant 06_round_settings_modes.e2e left
// active_round_type one step from where it found it on every run, and round
// settings persist to the save file (TODO 14).
//
// Determinism: no clock, no rand(), no unordered iteration. Every label is a
// pure function of RoundManager state.
// ----------------------------------------------------------------------------
namespace round_rules {

namespace cs = character_select;
using afterhours::Color;

constexpr Color orchid{224, 107, 221, 255};
constexpr Color body_text{230, 220, 247, 255};
constexpr Color knob_off{143, 131, 184, 255};

// makers.cpp has exactly two firing slots: ShootLeft and ShootRight.
constexpr size_t max_weapon_slots = 2;

inline std::string text_for(strings::i18n key) {
  return translation_manager::make_translatable_string(key).get_text();
}

inline std::string mode_name(RoundType type) {
  switch (type) {
  case RoundType::Lives:
    return text_for(strings::i18n::round_type_lives);
  case RoundType::Kills:
    return text_for(strings::i18n::round_type_kills);
  case RoundType::Hippo:
    return text_for(strings::i18n::round_type_hippo);
  case RoundType::TagAndGo:
    return text_for(strings::i18n::round_type_tag);
  }
  return text_for(strings::i18n::unknown);
}

// The one place the clock is put into words. There used to be three copies,
// all emitting different text, plus a dropdown showing "Minutes_1" (TODO 11).
inline std::string time_option_label(RoundSettings::TimeOptions option) {
  switch (option) {
  case RoundSettings::TimeOptions::Unlimited:
    return text_for(strings::i18n::unlimited);
  case RoundSettings::TimeOptions::Seconds_10:
    return text_for(strings::i18n::time_10_seconds);
  case RoundSettings::TimeOptions::Seconds_30:
    return text_for(strings::i18n::time_30_seconds);
  case RoundSettings::TimeOptions::Minutes_1:
    return text_for(strings::i18n::time_1_minute);
  }
  return text_for(strings::i18n::unknown);
}

inline std::string weapon_name(Weapon::Type type) {
  switch (type) {
  case Weapon::Type::Cannon:
    return text_for(strings::i18n::weapon_cannon);
  case Weapon::Type::Shotgun:
    return text_for(strings::i18n::weapon_shotgun);
  case Weapon::Type::Sniper:
    return text_for(strings::i18n::weapon_sniper);
  case Weapon::Type::MachineGun:
    return text_for(strings::i18n::weapon_machine_gun);
  }
  return text_for(strings::i18n::unknown);
}

// Numbers taken from weapons.h and ProjectileSpawnSystem, not invented.
inline const char *weapon_blurb(Weapon::Type type) {
  switch (type) {
  case Weapon::Type::Cannon:
    return "ONE FAT SLUG. 3 HITS AND THEY POP. 1s BETWEEN SHOTS.";
  case Weapon::Type::Shotgun:
    return "4 PELLETS, WIDE SPRAY. 3s BETWEEN SHOTS.";
  case Weapon::Type::Sniper:
    return "ONE SHOT, ONE KILL. 3s RELOAD. DON'T MISS.";
  case Weapon::Type::MachineGun:
    return "12 SHOTS, 0.2s APART. BULLETS DIE AFTER 1s.";
  }
  return "";
}

inline const char *mode_blurb(RoundType type) {
  switch (type) {
  case RoundType::Lives:
    return "LAST KART STILL DRIVING TAKES IT.\n"
           "RUN OUT OF LIVES AND YOU'RE OUT FOR GOOD.";
  case RoundType::Kills:
    return "BLOW UP THE MOST KARTS BEFORE THE CLOCK DIES.\n"
           "DYING JUST REFILLS YOU - NOTHING LOST.";
  case RoundType::Hippo:
    return "HOOVER UP MORE HIPPOS THAN ANYONE ELSE.\n"
           "THE CLOCK DECIDES WHEN IT'S OVER.";
  case RoundType::TagAndGo:
    return "DON'T BE \"IT\". THE TAGGER DRIVES A BIGGER KART.\n"
           "MOST TIME NOT-IT TAKES IT.";
  }
  return "";
}

// Corner tick goes in the fg hook: on_draw_bg draws behind the panel fill.
inline ComponentConfig panel_config(Color edge, const std::string &debug_name) {
  return ComponentConfig{}
      .with_size(ComponentSize{expand(), percent(1.f)})
      .with_custom_background(cs::panel_bg)
      .with_border(edge, 3.f)
      .with_corner_radius(16.f)
      .with_padding(Padding{.top = ui::h720(12.f),
                            .left = ui::w1280(14.f),
                            .bottom = ui::h720(12.f),
                            .right = ui::w1280(14.f)})
      .with_on_draw_fg([](RectangleType r) {
        raylib::DrawRectangleRec(RectangleType{r.x, r.y, 16.f, 4.f}, cs::butter);
        raylib::DrawRectangleRec(RectangleType{r.x, r.y, 4.f, 16.f}, cs::butter);
      })
      .with_debug_name(debug_name);
}

inline ElementResult settings_row(UIContext<InputAction> &context,
                                  Entity &parent, int index,
                                  const std::string &label,
                                  const std::string &debug_name) {
  auto row = imm::hstack(context, mk(parent, index),
                         ComponentConfig{}
                             .with_size(ComponentSize{percent(1.f),
                                                      ui::h720(38.f)})
                             .with_align_items(AlignItems::Center)
                             .with_transparent_bg()
                             .with_no_wrap()
                             .with_debug_name(debug_name));

  imm::div(context, mk(row.ent(), 0),
           ComponentConfig{}
               .with_size(ComponentSize{ui::w1280(150.f), percent(1.f)})
               .with_label(label)
               .with_transparent_bg()
               .with_custom_text_color(body_text)
               .with_alignment(TextAlignment::Left)
               .with_font_size(13.f)
               .with_skip_tabbing(true)
               .with_debug_name(debug_name + "_label"));

  return row;
}

inline bool toggle_row(UIContext<InputAction> &context, Entity &parent,
                       int index, const std::string &label, bool &value,
                       const std::string &debug_name) {
  auto row = settings_row(context, parent, index, label, debug_name);

  const bool on = value;
  auto config = ComponentConfig{}
                    .with_size(ComponentSize{ui::w1280(56.f), ui::h720(24.f)})
                    .with_padding(Padding{})
                    .with_custom_background(cs::well_bg)
                    .with_custom_hover_bg(cs::panel_bg)
                    .with_border(on ? cs::mint : cs::sky, 3.f)
                    .with_corner_radius(12.f)
                    .with_on_draw_fg([on](RectangleType r) {
                      const float knob = r.width * 0.42f;
                      const float pad = 4.f;
                      raylib::DrawRectangleRounded(
                          RectangleType{on ? r.x + r.width - knob - pad
                                           : r.x + pad,
                                        r.y + pad, knob, r.height - pad * 2.f},
                          1.f, 6, on ? cs::mint : knob_off);
                    })
                    .with_debug_name(debug_name + "_switch");
  cs::keep_visuals(config, 12.f);

  if (imm::button(context, mk(row.ent(), 1), config)) {
    value = !value;
    return true;
  }
  return false;
}

inline bool stepper_row(UIContext<InputAction> &context, Entity &parent,
                        int index, const std::string &label,
                        const std::vector<std::string> &options,
                        size_t &option_index, const std::string &debug_name) {
  auto row = settings_row(context, parent, index, label, debug_name);

  return static_cast<bool>(imm::stepper(
      context, mk(row.ent(), 1), options, option_index,
      ComponentConfig{}
          .with_size(ComponentSize{ui::w1280(190.f), ui::h720(30.f)})
          .with_transparent_bg()
          .with_border(cs::mint, 3.f)
          .with_corner_radius(15.f)
          .with_custom_text_color(cs::mint)
          .with_font_size(13.f)
          .with_debug_name(debug_name + "_stepper")));
}

inline void clock_row(UIContext<InputAction> &context, Entity &parent,
                      int index, RoundSettings &settings) {
  std::vector<std::string> options;
  options.reserve(magic_enum::enum_count<RoundSettings::TimeOptions>());
  for (auto option : magic_enum::enum_values<RoundSettings::TimeOptions>())
    options.push_back(time_option_label(option));

  size_t selected = magic_enum::enum_index(settings.time_option).value();
  if (stepper_row(context, parent, index, text_for(strings::i18n::round_length),
                  options, selected, "row_clock"))
    settings.set_time_option(selected);
}

// Only rows backed by a real field in round_settings.h. Nothing here invents
// a setting the game does not read.
inline void mode_rows(UIContext<InputAction> &context, Entity &parent) {
  auto &manager = RoundManager::get();
  auto &settings = manager.get_active_settings();

  switch (manager.active_round_type) {
  case RoundType::Lives: {
    auto &lives = manager.get_active_rt<RoundLivesSettings>();
    static const std::vector<std::string> counts{"1", "2", "3", "4", "5"};
    size_t selected = static_cast<size_t>(
        std::clamp(lives.num_starting_lives, 1, 5) - 1);
    if (stepper_row(context, parent, 0, "LIVES", counts, selected, "row_lives"))
      lives.num_starting_lives = static_cast<int>(selected) + 1;
    break;
  }
  case RoundType::Kills:
    clock_row(context, parent, 0, settings);
    break;
  case RoundType::Hippo: {
    auto &hippo = manager.get_active_rt<RoundHippoSettings>();
    clock_row(context, parent, 0, settings);
    static constexpr std::array<int, 4> hippo_counts{10, 25, 50, 100};
    static const std::vector<std::string> hippo_labels{"10", "25", "50", "100"};
    size_t selected = 2;
    for (size_t i = 0; i < hippo_counts.size(); i++) {
      if (hippo.total_hippos <= hippo_counts[i]) {
        selected = i;
        break;
      }
    }
    if (stepper_row(context, parent, 1, "HIPPOS", hippo_labels, selected,
                    "row_hippos"))
      hippo.set_total_hippos(hippo_counts[selected]);
    break;
  }
  case RoundType::TagAndGo: {
    auto &tag = manager.get_active_rt<RoundTagAndGoSettings>();
    clock_row(context, parent, 0, settings);
    toggle_row(context, parent, 1, text_for(strings::i18n::allow_tag_backs),
               tag.allow_tag_backs, "row_tag_backs");
    break;
  }
  }

  toggle_row(context, parent, 2, "TEAMS", settings.team_mode_enabled,
             "row_teams");
  if (manager.uses_timer())
    toggle_row(context, parent, 3, "SHOW CLOCK", settings.show_countdown_timer,
               "row_show_clock");
}

inline void left_panel(UIContext<InputAction> &context, Entity &parent) {
  auto panel = imm::vstack(context, mk(parent, 0),
                           panel_config(cs::mint, "round_mode_panel"));

  imm::div(context, mk(panel.ent(), 0),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), ui::h720(64.f)})
               .with_label(mode_blurb(RoundManager::get().active_round_type))
               .with_transparent_bg()
               .with_custom_text_color(body_text)
               .with_alignment(TextAlignment::Left)
               .with_font_size(13.f)
               .with_skip_tabbing(true)
               .with_debug_name("round_mode_blurb"));

  auto rows = imm::vstack(context, mk(panel.ent(), 1),
                          ComponentConfig{}
                              .with_size(ComponentSize{percent(1.f), expand()})
                              .with_transparent_bg()
                              .with_debug_name("round_mode_rows"));
  mode_rows(context, rows.ent());
}

inline void weapon_row(UIContext<InputAction> &context, Entity &parent,
                       int index, Weapon::Type type, WeaponSet &weapons) {
  const size_t bit = static_cast<size_t>(type);
  const bool on = weapons.test(bit);
  const size_t picked = weapons.count();
  // Min one, max two: makers.cpp falls back to a fixed pair when the set is
  // empty, which would quietly ignore the player.
  const bool locked = on ? picked <= 1 : picked >= max_weapon_slots;

  auto row = imm::hstack(
      context, mk(parent, index),
      ComponentConfig{}
          .with_size(ComponentSize{percent(1.f), ui::h720(62.f)})
          .with_align_items(AlignItems::Center)
          .with_custom_background(on ? Color{40, 36, 30, 255} : cs::panel_bg)
          .with_border(on ? cs::butter : cs::panel_bg, 2.f)
          .with_corner_radius(12.f)
          .with_no_wrap()
          .with_debug_name(fmt::format("weapon_row_{}", bit)));

  const auto toggle = [&weapons, bit, on, locked]() {
    if (locked)
      return;
    if (on)
      weapons.reset(bit);
    else
      weapons.set(bit);
    RoundManager::get().set_enabled_weapons(weapons.to_ulong());
  };

  auto *sheet_cmp = EntityHelper::get_singleton_cmp<
      afterhours::texture_manager::HasSpritesheet>();

  auto well = ComponentConfig{}
                  .with_size(ComponentSize{ui::w1280(58.f), ui::h720(50.f)})
                  .with_padding(Padding{})
                  .with_custom_background(cs::well_bg)
                  .with_custom_hover_bg(cs::panel_bg)
                  .with_border(on      ? cs::butter
                               : locked ? cs::open_edge
                                        : cs::sky,
                               3.f)
                  .with_corner_radius(12.f)
                  .with_debug_name(fmt::format("weapon_well_{}", bit));
  if (sheet_cmp) {
    const auto sheet = sheet_cmp->texture;
    const auto src = weapon_icon_frame(type);
    // Dimmed via tint: with_opacity on a transparent bg paints a black box.
    // See docs/afterhours_gaps.md.
    const Color tint = on ? cs::butter : locked ? cs::open_text : cs::muted;
    well.with_on_draw_fg([sheet, src, tint](RectangleType r) {
      const float side = std::min(r.width, r.height) * 0.72f;
      const RectangleType dest{r.x + (r.width - side) * 0.5f,
                               r.y + (r.height - side) * 0.5f, side, side};
      raylib::DrawTexturePro(sheet, src, dest, raylib::Vector2{0.f, 0.f}, 0.f,
                             tint);
    });
  }
  cs::keep_visuals(well, 11.f);

  if (imm::button(context, mk(row.ent(), 0), well))
    toggle();

  auto text = imm::vstack(
      context, mk(row.ent(), 1),
      ComponentConfig{}
          .with_size(ComponentSize{expand(), percent(1.f)})
          .with_padding(Padding{.left = ui::w1280(10.f)})
          .with_transparent_bg()
          .with_debug_name(fmt::format("weapon_text_{}", bit)));

  // A second hit target: the well alone is a small one.
  auto name = ComponentConfig{}
                  .with_size(ComponentSize{percent(1.f), ui::h720(20.f)})
                  .with_padding(Padding{})
                  .with_label(weapon_name(type))
                  .with_transparent_bg()
                  .with_custom_hover_bg(cs::panel_bg)
                  .with_custom_text_color(on ? cs::butter : cs::muted)
                  .with_alignment(TextAlignment::Left)
                  .with_skip_tabbing(true)
                  .with_debug_name(fmt::format("weapon_name_{}", bit));
  cs::keep_visuals(name, 13.f);
  if (imm::button(context, mk(text.ent(), 0), name))
    toggle();

  imm::div(context, mk(text.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), ui::h720(18.f)})
               .with_label(weapon_blurb(type))
               .with_transparent_bg()
               .with_custom_text_color(on ? Color{240, 232, 92, 210}
                                          : cs::open_text)
               .with_alignment(TextAlignment::Left)
               .with_font_size(10.f)
               .with_skip_tabbing(true)
               .with_debug_name(fmt::format("weapon_desc_{}", bit)));
}

inline void right_panel(UIContext<InputAction> &context, Entity &parent) {
  auto panel = imm::vstack(context, mk(parent, 1),
                           panel_config(orchid, "round_weapon_panel"));

  imm::div(context, mk(panel.ent(), 0),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), ui::h720(20.f)})
               .with_label("PICK TWO // ONE PER SIDE")
               .with_transparent_bg()
               .with_custom_text_color(cs::butter)
               .with_alignment(TextAlignment::Left)
               .with_font_size(12.f)
               .with_skip_tabbing(true)
               .with_debug_name("weapon_panel_header"));

  auto &weapons = RoundManager::get().get_enabled_weapons();
  for (size_t i = 0; i < WEAPON_COUNT; i++)
    weapon_row(context, panel.ent(), static_cast<int>(i) + 1,
               static_cast<Weapon::Type>(i), weapons);

  imm::div(context, mk(panel.ent(), 90),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), ui::h720(18.f)})
               .with_label(weapons.count() >= max_weapon_slots
                               ? "BOTH SLOTS FULL // UNPICK TO SWAP"
                               : "")
               .with_transparent_bg()
               .with_custom_text_color(cs::mint)
               .with_alignment(TextAlignment::Left)
               .with_font_size(11.f)
               .with_skip_tabbing(true)
               .with_debug_name("weapon_panel_footer"));
}

// Selection is absolute, so an e2e script can name the mode it wants.
inline void mode_tabs(UIContext<InputAction> &context, Entity &parent) {
  static constexpr std::array<const char *, num_round_types> debug_names{
      "tab_mode_lives", "tab_mode_kills", "tab_mode_hippo", "tab_mode_tag"};

  auto tabs = imm::hstack(context, mk(parent, 2),
                          ComponentConfig{}
                              .with_size(ComponentSize{percent(1.f),
                                                       ui::h720(38.f)})
                              .with_gap(ui::w1280(8.f))
                              .with_transparent_bg()
                              .with_no_wrap()
                              .with_debug_name("round_mode_tabs"));

  const RoundType active = RoundManager::get().active_round_type;
  for (size_t i = 0; i < num_round_types; i++) {
    const auto type = static_cast<RoundType>(i);
    const bool on = (type == active);

    auto config = ComponentConfig{}
                      .with_size(ComponentSize{ui::w1280(150.f), percent(1.f)})
                      .with_padding(Padding{})
                      .with_label(mode_name(type))
                      .with_custom_background(on ? orchid : cs::panel_bg)
                      .with_custom_hover_bg(on ? orchid : cs::well_bg)
                      .with_custom_text_color(on ? cs::ink : cs::muted)
                      .with_border(on ? cs::ink : cs::open_edge, 3.f)
                      .with_corner_radius(10.f)
                      .with_debug_name(debug_names[i]);
    cs::keep_visuals(config, 13.f);

    if (imm::button(context, mk(tabs.ent(), static_cast<int>(i)), config) && !on)
      RoundManager::get().set_active_round_type(static_cast<int>(i));
  }
}

} // namespace round_rules

// ----------------------------------------------------------------------------
// Track select ("PICK A TRACK") -- see docs/ui-mock.html section 04.
//
// Every tile is the same size on a 3-wide grid. The old list sized each button
// to its own text, so "Arena" and "Race Track" disagreed and the row ran out
// past the column it lived in. Thumbnails are flat and striped fills, drawn,
// so the set reads as a set before there is any art.
//
// Determinism: a tile click only records the choice. GO commits, and GO is the
// only thing that resolves RANDOM, so nothing calls GetRandomValue while this
// screen is being drawn.
// ----------------------------------------------------------------------------
namespace track_select {

namespace cs = character_select;
namespace rr = round_rules;
using afterhours::Color;

constexpr int columns = 3;
constexpr Color deep_purple{46, 27, 105, 255};
constexpr Color orchid_dim{224, 107, 221, 90};
constexpr Color slate{107, 95, 150, 255};

enum class Fill { Flat, Rows, Cols, Diagonal };

struct Thumb {
  Fill fill;
  Color base;
  Color stripe;
};

// Indexed the same as MapManager::available_maps.
constexpr std::array<Thumb, MapManager::MAP_COUNT> map_thumbs{{
    {Fill::Flat, cs::mint, cs::mint},
    {Fill::Rows, rr::orchid, orchid_dim},
    {Fill::Flat, cs::sky, cs::sky},
    {Fill::Cols, cs::sky, cs::mint},
    {Fill::Cols, cs::butter, rr::orchid},
    {Fill::Flat, slate, slate},
}};

constexpr Thumb random_thumb{Fill::Diagonal, deep_purple, cs::butter};

inline void draw_thumb(RectangleType r, const Thumb &thumb) {
  raylib::DrawRectangleRec(r, thumb.base);
  if (thumb.fill == Fill::Flat)
    return;

  constexpr float band = 6.f;
  begin_scissor_mode(static_cast<int>(r.x), static_cast<int>(r.y),
                     static_cast<int>(r.width), static_cast<int>(r.height));
  switch (thumb.fill) {
  case Fill::Rows:
    for (float y = r.y; y < r.y + r.height; y += band * 2.f)
      raylib::DrawRectangleRec(RectangleType{r.x, y, r.width, band},
                               thumb.stripe);
    break;
  case Fill::Cols:
    for (float x = r.x; x < r.x + r.width; x += band * 2.f)
      raylib::DrawRectangleRec(RectangleType{x, r.y, band, r.height},
                               thumb.stripe);
    break;
  case Fill::Diagonal: {
    const float run = r.width + r.height;
    for (float d = -r.height; d < run; d += band * 2.f)
      raylib::DrawRectanglePro(RectangleType{r.x + d, r.y, band, run},
                               raylib::Vector2{0.f, 0.f}, 45.f, thumb.stripe);
    break;
  }
  case Fill::Flat:
    break;
  }
  end_scissor_mode();
}

inline std::string rules_pill_text(size_t driver_count) {
  auto &manager = RoundManager::get();
  const std::string middle =
      manager.uses_timer()
          ? rr::time_option_label(manager.get_active_settings().time_option)
          : fmt::format("{} LIVES", manager.fetch_num_starting_lives());
  return fmt::format("{} // {} // {}P",
                     rr::mode_name(manager.active_round_type), middle,
                     driver_count);
}

inline std::string track_name(int map_index) {
  if (map_index == MapManager::RANDOM_MAP_INDEX)
    return "RANDOM";
  return MapManager::available_maps[static_cast<size_t>(map_index)]
      .display_name;
}

inline std::string track_blurb(int map_index) {
  if (map_index == MapManager::RANDOM_MAP_INDEX)
    return fmt::format("WE PICK ANY TRACK THAT SUITS {}.",
                       rr::mode_name(RoundManager::get().active_round_type));
  return MapManager::available_maps[static_cast<size_t>(map_index)].description;
}

inline const Thumb &thumb_for(int map_index) {
  if (map_index == MapManager::RANDOM_MAP_INDEX)
    return random_thumb;
  return map_thumbs[static_cast<size_t>(map_index)];
}

inline std::string debug_name_for(int map_index) {
  if (map_index == MapManager::RANDOM_MAP_INDEX)
    return "tile_random";
  return fmt::format("tile_map_{}", map_index);
}

inline void tile(UIContext<InputAction> &context, Entity &row, int column,
                 int map_index, bool on,
                 const std::function<void()> &on_pick) {
  const std::string dbg = debug_name_for(map_index);

  auto cell = imm::div(context, mk(row, column),
                       ComponentConfig{}
                           .with_size(ComponentSize{expand(), percent(1.f)})
                           .with_padding(Padding{.top = ui::h720(4.f),
                                                 .left = ui::w1280(5.f),
                                                 .bottom = ui::h720(4.f),
                                                 .right = ui::w1280(5.f)})
                           .with_transparent_bg()
                           .with_debug_name(dbg + "_cell"));

  // The fill goes in the bg hook, not with_custom_background, because the
  // widget's own fill paints over anything the bg hook drew.
  auto pic =
      ComponentConfig{}
          .with_size(ComponentSize{percent(1.f), expand()})
          .with_padding(Padding{})
          .with_label(map_index == MapManager::RANDOM_MAP_INDEX ? "?" : "")
          .with_custom_text_color(cs::ink)
          .with_transparent_bg()
          .with_border(on ? cs::butter : cs::open_edge, 3.f)
          .disable_rounded_corners()
          .with_alignment(TextAlignment::Center)
          .with_on_draw_bg([thumb = thumb_for(map_index)](RectangleType r) {
            draw_thumb(r, thumb);
          })
          .with_debug_name(dbg);
  cs::keep_visuals(pic, 26.f);

  if (imm::button(context, mk(cell.ent(), 0), pic))
    on_pick();

  imm::div(context, mk(cell.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), ui::h720(18.f)})
               .with_label(track_name(map_index))
               .with_transparent_bg()
               .with_custom_text_color(on ? cs::butter : cs::muted)
               .with_font_size(11.f)
               .with_skip_tabbing(true)
               .with_debug_name(dbg + "_name"));
}

inline void preview_art(UIContext<InputAction> &context, Entity &panel,
                        int map_index) {
  auto config = ComponentConfig{}
                    .with_size(ComponentSize{percent(1.f), percent(1.f)})
                    .with_transparent_bg()
                    .with_alignment(TextAlignment::Center)
                    .with_skip_tabbing(true)
                    .with_debug_name("map_preview_art");

  // Same "?" for RANDOM and for a track whose preview has not been rendered
  // yet: in both cases we genuinely cannot show what you are about to drive.
  auto &maps = MapManager::get();
  if (map_index == MapManager::RANDOM_MAP_INDEX ||
      !maps.preview_textures_initialized) {
    config.with_label("?").with_custom_text_color(cs::butter).with_font_size(
        96.f);
    imm::div(context, mk(panel, 0), config);
    return;
  }

  const auto tex = maps.get_preview_texture(map_index).texture;
  config.with_on_draw_bg([tex](RectangleType r) {
    const float scale = std::min(r.width / static_cast<float>(tex.width),
                                 r.height / static_cast<float>(tex.height));
    const float w = tex.width * scale;
    const float h = tex.height * scale;
    const RectangleType dest{r.x + (r.width - w) * 0.5f,
                             r.y + (r.height - h) * 0.5f, w, h};
    // Render textures come off the GPU bottom-up; a negative source height is
    // how raylib asks for the flip back.
    const RectangleType src{0.f, 0.f, static_cast<float>(tex.width),
                            -static_cast<float>(tex.height)};
    raylib::DrawTexturePro(tex, src, dest, raylib::Vector2{0.f, 0.f}, 0.f,
                           raylib::WHITE);
  });
  imm::div(context, mk(panel, 0), config);
}

} // namespace track_select

void ScheduleMainMenuUI::round_end_player_column(
    Entity &parent, UIContext<InputAction> &context, const size_t index,
    const std::vector<OptEntity> &round_players,
    const std::vector<OptEntity> &round_ais, std::optional<int> ranking) {

  bool is_slot_ai = index >= round_players.size();

  OptEntity car;
  if (index < round_players.size()) {
    car = round_players[index];
  } else {
    car = round_ais[index - round_players.size()];
  }

  if (!car.has_value()) {
    return;
  }

  auto bg_color = car->get<HasColor>().color();
  const auto num_cols = std::min(
      4.f, static_cast<float>(round_players.size() + round_ais.size()));

  float card_v = 1.0f;
  if (!animation_control::disabled()) {
    afterhours::animation::one_shot(
        UIKey::RoundEndCard, index,
        ui_anims::make_round_end_card_stagger(index));
    card_v = afterhours::animation::clamp_value(UIKey::RoundEndCard, index,
                                                0.0f, 1.0f);
  }
  auto column =
      imm::div(context, mk(parent, (int)index),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(1.f / num_cols, 0.1f),
                                            percent(1.f, 0.4f)})
                   .with_margin(Spacing::xs)
                   .with_custom_background(bg_color)
                   .with_translate(0.0f, (1.0f - card_v) * 20.0f)
                   .with_opacity(card_v)
                   .disable_rounded_corners());

  // Create player label
  std::string player_label = fmt::format("{} {}", index, car->id);

  // Get stats text based on round type
  std::optional<std::string> stats_text;
  switch (RoundManager::get().active_round_type) {
  case RoundType::Lives:
    if (car->has<HasMultipleLives>()) {
      stats_text = translation_manager::translate_formatted(
          translation_manager::make_translatable_string(
              strings::i18n::lives_label)
              .set_param(translation_manager::i18nParam::number_count,
                         car->get<HasMultipleLives>().num_lives_remaining,
                         translation_manager::translation_param));
    }
    break;
  case RoundType::Kills:
    if (car->has<HasKillCountTracker>()) {
      stats_text = translation_manager::translate_formatted(
          translation_manager::make_translatable_string(
              strings::i18n::kills_label)
              .set_param(translation_manager::i18nParam::number_count,
                         car->get<HasKillCountTracker>().kills,
                         translation_manager::translation_param));
    }
    break;
  case RoundType::Hippo:
    if (car->has<HasHippoCollection>()) {
      stats_text = translation_manager::translate_formatted(
          translation_manager::make_translatable_string(
              strings::i18n::hippos_label)
              .set_param(translation_manager::i18nParam::number_count,
                         car->get<HasHippoCollection>().get_hippo_count(),
                         translation_manager::translation_param));
    } else {
      stats_text = translation_manager::make_translatable_string(
                       strings::i18n::hippos_zero)
                       .get_text();
    }
    break;
  case RoundType::TagAndGo:
    if (car->has<HasTagAndGoTracking>()) {
      stats_text = translation_manager::translate_formatted(
          translation_manager::make_translatable_string(
              strings::i18n::not_it_timer)
              // Rounded here, not in the format string: TranslatableString
              // stringifies every param, so "{:.1f}" would throw.
              .set_param(translation_manager::i18nParam::number_time,
                         fmt::format("{:.1f}", car->get<HasTagAndGoTracking>()
                                                   .time_as_not_it),
                         translation_manager::translation_param));
    }
    break;
  default:
    stats_text =
        translation_manager::make_translatable_string(strings::i18n::unknown)
            .get_text();
    break;
  }

  std::optional<std::string> kills_text;
  if (car->has<HasKillCountTracker>()) {
    kills_text = translation_manager::translate_formatted(
        translation_manager::make_translatable_string(
            strings::i18n::kills_label)
            .set_param(translation_manager::i18nParam::number_count,
                       car->get<HasKillCountTracker>().kills,
                       translation_manager::translation_param));
  }

  // Score roll-up value (0..1). We keep it generic regardless of round type
  float score_t = 1.0f;
  if (!animation_control::disabled()) {
    afterhours::animation::one_shot(UIKey::RoundEndScore, index, [](auto h) {
      h.from(0.0f).to(1.0f, 0.8f,
             afterhours::animation::EasingType::EaseOutQuad);
    });
    score_t = afterhours::animation::clamp_value(UIKey::RoundEndScore, index,
                                                 0.0f, 1.0f);
  }

  // Compute animated stats text per-round
  std::optional<std::string> animated_stats = std::nullopt;
  switch (RoundManager::get().active_round_type) {
  case RoundType::Lives: {
    if (car->has<HasMultipleLives>()) {
      int final_val = car->get<HasMultipleLives>().num_lives_remaining;
      int shown = static_cast<int>(std::round(score_t * final_val));
      animated_stats = translation_manager::translate_formatted(
          translation_manager::make_translatable_string(
              strings::i18n::lives_label)
              .set_param(translation_manager::i18nParam::number_count, shown,
                         translation_manager::translation_param));
    }
    break;
  }
  case RoundType::Kills: {
    if (car->has<HasKillCountTracker>()) {
      int final_val = car->get<HasKillCountTracker>().kills;
      int shown = static_cast<int>(std::round(score_t * final_val));
      animated_stats = translation_manager::translate_formatted(
          translation_manager::make_translatable_string(
              strings::i18n::kills_label)
              .set_param(translation_manager::i18nParam::number_count, shown,
                         translation_manager::translation_param));
    }
    break;
  }
  case RoundType::Hippo: {
    int final_val = car->has<HasHippoCollection>()
                        ? car->get<HasHippoCollection>().get_hippo_count()
                        : 0;
    int shown = static_cast<int>(std::round(score_t * final_val));
    animated_stats = translation_manager::translate_formatted(
        translation_manager::make_translatable_string(
            strings::i18n::hippos_label)
            .set_param(translation_manager::i18nParam::number_count, shown,
                       translation_manager::translation_param));
    break;
  }
  case RoundType::TagAndGo: {
    if (car->has<HasTagAndGoTracking>()) {
      float final_val = car->get<HasTagAndGoTracking>().time_as_not_it;
      animated_stats = translation_manager::translate_formatted(
          translation_manager::make_translatable_string(
              strings::i18n::not_it_timer)
              .set_param(translation_manager::i18nParam::number_time,
                         fmt::format("{:.1f}", score_t * final_val),
                         translation_manager::translation_param));
    }
    break;
  }
  default: {
    break;
  }
  }

  // Combine round-specific stats with kill count
  std::optional<std::string> combined_stats;
  std::string final_stats_text =
      animated_stats.has_value()
          ? animated_stats.value()
          : (stats_text.has_value() ? stats_text.value() : "");

  if (kills_text.has_value()) {
    if (!final_stats_text.empty()) {
      combined_stats = final_stats_text + " | " + kills_text.value();
    } else {
      combined_stats = kills_text.value();
    }
  } else if (!final_stats_text.empty()) {
    combined_stats = final_stats_text;
  }

  ui_helpers::PlayerCardData data{
      .parent = column.ent(),
      .index = 0,
      .label = player_label,
      .bg_color = bg_color,
      .is_ai = is_slot_ai,
      .ranking = ranking,
      .stats_text = combined_stats,
  };

  ui_helpers::create_player_card(context, column.ent(), data);
}

std::map<EntityID, int> ScheduleMainMenuUI::get_tag_and_go_rankings(
    const std::vector<OptEntity> &round_players,
    const std::vector<OptEntity> &round_ais) {
  std::map<EntityID, int> rankings;

  // Combine all players and AIs
  std::vector<std::pair<EntityID, float>> player_times;

  for (const auto &player : round_players) {
    if (player->has<HasTagAndGoTracking>()) {
      player_times.emplace_back(
          player->id, player->get<HasTagAndGoTracking>().time_as_not_it);
    }
  }

  for (const auto &ai : round_ais) {
    if (ai->has<HasTagAndGoTracking>()) {
      player_times.emplace_back(ai->id,
                                ai->get<HasTagAndGoTracking>().time_as_not_it);
    }
  }

  // Sort by runner time (highest first - most time not it wins)
  std::sort(player_times.begin(), player_times.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; });

  // Assign rankings (1-based)
  for (size_t i = 0; i < player_times.size(); ++i) {
    rankings[player_times[i].first] = static_cast<int>(i + 1);
  }

  return rankings;
}

Screen ScheduleMainMenuUI::character_creation(Entity &entity,
                                              UIContext<InputAction> &context) {
  namespace cs = character_select;

  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("character_creation"));

  auto content =
      imm::vstack(context, mk(elem.ent()),
                  ComponentConfig{}
                      .with_size(ComponentSize{screen_pct(0.90f, 1.f),
                                               screen_pct(0.86f, 1.f)})
                      .with_absolute_position(screen_pct(0.05f),
                                              screen_pct(0.07f))
                      .with_transparent_bg()
                      .with_debug_name("character_content"));

  auto &active_settings = RoundManager::get().get_active_settings();
  const bool team_mode = active_settings.team_mode_enabled;

  // --- header: heading + FREE FOR ALL | TEAMS -------------------------------
  auto header =
      imm::hstack(context, mk(content.ent(), 0),
                  ComponentConfig{}
                      .with_size(ComponentSize{percent(1.f), ui::h720(54.f)})
                      .with_align_items(AlignItems::Center)
                      .with_transparent_bg()
                      .with_no_wrap()
                      .with_debug_name("character_header"));

  imm::div(context, mk(header.ent(), 0),
           ComponentConfig{}
               .with_size(ComponentSize{expand(), percent(1.f)})
               .with_label("WHO'S DRIVING")
               .with_font_size(34.f)
               .with_alignment(TextAlignment::Left)
               .with_transparent_bg()
               .with_skip_tabbing(true)
               .with_debug_name("character_heading"));

  // The old imm::checkbox labelled "Team Mode" read as an afterthought. A
  // two-up segmented control with the live half filled says which mode you
  // are in without reading anything.
  auto segmented =
      imm::hstack(context, mk(header.ent(), 1),
                  ComponentConfig{}
                      .with_size(ComponentSize{ui::w1280(320.f),
                                               ui::h720(42.f)})
                      .with_custom_background(cs::well_bg)
                      .with_border(cs::butter, 3.f)
                      .with_corner_radius(20.f)
                      .with_no_wrap()
                      .with_debug_name("mode_segmented"));

  const auto segment = [&](int idx, const char *label, bool on,
                           const char *debug_name) {
    auto config =
        ComponentConfig{}
            .with_size(ComponentSize{expand(), percent(1.f)})
            .with_padding(Padding{})
            .with_label(label)
            .with_custom_background(on ? cs::butter : cs::well_bg)
            .with_custom_hover_bg(on ? cs::butter : cs::panel_bg)
            .with_custom_text_color(on ? cs::ink : cs::muted)
            .with_corner_radius(18.f)
            .with_debug_name(debug_name);
    cs::keep_visuals(config, 13.f);
    return static_cast<bool>(
               imm::button(context, mk(segmented.ent(), idx), config)) &&
           !on;
  };

  if (segment(0, "FREE FOR ALL", !team_mode, "mode_free_for_all"))
    active_settings.team_mode_enabled = false;
  if (segment(1, "TEAMS", team_mode, "mode_teams"))
    active_settings.team_mode_enabled = true;

  imm::div(context, mk(content.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{ui::w1280(260.f), ui::h720(8.f)})
               .with_custom_background(cs::butter)
               .with_skip_tabbing(true)
               .with_debug_name("character_rule"));

  // --- slots ----------------------------------------------------------------
  std::vector<cs::Slot> slots;
  slots.reserve(input::MAX_GAMEPAD_ID);
  for (size_t i = 0; i < players.size(); i++)
    slots.push_back({&players[i].get(), i, false});
  for (size_t i = 0; i < ais.size(); i++)
    slots.push_back({&ais[i].get(), i, true});

  if (team_mode) {
    // Everyone needs a team before we can group by one.
    for (size_t i = 0; i < slots.size(); i++)
      slots[i].car->addComponentIfMissing<TeamID>(i % 2 == 0 ? 0 : 1);
    // One grid, Team A first -- the user asked for grouping, not two columns.
    std::stable_sort(slots.begin(), slots.end(),
                     [](const cs::Slot &a, const cs::Slot &b) {
                       return a.car->get<TeamID>().team_id <
                              b.car->get<TeamID>().team_id;
                     });
  }

  slots.resize(input::MAX_GAMEPAD_ID);

  auto grid = imm::vstack(context, mk(content.ent(), 2),
                          ComponentConfig{}
                              .with_size(ComponentSize{percent(1.f), expand()})
                              .with_transparent_bg()
                              .with_debug_name("character_grid"));

  constexpr int kRows = 2;
  constexpr int kCols = input::MAX_GAMEPAD_ID / kRows;
  for (int row_id = 0; row_id < kRows; row_id++) {
    auto row = imm::hstack(
        context, mk(grid.ent(), row_id),
        ComponentConfig{}
            .with_size(ComponentSize{percent(1.f), percent(1.f / kRows)})
            .with_transparent_bg()
            .with_no_wrap()
            .with_debug_name(fmt::format("character_grid_row_{}", row_id)));

    for (int col = 0; col < kCols; col++) {
      const int slot_index = row_id * kCols + col;
      auto cell = imm::div(
          context, mk(row.ent(), col),
          ComponentConfig{}
              .with_size(ComponentSize{expand(), percent(1.f)})
              .with_padding(Padding{.top = ui::h720(6.f),
                                    .left = ui::w1280(6.f),
                                    .bottom = ui::h720(6.f),
                                    .right = ui::w1280(6.f)})
              .with_transparent_bg()
              .with_debug_name(fmt::format("slot_{}_cell", slot_index)));

      cs::slot_card(context, cell.ent(), slot_index,
                    slots[static_cast<size_t>(slot_index)], team_mode);
    }
  }

  // --- bottom bar -----------------------------------------------------------
  auto bottom =
      imm::hstack(context, mk(content.ent(), 3),
                  ComponentConfig{}
                      .with_size(ComponentSize{percent(1.f), ui::h720(54.f)})
                      .with_align_items(AlignItems::Center)
                      .with_transparent_bg()
                      .with_no_wrap()
                      .with_debug_name("character_bottom_bar"));

  const auto chrome_button = [&](int idx, const std::string &label,
                                 Color fill, Color text,
                                 const char *debug_name) {
    auto config = ComponentConfig{}
                      .with_size(ComponentSize{ui::w1280(150.f),
                                               ui::h720(42.f)})
                      .with_label(label)
                      .with_custom_background(fill)
                      .with_custom_text_color(text)
                      .with_border(cs::ink, 3.f)
                      .with_corner_radius(12.f)
                      .with_debug_name(debug_name);
    cs::keep_visuals(config, 15.f);
    animation_control::apply_slide_in(config);
    return static_cast<bool>(imm::button(context, mk(bottom.ent(), idx), config));
  };

  if (chrome_button(0,
                    translation_manager::make_translatable_string(
                        strings::i18n::back)
                        .get_text(),
                    cs::well_bg, cs::mint, "btn_back")) {
    navigation::back();
  }

  imm::div(context, mk(bottom.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{expand(), percent(1.f)})
               .with_label("A ADD BOT // X PAINT // < > SKILL")
               .with_transparent_bg()
               .with_custom_text_color(cs::mint)
               .with_alignment(TextAlignment::Left)
               .with_font_size(12.f)
               .with_skip_tabbing(true)
               .with_debug_name("character_hints"));

  const size_t driver_count = players.size() + ais.size();
  imm::div(context, mk(bottom.ent(), 2),
           ComponentConfig{}
               .with_size(ComponentSize{ui::w1280(120.f), percent(1.f)})
               .with_label(fmt::format("{} DRIVER{}", driver_count,
                                       driver_count == 1 ? "" : "S"))
               .with_transparent_bg()
               .with_custom_text_color(cs::butter)
               .with_font_size(12.f)
               .with_skip_tabbing(true)
               .with_debug_name("character_driver_count"));

  if (chrome_button(3, "NEXT", cs::butter, cs::ink, "btn_next")) {
    navigation::to(GameStateManager::Screen::RoundSettings);
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

void ScheduleMainMenuUI::for_each_with(Entity &entity,
                                       UIContext<InputAction> &context, float) {
  GameStateManager::get().update_screen();

  switch (get_active_screen()) {
  case Screen::None:
    break;
  case Screen::CharacterCreation:
    set_active_screen(character_creation(entity, context));
    break;
  case Screen::About:
    set_active_screen(about_screen(entity, context));
    break;
  case Screen::Settings:
    set_active_screen(settings_screen(entity, context));
    break;
  case Screen::Main:
    set_active_screen(main_screen(entity, context));
    break;
  case Screen::RoundSettings:
    set_active_screen(round_settings(entity, context));
    break;
  case Screen::MapSelection:
    set_active_screen(map_selection(entity, context));
    break;
  case Screen::RoundEnd:
    set_active_screen(round_end_screen(entity, context));
    break;
  }
}

bool ScheduleDebugUI::should_run(float dt) {
  enableCooldown -= dt;

  if (enableCooldown < 0) {
    enableCooldown = enableCooldownReset;
    input::PossibleInputCollector inpc = input::get_input_collector();

    bool debug_pressed =
        std::ranges::any_of(inpc.inputs(), [](const auto &actions_done) {
          return action_matches(actions_done.action,
                                InputAction::ToggleUIDebug);
        });
    if (debug_pressed) {
      enabled = !enabled;
    }
  }
  return enabled;
}

void ScheduleDebugUI::for_each_with(Entity &entity,
                                    UIContext<InputAction> &context, float) {

  if (!enabled) {
    return;
  }

  struct SliderSpec {
    const char *debug_name;
    std::function<std::string()> make_label;
    std::function<float()> get_pct;
    std::function<void(float)> set_pct;
  };

  const std::array<SliderSpec, 11> all_specs{
      SliderSpec{"max_speed",
                 []() {
                   return fmt::format("Max Speed\n {:.2f} m/s",
                                      Config::get().max_speed.data);
                 },
                 []() { return Config::get().max_speed.get_pct(); },
                 [](float value) { Config::get().max_speed.set_pct(value); }},
      SliderSpec{"breaking_acceleration",
                 []() {
                   return fmt::format("Breaking \nPower \n -{:.2f} m/s^2",
                                      Config::get().breaking_acceleration.data);
                 },
                 []() { return Config::get().breaking_acceleration.get_pct(); },
                 [](float value) {
                   Config::get().breaking_acceleration.set_pct(value);
                 }},
      SliderSpec{"forward_acceleration",
                 []() {
                   return fmt::format("Forward \nAcceleration \n {:.2f} m/s^2",
                                      Config::get().forward_acceleration.data);
                 },
                 []() { return Config::get().forward_acceleration.get_pct(); },
                 [](float value) {
                   Config::get().forward_acceleration.set_pct(value);
                 }},
      SliderSpec{"reverse_acceleration",
                 []() {
                   return fmt::format("Reverse \nAcceleration \n {:.2f} m/s^2",
                                      Config::get().reverse_acceleration.data);
                 },
                 []() { return Config::get().reverse_acceleration.get_pct(); },
                 [](float value) {
                   Config::get().reverse_acceleration.set_pct(value);
                 }},
      SliderSpec{
          "boost_acceleration",
          []() {
            return fmt::format("Boost \nAcceleration \n {:.2f} m/s^2",
                               Config::get().boost_acceleration.data);
          },
          []() { return Config::get().boost_acceleration.get_pct(); },
          [](float value) { Config::get().boost_acceleration.set_pct(value); }},
      SliderSpec{"boost_decay_percent",
                 []() {
                   return fmt::format("Boost \nDecay \n {:.2f} decay%/frame",
                                      Config::get().boost_decay_percent.data);
                 },
                 []() { return Config::get().boost_decay_percent.get_pct(); },
                 [](float value) {
                   Config::get().boost_decay_percent.set_pct(value);
                 }},
      SliderSpec{
          "skid_threshold",
          []() {
            return fmt::format("Skid \nThreshold \n {:.2f} %",
                               Config::get().skid_threshold.data);
          },
          []() { return Config::get().skid_threshold.get_pct(); },
          [](float value) { Config::get().skid_threshold.set_pct(value); }},
      SliderSpec{"steering_sensitivity",
                 []() {
                   return fmt::format("Steering \nSensitivity \n {:.2f} %",
                                      Config::get().steering_sensitivity.data);
                 },
                 []() { return Config::get().steering_sensitivity.get_pct(); },
                 [](float value) {
                   Config::get().steering_sensitivity.set_pct(value);
                 }},
      SliderSpec{
          "minimum_steering_radius",
          []() {
            return fmt::format("Min Steering \nSensitivity \n {:.2f} m",
                               Config::get().minimum_steering_radius.data);
          },
          []() { return Config::get().minimum_steering_radius.get_pct(); },
          [](float value) {
            Config::get().minimum_steering_radius.set_pct(value);
          }},
      SliderSpec{
          "maximum_steering_radius",
          []() {
            return fmt::format("Max Steering \nSensitivity \n {:.2f} m",
                               Config::get().maximum_steering_radius.data);
          },
          []() { return Config::get().maximum_steering_radius.get_pct(); },
          [](float value) {
            Config::get().maximum_steering_radius.set_pct(value);
          }},
      SliderSpec{
          "collision_scalar",
          []() {
            return fmt::format("Collision \nScalar \n {:.4f}",
                               Config::get().collision_scalar.data);
          },
          []() { return Config::get().collision_scalar.get_pct(); },
          [](float value) { Config::get().collision_scalar.set_pct(value); }},
  };

  auto screen_container =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(0.5f)})
                   .with_absolute_position()
                   .with_debug_name("debug_screen_container"));

  const int items_per_row = 3;
  const int num_rows =
      static_cast<int>((all_specs.size() + items_per_row - 1) / items_per_row);
  for (int row = 0; row < num_rows; ++row) {
    const int start = row * items_per_row;
    const int remaining = static_cast<int>(all_specs.size()) - start;
    if (remaining <= 0)
      break;
    const int count_in_row = std::min(items_per_row, remaining);
    const float row_height = 1.f / static_cast<float>(num_rows);

    auto row_elem = imm::hstack(
        context, mk(screen_container.ent(), row),
        ComponentConfig{}
            .with_size(ComponentSize{percent(1.f), percent(row_height)}));

    for (int j = 0; j < count_in_row; ++j) {
      const auto &spec = all_specs[start + j];
      float pct = spec.get_pct();
      auto label = spec.make_label();
      if (auto result =
              slider(context, mk(row_elem.ent(), row * items_per_row + j), pct,
                     ComponentConfig{}
                         .with_size(ComponentSize{pixels(200.f), pixels(50.f)})
                         .with_label(std::move(label))
                         .with_skip_tabbing(true));
          result) {
        spec.set_pct(result.as<float>());
      }
    }
  }
}

bool SchedulePauseUI::should_run(float) {
  inpc = input::get_input_collector();
  return GameStateManager::get().is_game_active() ||
         GameStateManager::get().is_paused();
}

void SchedulePauseUI::for_each_with(Entity &entity,
                                    UIContext<InputAction> &context, float) {
  const bool pause_pressed =
      std::ranges::any_of(inpc.inputs_pressed(), [](const auto &actions_done) {
        return action_matches(actions_done.action, InputAction::PauseButton);
      });

  if (pause_pressed) {
    if (GameStateManager::get().is_paused()) {
      GameStateManager::get().unpause_game();
      return;
    } else if (GameStateManager::get().is_game_active()) {
      GameStateManager::get().pause_game();
      return;
    }
  }

  if (!GameStateManager::get().is_paused()) {
    return;
  }

  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("pause_screen"));

  auto left_col =
      imm::vstack(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(0.2f), percent(1.0f)})
                   .with_padding(Padding{.top = screen_pct(0.02f),
                                         .left = screen_pct(0.02f)})
                   .with_debug_name("pause_left"));

  imm::div(context, mk(left_col.ent(), 0),
           ComponentConfig{}
               .with_label(translation_manager::make_translatable_string(
                               strings::i18n::paused)
                               .get_text())
               .with_skip_tabbing(true)
               .with_size(ComponentSize{pixels(400.f), pixels(100.f)}));

  if (imm::button(context, mk(left_col.ent(), 1),
                  ComponentConfig{}
                      .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                            .left = imm::DefaultSpacing::tiny(),
                                            .bottom = imm::DefaultSpacing::tiny(),
                                            .right = imm::DefaultSpacing::tiny()})
                      .with_label(translation_manager::make_translatable_string(
                                      strings::i18n::resume)
                                      .get_text()))) {
    GameStateManager::get().unpause_game();
  }

  if (imm::button(context, mk(left_col.ent(), 2),
                  ComponentConfig{}
                      .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                            .left = imm::DefaultSpacing::tiny(),
                                            .bottom = imm::DefaultSpacing::tiny(),
                                            .right = imm::DefaultSpacing::tiny()})
                      .with_label(translation_manager::make_translatable_string(
                                      strings::i18n::back_to_setup)
                                      .get_text()))) {
    GameStateManager::get().end_game();
  }

  if (imm::button(context, mk(left_col.ent(), 3),
                  ComponentConfig{}
                      .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                            .left = imm::DefaultSpacing::tiny(),
                                            .bottom = imm::DefaultSpacing::tiny(),
                                            .right = imm::DefaultSpacing::tiny()})
                      .with_label(translation_manager::make_translatable_string(
                                      strings::i18n::exit_game)
                                      .get_text()))) {
    exit_game();
  }
}

Screen ScheduleMainMenuUI::round_settings(Entity &entity,
                                          UIContext<InputAction> &context) {
  namespace rr = round_rules;
  namespace cs = character_select;

  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_debug_name("round_settings")
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position());

  auto content =
      imm::vstack(context, mk(elem.ent()),
                  ComponentConfig{}
                      .with_size(ComponentSize{screen_pct(0.90f, 1.f),
                                               screen_pct(0.86f, 1.f)})
                      .with_absolute_position(screen_pct(0.05f),
                                              screen_pct(0.07f))
                      .with_transparent_bg()
                      .with_debug_name("round_settings_content"));

  imm::div(context, mk(content.ent(), 0),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), ui::h720(48.f)})
               .with_label("HOW DO WE WIN")
               .with_font_size(34.f)
               .with_alignment(TextAlignment::Left)
               .with_transparent_bg()
               .with_skip_tabbing(true)
               .with_debug_name("round_settings_heading"));

  imm::div(context, mk(content.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{ui::w1280(280.f), ui::h720(8.f)})
               .with_custom_background(cs::butter)
               .with_skip_tabbing(true)
               .with_debug_name("round_settings_rule"));

  rr::mode_tabs(context, content.ent());

  auto panels =
      imm::hstack(context, mk(content.ent(), 3),
                  ComponentConfig{}
                      .with_size(ComponentSize{percent(1.f), expand()})
                      .with_gap(ui::w1280(16.f))
                      .with_margin(Margin{.top = ui::h720(12.f),
                                          .bottom = ui::h720(12.f)})
                      .with_transparent_bg()
                      .with_no_wrap()
                      .with_debug_name("round_settings_panels"));

  rr::left_panel(context, panels.ent());
  rr::right_panel(context, panels.ent());

  auto bottom =
      imm::hstack(context, mk(content.ent(), 4),
                  ComponentConfig{}
                      .with_size(ComponentSize{percent(1.f), ui::h720(48.f)})
                      .with_align_items(AlignItems::Center)
                      .with_transparent_bg()
                      .with_no_wrap()
                      .with_debug_name("round_settings_bottom_bar"));

  const auto chrome_button = [&](int idx, const std::string &label,
                                 Color fill, Color text,
                                 const char *debug_name) {
    auto config = ComponentConfig{}
                      .with_size(ComponentSize{ui::w1280(190.f),
                                               ui::h720(42.f)})
                      .with_label(label)
                      .with_custom_background(fill)
                      .with_custom_text_color(text)
                      .with_border(cs::ink, 3.f)
                      .with_corner_radius(12.f)
                      .with_debug_name(debug_name);
    cs::keep_visuals(config, 15.f);
    animation_control::apply_slide_in(config);
    return static_cast<bool>(
        imm::button(context, mk(bottom.ent(), idx), config));
  };

  if (chrome_button(0,
                    translation_manager::make_translatable_string(
                        strings::i18n::back)
                        .get_text(),
                    cs::well_bg, cs::mint, "btn_back")) {
    navigation::back();
  }

  imm::div(context, mk(bottom.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{expand(), percent(1.f)})
               .with_transparent_bg()
               .with_skip_tabbing(true)
               .with_debug_name("round_settings_bottom_gap"));

  if (chrome_button(2,
                    translation_manager::make_translatable_string(
                        strings::i18n::select_map)
                        .get_text(),
                    cs::butter, cs::ink, "btn_select_map")) {
    navigation::to(GameStateManager::Screen::MapSelection);
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::map_selection(Entity &entity,
                                         UIContext<InputAction> &context) {
  namespace ts = track_select;
  namespace cs = character_select;

  auto &maps = MapManager::get();
  const auto compatible =
      maps.get_maps_for_round_type(RoundManager::get().active_round_type);

  // The round type can change after a track was picked, leaving a selection
  // this mode cannot play. Drop back to RANDOM rather than launching it.
  const bool selection_playable =
      maps.get_selected_map() == MapManager::RANDOM_MAP_INDEX ||
      std::ranges::any_of(compatible, [&](const auto &pair) {
        return pair.first == maps.get_selected_map();
      });
  if (!selection_playable)
    maps.set_selected_map(MapManager::RANDOM_MAP_INDEX);
  const int selected = maps.get_selected_map();

  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("map_selection"));

  auto content =
      imm::vstack(context, mk(elem.ent()),
                  ComponentConfig{}
                      .with_size(ComponentSize{screen_pct(0.90f, 1.f),
                                               screen_pct(0.86f, 1.f)})
                      .with_absolute_position(screen_pct(0.05f),
                                              screen_pct(0.07f))
                      .with_transparent_bg()
                      .with_debug_name("map_selection_content"));

  auto header =
      imm::hstack(context, mk(content.ent(), 0),
                  ComponentConfig{}
                      .with_size(ComponentSize{percent(1.f), ui::h720(48.f)})
                      .with_align_items(AlignItems::Center)
                      .with_transparent_bg()
                      .with_no_wrap()
                      .with_debug_name("map_header"));

  imm::div(context, mk(header.ent(), 0),
           ComponentConfig{}
               .with_size(ComponentSize{expand(), percent(1.f)})
               .with_label("PICK A TRACK")
               .with_font_size(34.f)
               .with_alignment(TextAlignment::Left)
               .with_transparent_bg()
               .with_skip_tabbing(true)
               .with_debug_name("map_heading"));

  // The round rules used to be re-listed down the left of this screen next to
  // the tracks. One pill, because picking a track is the job here.
  imm::div(context, mk(header.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{ui::w1280(320.f), ui::h720(30.f)})
               .with_label(ts::rules_pill_text(players.size() + ais.size()))
               .with_custom_background(cs::well_bg)
               .with_custom_text_color(cs::butter)
               .with_corner_radius(15.f)
               .with_font_size(12.f)
               .with_skip_tabbing(true)
               .with_debug_name("map_rules_pill"));

  imm::div(context, mk(content.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{ui::w1280(240.f), ui::h720(8.f)})
               .with_custom_background(cs::butter)
               .with_skip_tabbing(true)
               .with_debug_name("map_rule"));

  auto body = imm::hstack(context, mk(content.ent(), 2),
                          ComponentConfig{}
                              .with_size(ComponentSize{percent(1.f), expand()})
                              .with_gap(ui::w1280(16.f))
                              .with_margin(Margin{.top = ui::h720(12.f),
                                                  .bottom = ui::h720(12.f)})
                              .with_transparent_bg()
                              .with_no_wrap()
                              .with_debug_name("map_body"));

  auto grid = imm::vstack(context, mk(body.ent(), 0),
                          ComponentConfig{}
                              .with_size(ComponentSize{ui::w1280(420.f),
                                                       percent(1.f)})
                              .with_transparent_bg()
                              .with_debug_name("map_grid"));

  std::vector<int> tiles;
  tiles.reserve(compatible.size() + 1);
  tiles.push_back(MapManager::RANDOM_MAP_INDEX);
  for (const auto &pair : compatible)
    tiles.push_back(pair.first);

  const int row_count =
      (static_cast<int>(tiles.size()) + ts::columns - 1) / ts::columns;
  for (int row_id = 0; row_id < row_count; row_id++) {
    auto row = imm::hstack(
        context, mk(grid.ent(), row_id),
        ComponentConfig{}
            .with_size(ComponentSize{percent(1.f), ui::h720(104.f)})
            .with_transparent_bg()
            .with_no_wrap()
            .with_debug_name(fmt::format("map_grid_row_{}", row_id)));

    for (int column = 0; column < ts::columns; column++) {
      const size_t slot =
          static_cast<size_t>(row_id * ts::columns + column);
      // Empty cells still take a column, so a short last row cannot stretch
      // the tiles above it.
      if (slot >= tiles.size()) {
        imm::div(context, mk(row.ent(), column),
                 ComponentConfig{}
                     .with_size(ComponentSize{expand(), percent(1.f)})
                     .with_transparent_bg()
                     .with_skip_tabbing(true)
                     .with_debug_name(
                         fmt::format("map_grid_gap_{}_{}", row_id, column)));
        continue;
      }

      const int map_index = tiles[slot];
      ts::tile(context, row.ent(), column, map_index, map_index == selected,
               [&maps, map_index]() { maps.set_selected_map(map_index); });
    }
  }

  auto right = imm::vstack(context, mk(body.ent(), 1),
                           ComponentConfig{}
                               .with_size(ComponentSize{expand(), percent(1.f)})
                               .with_gap(ui::h720(14.f))
                               .with_transparent_bg()
                               .with_debug_name("map_right"));

  auto preview_cfg =
      round_rules::panel_config(cs::butter, "map_preview_panel");
  preview_cfg.with_size(ComponentSize{percent(1.f), expand()});
  auto preview = imm::div(context, mk(right.ent(), 0), preview_cfg);
  ts::preview_art(context, preview.ent(), selected);

  auto caption_cfg =
      round_rules::panel_config(round_rules::orchid, "map_caption_panel");
  caption_cfg.with_size(ComponentSize{percent(1.f), ui::h720(84.f)});
  auto caption = imm::vstack(context, mk(right.ent(), 1), caption_cfg);

  imm::div(context, mk(caption.ent(), 0),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), ui::h720(24.f)})
               .with_label(ts::track_name(selected))
               .with_transparent_bg()
               .with_custom_text_color(cs::butter)
               .with_alignment(TextAlignment::Left)
               .with_font_size(15.f)
               .with_skip_tabbing(true)
               .with_debug_name("map_caption_name"));

  imm::div(context, mk(caption.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), ui::h720(20.f)})
               .with_label(ts::track_blurb(selected))
               .with_transparent_bg()
               .with_custom_text_color(round_rules::body_text)
               .with_alignment(TextAlignment::Left)
               .with_font_size(11.f)
               .with_skip_tabbing(true)
               .with_debug_name("map_caption_blurb"));

  auto bottom =
      imm::hstack(context, mk(content.ent(), 3),
                  ComponentConfig{}
                      .with_size(ComponentSize{percent(1.f), ui::h720(48.f)})
                      .with_align_items(AlignItems::Center)
                      .with_transparent_bg()
                      .with_no_wrap()
                      .with_debug_name("map_bottom_bar"));

  const auto chrome_button = [&](int idx, const std::string &label,
                                 Color fill, Color text,
                                 const char *debug_name) {
    auto config = ComponentConfig{}
                      .with_size(ComponentSize{ui::w1280(190.f),
                                               ui::h720(42.f)})
                      .with_label(label)
                      .with_custom_background(fill)
                      .with_custom_text_color(text)
                      .with_border(cs::ink, 3.f)
                      .with_corner_radius(12.f)
                      .with_debug_name(debug_name);
    cs::keep_visuals(config, 15.f);
    animation_control::apply_slide_in(config);
    return static_cast<bool>(
        imm::button(context, mk(bottom.ent(), idx), config));
  };

  if (chrome_button(0,
                    translation_manager::make_translatable_string(
                        strings::i18n::back)
                        .get_text(),
                    cs::well_bg, cs::mint, "btn_back")) {
    navigation::back();
  }

  imm::div(context, mk(bottom.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{expand(), percent(1.f)})
               .with_transparent_bg()
               .with_skip_tabbing(true)
               .with_debug_name("map_bottom_gap"));

  // create_map resolves RANDOM itself, so GO is one path either way.
  if (chrome_button(2, "GO", cs::mint, cs::ink, "btn_go")) {
    maps.create_map();
    GameStateManager::get().start_game();
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::main_screen(Entity &entity,
                                       UIContext<InputAction> &context) {
  auto elem =
      ui_helpers::create_screen_container(context, entity, "main_screen");
  auto top_left = ui_helpers::create_top_left_container(context, elem.ent(),
                                                        "main_top_left", 0);

  // Title lockup. The game has never said its own name on screen.
  // Two lines so CHAOS can take the accent colour on its own.
  const auto title_line = [&](int idx, const char *text, float px,
                              Theme::Usage color, const char *name) {
    imm::div(context, mk(top_left.ent(), idx),
             ComponentConfig{}
                 .with_label(text)
                 .with_font_size(px)
                 .with_size(ComponentSize{pixels(560.f), pixels(px * 1.10f)})
                 .with_text_color(color)
                 .with_alignment(TextAlignment::Left)
                 .with_transparent_bg()
                 .with_skip_tabbing(true)
                 .with_debug_name(name));
  };
  title_line(90, "CART", 78.f, Theme::Usage::Accent, "title_cart");
  title_line(91, "CHAOS", 78.f, Theme::Usage::Primary, "title_chaos");
  title_line(92, "BATTLE MODE FOR 2-8 PLAYERS", 15.f, Theme::Usage::Secondary,
             "title_tagline");
  // Breathing room between the lockup and the menu.
  imm::div(context, mk(top_left.ent(), 93),
           ComponentConfig{}
               .with_size(ComponentSize{pixels(560.f), pixels(28.f)})
               .with_transparent_bg()
               .with_skip_tabbing(true)
               .with_debug_name("title_spacer"));

  // Play button
  ui_helpers::create_styled_button(
      context, top_left.ent(),
      translation_manager::make_translatable_string(strings::i18n::play)
          .get_text(),
      []() { navigation::to(GameStateManager::Screen::CharacterCreation); }, 0, "btn_play");

  // About button
  ui_helpers::create_styled_button(
      context, top_left.ent(),
      translation_manager::make_translatable_string(strings::i18n::about)
          .get_text(),
      []() { navigation::to(GameStateManager::Screen::About); }, 1, "btn_howtoplay");

  // Settings button
  ui_helpers::create_styled_button(
      context, top_left.ent(),
      translation_manager::make_translatable_string(strings::i18n::settings)
          .get_text(),
      []() { navigation::to(GameStateManager::Screen::Settings); }, 2, "btn_options");

  // Exit button
  ui_helpers::create_styled_button(
      context, top_left.ent(),
      translation_manager::make_translatable_string(strings::i18n::exit)
          .get_text(),
      [this]() { exit_game(); }, 3, "btn_quit");

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::settings_screen(Entity &entity,
                                           UIContext<InputAction> &context) {
  auto elem =
      ui_helpers::create_screen_container(context, entity, "settings_screen");
  auto top_left = ui_helpers::create_top_left_container(context, elem.ent(),
                                                        "settings_top_left", 0);
  {
    ui_helpers::create_styled_button(
        context, top_left.ent(),
        translation_manager::make_translatable_string(strings::i18n::back)
            .get_text(),
        []() {
          Settings::update_resolution(
              EntityHelper::get_singleton_cmp<
                  window_manager::ProvidesCurrentResolution>()
                  ->current_resolution);
          navigation::back();
        },
        0, "btn_back");
  }

  // Master volume slider
  {
    float master_volume = Settings::get_master_volume();
    ui_helpers::create_volume_slider(
        context, top_left.ent(),
        translation_manager::make_translatable_string(
            strings::i18n::master_volume)
            .get_text(),
        master_volume,
        [](float volume) { Settings::update_master_volume(volume); }, 0);
  }

  // Music volume slider
  {
    float music_volume = Settings::get_music_volume();
    ui_helpers::create_volume_slider(
        context, top_left.ent(),
        translation_manager::make_translatable_string(
            strings::i18n::music_volume)
            .get_text(),
        music_volume,
        [](float volume) { Settings::update_music_volume(volume); }, 1);
  }

  // SFX volume slider
  {
    float sfx_volume = Settings::get_sfx_volume();
    ui_helpers::create_volume_slider(
        context, top_left.ent(),
        translation_manager::make_translatable_string(strings::i18n::sfx_volume)
            .get_text(),
        sfx_volume, [](float volume) { Settings::update_sfx_volume(volume); },
        2);
  }

  // Resolution dropdown
  {
    // TODO for some reason the dropdown button isnt wiggling
    if (imm::dropdown(
            context, mk(top_left.ent(), 3), resolution_strs, resolution_index,
            ComponentConfig{}
                .with_size(ComponentSize{pixels(400.f), pixels(40.f)})
                .with_label(translation_manager::make_translatable_string(
                                strings::i18n::resolution)
                                .get_text())
                .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                      .left = imm::DefaultSpacing::tiny(),
                                      .bottom = imm::DefaultSpacing::tiny(),
                                      .right = imm::DefaultSpacing::tiny()}))) {
      resolution_provider->on_data_changed(resolution_index);
    }
  }

  // Language dropdown
  {
    static std::vector<std::string> language_names =
        translation_manager::get_available_languages();
    static size_t language_dropdown_index =
        0; // Unique variable for language dropdown
    static translation_manager::Language last_language =
        translation_manager::Language::English;

    // Only update index when language actually changes
    auto current_lang = translation_manager::get_language();
    if (current_lang != last_language) {
      language_dropdown_index =
          translation_manager::get_language_index(current_lang);
      last_language = current_lang;
    }

    if (imm::dropdown(
            context, mk(top_left.ent(), 4), language_names,
            language_dropdown_index,
            ComponentConfig{}
                .with_size(ComponentSize{pixels(400.f), pixels(40.f)})
                .with_label(translation_manager::make_translatable_string(
                                strings::i18n::language)
                                .get_text())
                .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                      .left = imm::DefaultSpacing::tiny(),
                                      .bottom = imm::DefaultSpacing::tiny(),
                                      .right = imm::DefaultSpacing::tiny()}))) {

      auto new_language = translation_manager::Language::English;
      // Update language when selection changes
      switch (language_dropdown_index) {
      case 0:
        new_language = translation_manager::Language::English;
        break;
      case 1:
        new_language = translation_manager::Language::Korean;
        break;
      case 2:
        new_language = translation_manager::Language::Japanese;
        break;
      default:
        // This will cause a compilation error if we add a new language without
        // updating this switch
        static_assert(magic_enum::enum_count<translation_manager::Language>() ==
                          3,
                      "Add new language case to this switch statement");
        break;
      }

      translation_manager::set_language(new_language);
      Settings::set_language(new_language);
      Settings::write_save_file();

      auto &styling_defaults = afterhours::ui::imm::UIStylingDefaults::get();
      auto font_name =
          get_font_name(translation_manager::get_font_for_language());
      styling_defaults.set_default_font(font_name, 16.f);
    }
  }

  // Fullscreen checkbox
  if (imm::checkbox(
          context, mk(top_left.ent(), 5), Settings::get_fullscreen_enabled(),
          ComponentConfig{}
              .with_size(ComponentSize{pixels(400.f), pixels(40.f)})
              .with_label(translation_manager::make_translatable_string(
                              strings::i18n::fullscreen)
                              .get_text())
              .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                    .left = imm::DefaultSpacing::tiny(),
                                    .bottom = imm::DefaultSpacing::tiny(),
                                    .right = imm::DefaultSpacing::tiny()}))) {
    Settings::toggle_fullscreen();
  }

  // Post Processing checkbox
  if (imm::checkbox(
          context, mk(top_left.ent(), 6),
          Settings::get_post_processing_enabled(),
          ComponentConfig{}
              .with_size(ComponentSize{pixels(400.f), pixels(40.f)})
              .with_label(translation_manager::make_translatable_string(
                              strings::i18n::post_processing)
                              .get_text())
              .with_padding(Padding{.top = imm::DefaultSpacing::tiny(),
                                    .left = imm::DefaultSpacing::tiny(),
                                    .bottom = imm::DefaultSpacing::tiny(),
                                    .right = imm::DefaultSpacing::tiny()}))) {
    Settings::toggle_post_processing();
  }

  // leave control group without the back button now that it's top-left

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::about_screen(Entity &entity,
                                        UIContext<InputAction> &context) {
  if (!current_resolution_provider)
    return GameStateManager::get().active_screen;

  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("about_screen"));

  {
    auto top_left = ui_helpers::create_top_left_container(context, elem.ent(),
                                                          "about_top_left", 0);
    ui_helpers::create_styled_button(
        context, top_left.ent(),
        translation_manager::make_translatable_string(strings::i18n::back)
            .get_text(),
        []() { navigation::back(); }, 0, "btn_back");
  }

  // Clear the top-left back button, which shares this origin.
  auto control_group =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_padding(Padding{.top = pixels(104.f),
                                         .left = pixels(28.f)})
                   .with_absolute_position()
                   .with_debug_name("control_group"));

  // This screen used to be a back button and three spritesheet icons, with no
  // text at all. The controls only existed in HOW_TO_PLAY.md, where no player
  // will ever look. English-only for now -- see TODO.
  const auto line = [&](int idx, const char *text, float px,
                        Theme::Usage color, const char *name) {
    imm::div(context, mk(control_group.ent(), idx),
             ComponentConfig{}
                 .with_label(text)
                 .with_font_size(px)
                 .with_size(ComponentSize{pixels(900.f), pixels(px * 1.6f)})
                 .with_text_color(color)
                 .with_alignment(TextAlignment::Left)
                 .with_transparent_bg()
                 .with_skip_tabbing(true)
                 .with_debug_name(name));
  };

  line(10, "SMASH THEM. DON'T GET SMASHED.", 30.f, Theme::Usage::Accent,
       "about_hook");
  line(11, "EIGHT KARTS  //  ONE ARENA  //  FAR TOO MANY WEAPONS", 16.f,
       Theme::Usage::Secondary, "about_sub");
  line(12, " ", 10.f, Theme::Usage::Font, "about_gap1");

  line(20, "FOUR WAYS TO PLAY", 18.f, Theme::Usage::Accent, "about_modes_hd");
  line(21, "SURVIVAL     DON'T BLOW UP", 15.f, Theme::Usage::Font, "about_m1");
  line(22, "MOST KILLS   BLOW UP EVERYONE ELSE", 15.f, Theme::Usage::Font,
       "about_m2");
  line(23, "HIPPO GRAB   HOOVER UP THE PICKUPS", 15.f, Theme::Usage::Font,
       "about_m3");
  line(24, "TAG          DON'T BE \"IT\"", 15.f, Theme::Usage::Font,
       "about_m4");
  line(25, " ", 10.f, Theme::Usage::Font, "about_gap2");

  line(30, "CONTROLS", 18.f, Theme::Usage::Accent, "about_ctrl_hd");
  line(31, "STICK / ARROWS     DRIVE", 15.f, Theme::Usage::Font, "about_c1");
  line(32, "RT / SPACE         BOOST", 15.f, Theme::Usage::Font, "about_c2");
  line(33, "LB & RB / Q & E    SHOOT LEFT & RIGHT", 15.f, Theme::Usage::Font,
       "about_c3");
  line(34, "RS / H             HONK", 15.f, Theme::Usage::Font, "about_c4");
  line(35, "START / ESC        PAUSE", 15.f, Theme::Usage::Font, "about_c5");

  // back button moved to top-left
  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

void ScheduleMainMenuUI::render_team_column_results(
    UIContext<InputAction> &context, Entity &parent,
    const std::string &team_name, int team_id,
    const std::vector<OptEntity> &team_players, int team_score) {
  // Determine team color
  auto team_color =
      team_id == 0
          ? afterhours::ui::imm::ThemeDefaults::get().get_theme().from_usage(
                Theme::Usage::Primary) // Light blue for Team A
          : afterhours::ui::imm::ThemeDefaults::get().get_theme().from_usage(
                Theme::Usage::Accent); // Light orange for Team B

  // Create team column
  auto team_column =
      imm::vstack(context, mk(parent, team_id),
               ComponentConfig{}
                   .with_size(ComponentSize{percent(0.5f), percent(1.f)})
                   .with_custom_background(team_color)
                   .disable_rounded_corners()
                   .with_debug_name(team_name + "_column"));

  imm::div(context, mk(team_column.ent()),
           ComponentConfig{}
               .with_label(team_name)
               .with_size(ComponentSize{percent(1.f), percent(0.15f)})
               .with_debug_name(team_name + "_header"));

  // Team score
  imm::div(context, mk(team_column.ent()),
           ComponentConfig{}
               .with_label("Score: " + std::to_string(team_score))
               .with_size(ComponentSize{percent(1.f), percent(0.1f)})
               .with_debug_name(team_name + "_score"));

  // Team players
  for (size_t i = 0; i < team_players.size(); i++) {
    const auto &player = team_players[i];
    if (player.has_value()) {
      std::string player_name = "Player" + std::to_string(i + 1);
      if (player->has<PlayerID>()) {
        player_name = "Player" + std::to_string(player->get<PlayerID>().id);
      } else if (player->has<AIControlled>()) {
        player_name = "AI" + std::to_string(i + 1);
      }

      imm::div(
          context, mk(team_column.ent(), i),
          ComponentConfig{}
              .with_label(player_name)
              .with_size(ComponentSize{percent(1.f), percent(0.1f)})
              .with_debug_name(team_name + "_player_" + std::to_string(i)));
    }
  }
}

void ScheduleMainMenuUI::render_team_results(
    UIContext<InputAction> &context, Entity &parent,
    const std::vector<OptEntity> &round_players,
    const std::vector<OptEntity> &round_ais) {
  // Group players by team
  std::map<int, std::vector<OptEntity>> team_groups;

  // Group players by team
  for (const auto &player : round_players) {
    if (player.has_value()) {
      int team_id = -1; // Default to no team
      if (player->has<TeamID>()) {
        team_id = player->get<TeamID>().team_id;
      }
      team_groups[team_id].push_back(player);
    }
  }

  // Group AIs by team
  for (const auto &ai : round_ais) {
    if (ai.has_value()) {
      int team_id = -1; // Default to no team
      if (ai->has<TeamID>()) {
        team_id = ai->get<TeamID>().team_id;
      }
      team_groups[team_id].push_back(ai);
    }
  }

  // Calculate team scores
  std::map<int, int> team_scores;
  for (const auto &[team_id, players] : team_groups) {
    int total_score = 0;
    for (const auto &player : players) {
      if (player.has_value()) {
        // Calculate score based on game mode
        if (RoundManager::get().active_round_type == RoundType::Hippo) {
          if (player->has<HasHippoCollection>()) {
            total_score += player->get<HasHippoCollection>().get_hippo_count();
          }
        } else if (RoundManager::get().active_round_type == RoundType::Kills) {
          if (player->has<HasKillCountTracker>()) {
            total_score += player->get<HasKillCountTracker>().kills;
          }
        } else if (RoundManager::get().active_round_type == RoundType::Lives) {
          if (player->has<HasMultipleLives>()) {
            total_score += player->get<HasMultipleLives>().num_lives_remaining;
          }
        } else if (RoundManager::get().active_round_type ==
                   RoundType::TagAndGo) {
          if (player->has<HasTagAndGoTracking>()) {
            total_score += static_cast<int>(
                player->get<HasTagAndGoTracking>().time_as_not_it);
          }
        }
      }
    }
    team_scores[team_id] = total_score;
  }

  // Create two-column layout
  auto team_container =
      imm::hstack(context, mk(parent),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(0.6f), screen_pct(0.6f)})
                   .with_absolute_position(screen_pct(0.2f), screen_pct(0.2f))
                   .with_debug_name("team_results_container"));

  render_team_column_results(context, team_container.ent(), "TEAM A", 0,
                             team_groups[0], team_scores[0]);
  render_team_column_results(context, team_container.ent(), "TEAM B", 1,
                             team_groups[1], team_scores[1]);
}

Screen ScheduleMainMenuUI::round_end_screen(Entity &entity,
                                            UIContext<InputAction> &context) {
  auto elem =
      ui_helpers::create_screen_container(context, entity, "round_end_screen");
  auto top_left = ui_helpers::create_top_left_container(
      context, elem.ent(), "round_end_top_left", 0);
  // Get players from the round (filter out entities marked for cleanup)
  std::vector<OptEntity> round_players;
  std::vector<OptEntity> round_ais;

  try {
    auto round_players_ref =
        EQ(EntityQuery<EQ>::QueryOptions{.ignore_temp_warning = true})
            .whereHasComponent<PlayerID>()
            .orderByPlayerID()
            .gen();
    for (const auto &player_ref : round_players_ref) {
      if (!player_ref.get().cleanup) {
        round_players.push_back(OptEntity{player_ref.get()});
      }
    }
  } catch (...) {
    // If query fails, just continue with empty list
  }

  try {
    auto round_ais_ref =
        EQ(EntityQuery<EQ>::QueryOptions{.ignore_temp_warning = true})
            .whereHasComponent<AIControlled>()
            .gen();
    for (const auto &ai_ref : round_ais_ref) {
      if (!ai_ref.get().cleanup) {
        round_ais.push_back(OptEntity{ai_ref.get()});
      }
    }
  } catch (...) {
    // If query fails, just continue with empty list
  }

  // Title
  {
    imm::div(context, mk(elem.ent()),
             ComponentConfig{}
                 .with_label(translation_manager::make_translatable_string(
                                 strings::i18n::round_end)
                                 .get_text())
                 .with_skip_tabbing(true)
                 .with_size(ComponentSize{percent(0.5f), percent(0.2f)})
                 .with_margin(Margin{
                     .top = screen_pct(0.05f),
                     .left = screen_pct(0.2f),
                 })
                 .with_debug_name("round_end_title"));
  }

  // Check if team mode is enabled
  auto &settings = RoundManager::get().get_active_settings();
  if (settings.team_mode_enabled) {
    // Render team results in two columns
    render_team_results(context, elem.ent(), round_players, round_ais);
  } else {
    // Render individual results in grid layout
    std::map<EntityID, int> rankings;
    if (RoundManager::get().active_round_type == RoundType::TagAndGo) {
      rankings = get_tag_and_go_rankings(round_players, round_ais);
    }

    size_t num_slots = round_players.size() + round_ais.size();
    if (num_slots > 0) {
      int fours =
          static_cast<int>(std::ceil(static_cast<float>(num_slots) / 4.f));

      auto player_group = imm::div(
          context, mk(elem.ent()),
          ComponentConfig{}
              .with_size(ComponentSize{screen_pct(0.7f), screen_pct(fours == 1 ? 0.7f : 0.85f)})
              .with_absolute_position(screen_pct(0.2f), screen_pct(fours == 1 ? 0.3f : 0.15f))
              .with_debug_name("player_group"));

      for (int row_id = 0; row_id < fours; row_id++) {
        auto row = imm::hstack(
            context, mk(player_group.ent(), row_id),
            ComponentConfig{}
                .with_size(ComponentSize{percent(1.f), percent(0.5f, 0.4f)})
                .with_debug_name("row"));
        size_t start = row_id * 4;
        for (size_t i = start; i < std::min(num_slots, start + 4); i++) {
          OptEntity car;
          if (i < round_players.size()) {
            car = round_players[i];
          } else {
            car = round_ais[i - round_players.size()];
          }

          std::optional<int> ranking;
          if (car.has_value() &&
              RoundManager::get().active_round_type == RoundType::TagAndGo) {
            auto it = rankings.find(car->id);
            if (it != rankings.end() && it->second <= 3) {
              ranking = it->second;
            }
          }

          round_end_player_column(row.ent(), context, i, round_players,
                                  round_ais, ranking);
        }
      }
    }
  }

  ui_helpers::create_styled_button(
      context, top_left.ent(),
      translation_manager::make_translatable_string(
          strings::i18n::continue_game)
          .get_text(),
      []() { navigation::to(GameStateManager::Screen::CharacterCreation); }, 0);
  ui_helpers::create_styled_button(
      context, top_left.ent(),
      translation_manager::make_translatable_string(strings::i18n::quit)
          .get_text(),
      [this]() { exit_game(); }, 1);

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

void register_ui_systems(afterhours::SystemManager &systems) {
  ui::register_before_ui_updates<InputAction>(systems);
  {
    systems.register_update_system(
        std::make_unique<SetupGameStylingDefaults>());

    afterhours::animation::register_update_systems<UIKey>(systems);
    afterhours::animation::register_update_systems<
        afterhours::animation::CompositeKey>(systems);
    systems.register_update_system(
        std::make_unique<ui_game::UpdateUIWiggle<InputAction>>());
    systems.register_update_system(std::make_unique<NavigationSystem>());
    systems.register_update_system(std::make_unique<ScheduleMainMenuUI>());
    systems.register_update_system(std::make_unique<SchedulePauseUI>());
    systems.register_update_system(std::make_unique<ScheduleDebugUI>());
  }
  ui::register_after_ui_updates<InputAction>(systems);
  systems.register_update_system(
      std::make_unique<ui_game::ApplyInitialSlideInMask<InputAction>>());
  systems.register_update_system(
      std::make_unique<ui_game::UpdateUISlideIn<InputAction>>());
}