

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

// with_absolute_position resolves BOTH axes against the screen height
// (component_init.h:441), so an absolute x written as a screen_pct lands at
// pct * height. Spelling it in h720 units is what makes it mean what it reads.
// See docs/afterhours_gaps.md.
inline afterhours::ui::Size absolute_x(float px_at_720p) {
  return afterhours::ui::h720(px_at_720p);
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
  // visible.
  //
  // Paused counts too. OPTIONS on the pause card puts a menu screen up over a
  // live round, and this is the only system that applies a queued screen
  // change -- gated on menu state alone, navigating from the card would hang
  // on a next_screen nothing ever reads. While paused with no screen up the
  // switch below has nothing to draw.
  auto *nav = EntityHelper::get_singleton_cmp<MenuNavigationStack>();
  return GameStateManager::get().is_paused() ||
         (GameStateManager::get().is_menu_active() &&
          (nav ? nav->ui_visible : true));
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

inline std::string driver_name(size_t seat, bool is_ai) {
  if (is_ai)
    return bot_names[seat % bot_names.size()];
  return fmt::format("PLAYER {}", seat + 1);
}

inline std::string driver_name(const Slot &slot) {
  return driver_name(slot.seat, slot.is_ai);
}

// The screen-chrome button: flat fill, ink keyline, no rounding beyond the
// radius. Every bottom bar in the game is made of these.
struct ChromeButton {
  std::string label;
  Color fill;
  Color text;
  std::string debug_name;
  Size width = ui::w1280(150.f);
  Size height = ui::h720(42.f);
  float font_px = 15.f;
};

inline bool chrome_button(UIContext<InputAction> &context, Entity &parent,
                          int index, const ChromeButton &spec) {
  auto config = ComponentConfig{}
                    .with_size(ComponentSize{spec.width, spec.height})
                    .with_label(spec.label)
                    .with_custom_background(spec.fill)
                    .with_custom_text_color(spec.text)
                    .with_border(ink, 3.f)
                    .with_corner_radius(12.f)
                    .with_debug_name(spec.debug_name);
  keep_visuals(config, spec.font_px);
  animation_control::apply_slide_in(config);
  return static_cast<bool>(imm::button(context, mk(parent, index), config));
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

// A kart in a driver's paint, sized to fit whatever rect it lands in.
// imm::sprite always tints white (see docs/afterhours_gaps.md), so the tinted
// draw goes through the custom-draw escape hatch instead. Null before the
// spritesheet singleton exists.
inline std::function<void(RectangleType)> kart_sprite(Color tint) {
  auto *sheet_cmp = EntityHelper::get_singleton_cmp<
      afterhours::texture_manager::HasSpritesheet>();
  if (!sheet_cmp)
    return nullptr;

  const auto sheet = sheet_cmp->texture;
  const auto src = afterhours::texture_manager::idx_to_sprite_frame(0, 1);
  return [sheet, src, tint](RectangleType r) {
    const float side = std::min(r.width, r.height) * 0.86f;
    const RectangleType dest{r.x + (r.width - side) * 0.5f,
                             r.y + (r.height - side) * 0.5f, side, side};
    raylib::DrawTexturePro(sheet, src, dest, raylib::Vector2{0.f, 0.f}, 0.f,
                           tint);
  };
}

// The kart the player will drive, as a button that cycles their paint.
inline void kart_portrait(UIContext<InputAction> &context, Entity &card,
                          int slot_index, Color tint,
                          const std::function<void()> &on_click) {
  auto config = ComponentConfig{}
                    .with_size(ComponentSize{percent(1.f), expand()})
                    .with_custom_background(well_bg)
                    .with_custom_hover_bg(panel_bg)
                    .with_corner_radius(10.f)
                    .with_debug_name(fmt::format("slot_{}_kart", slot_index));
  keep_visuals(config, 12.f);

  if (auto sprite = kart_sprite(tint))
    config.with_on_draw_fg(sprite);

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
                             .with_gap(pixels(3.f))
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

// What the middle slot of a "MODE // ? // ..." pill says. Lives has no clock,
// so it states the stake it does have.
inline std::string clock_or_lives() {
  auto &manager = RoundManager::get();
  if (manager.uses_timer())
    return time_option_label(manager.get_active_settings().time_option);
  const int lives = manager.fetch_num_starting_lives();
  return fmt::format("{} {}", lives, lives == 1 ? "LIFE" : "LIVES");
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
        // Offset by the corner radius. Drawn at the corner itself the tick sits
        // in the area the rounded border cuts away, so it reads as a detached
        // mark floating beside the panel rather than an accent on its edge.
        constexpr float inset = 16.f;
        raylib::DrawRectangleRec(RectangleType{r.x + inset, r.y, 16.f, 4.f},
                                 cs::butter);
        raylib::DrawRectangleRec(RectangleType{r.x, r.y + inset, 4.f, 16.f},
                                 cs::butter);
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
                              .with_gap(pixels(8.f))
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
  return fmt::format("{} // {} // {}P",
                     rr::mode_name(RoundManager::get().active_round_type),
                     rr::clock_or_lives(), driver_count);
}

inline std::string track_name(int map_index) {
  if (map_index == MapManager::RANDOM_MAP_INDEX)
    return "RANDOM";
  return MapManager::available_maps[static_cast<size_t>(map_index)]
      .display_name;
}

// The tile is labelled RANDOM; the caption spells it out so a highlighted
// RANDOM tile cannot read as "a track called RANDOM is already chosen".
inline std::string caption_title(int map_index) {
  if (map_index == MapManager::RANDOM_MAP_INDEX)
    return "RANDOM TRACK";
  return track_name(map_index);
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
          // Corners on, radius zero. disable_rounded_corners() leaves
          // HasRoundedCorners off the entity entirely, and the focus ring then
          // falls back to the theme's rounded corners -- a rounded cream ring
          // inside a square butter border.
          .with_rounded_corners(RoundedCorners{})
          .with_corner_radius(0.f)
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
               .with_size(ComponentSize{percent(1.f), ui::h720(20.f)})
               .with_label(track_name(map_index))
               .with_transparent_bg()
               .with_custom_text_color(on ? cs::butter : cs::muted)
               .with_font_size(13.f)
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
        160.f);
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

// ----------------------------------------------------------------------------
// Options ("OPTIONS") -- see docs/ui-mock.html section 05.
//
// Every control here used to be a stock imm widget, and between them they
// produced all 2461 layout warnings left in the e2e suite. imm::slider is the
// worst of it: it lays a half-width label beside a full-width track inside a
// full-width parent, so every volume row overflowed itself by half its width
// at any configured size, and inherit_from copies typography only, so the
// track and handle take theme colours a caller cannot override.
//
// So the meter is drawn by hand and made interactive with the same two
// listener components imm::slider attaches, and the toggles and steppers are
// the ones round_rules already uses. See docs/afterhours_gaps.md.
//
// Determinism: no clock, no rand(). The resolution list is the window
// manager's own order (one entry in headless) and the language list is
// magic_enum's enum order.
// ----------------------------------------------------------------------------
namespace options {

namespace cs = character_select;
namespace rr = round_rules;
using afterhours::Color;

using VolumeGet = float (*)();
using VolumeSet = void (*)(float);

// mint at 55% over well_bg, precomputed: the stripe has to be opaque so the
// scissor below is the only thing deciding where the fill ends.
constexpr Color mint_stripe{53, 123, 114, 255};

inline void draw_meter(RectangleType r, float value) {
  constexpr float track_h = 20.f;
  constexpr float band = 8.f;
  constexpr float knob_w = 18.f;

  const RectangleType track{r.x, r.y + (r.height - track_h) * 0.5f, r.width,
                            track_h};
  raylib::DrawRectangleRounded(track, 1.f, 8, cs::well_bg);

  const float fill_w = track.width * value;
  if (fill_w > 1.f) {
    const RectangleType fill{track.x, track.y, fill_w, track.height};
    raylib::DrawRectangleRounded(fill, 1.f, 8, cs::mint);
    // Clipped to the fill rather than just drawn over it: an unclipped band is
    // how the old fill came to bleed past the end of its track.
    begin_scissor_mode(static_cast<int>(fill.x), static_cast<int>(fill.y),
                       static_cast<int>(fill.width),
                       static_cast<int>(fill.height));
    for (float x = fill.x; x < fill.x + fill.width; x += band * 2.f)
      raylib::DrawRectangleRec(RectangleType{x, fill.y, band, fill.height},
                               mint_stripe);
    end_scissor_mode();
  }

  raylib::DrawRectangleRoundedLinesEx(track, 1.f, 8, 3.f, cs::sky);

  const RectangleType knob{track.x + fill_w - knob_w * 0.5f, r.y, knob_w,
                           r.height};
  raylib::DrawRectangleRec(knob, cs::butter);
  raylib::DrawRectangleLinesEx(knob, 3.f, cs::ink);
}

inline void volume_row(UIContext<InputAction> &context, Entity &parent,
                       int index, strings::i18n label_key, VolumeGet get,
                       VolumeSet set, const std::string &debug_name) {
  auto row = rr::settings_row(context, parent, index, rr::text_for(label_key),
                              debug_name);

  const float value = get();
  auto config =
      ComponentConfig{}
          .with_size(ComponentSize{expand(), ui::h720(26.f)})
          .with_padding(Padding{})
          .with_transparent_bg()
          .with_corner_radius(14.f)
          .with_on_draw_fg([value](RectangleType r) { draw_meter(r, value); })
          .with_debug_name(debug_name + "_meter");
  cs::keep_visuals(config, 12.f);

  // A button rather than a div only for the focus ring -- a bare div with a
  // drag listener is focusable but draws nothing to say so. The two listeners
  // below are the same ones imm::slider attaches to its track.
  Entity &meter = imm::button(context, mk(row.ent(), 1), config).ent();
  meter.addComponentIfMissing<HasDragListener>([set](Entity &self) {
    const RectangleType r = self.get<UIComponent>().rect();
    set(std::clamp((input::get_mouse_position().x - r.x) / r.width, 0.f, 1.f));
  });
  meter.addComponentIfMissing<HasLeftRightListener>(
      [get, set](Entity &, int dir) {
        set(std::clamp(get() + static_cast<float>(dir) * 0.05f, 0.f, 1.f));
      });

  imm::div(context, mk(row.ent(), 2),
           ComponentConfig{}
               .with_size(ComponentSize{ui::w1280(50.f), percent(1.f)})
               .with_label(fmt::format(
                   "{}", static_cast<int>(std::lround(value * 100.f))))
               .with_transparent_bg()
               .with_custom_text_color(cs::butter)
               .with_alignment(TextAlignment::Right)
               .with_font_size(13.f)
               .with_skip_tabbing(true)
               .with_debug_name(debug_name + "_value"));
}

inline void panel_header(UIContext<InputAction> &context, Entity &panel,
                         const char *label, const char *debug_name) {
  imm::div(context, mk(panel, 0),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), ui::h720(20.f)})
               .with_label(label)
               .with_transparent_bg()
               .with_custom_text_color(cs::butter)
               .with_alignment(TextAlignment::Left)
               .with_font_size(12.f)
               .with_skip_tabbing(true)
               .with_debug_name(debug_name));
}

inline void sound_panel(UIContext<InputAction> &context, Entity &parent) {
  auto panel = imm::vstack(context, mk(parent, 0),
                           rr::panel_config(cs::mint, "sound_panel"));

  panel_header(context, panel.ent(), "SOUND", "sound_panel_header");
  volume_row(context, panel.ent(), 1, strings::i18n::master_volume,
             &Settings::get_master_volume, &Settings::update_master_volume,
             "row_master");
  volume_row(context, panel.ent(), 2, strings::i18n::music_volume,
             &Settings::get_music_volume, &Settings::update_music_volume,
             "row_music");
  volume_row(context, panel.ent(), 3, strings::i18n::sfx_volume,
             &Settings::get_sfx_volume, &Settings::update_sfx_volume,
             "row_effects");
}

} // namespace options

// ----------------------------------------------------------------------------
// Results ("<DRIVER> WINS") -- see docs/ui-mock.html section 07.
//
// A podium: each driver's kart above a bar whose height is their score, tallest
// first, the winner's bar in the butter stripe. It replaces a row of coloured
// cards each holding a sentence of numbers, which said who scored what but not
// who won.
//
// The mock also asks for static confetti. That is already on screen and cost
// nothing: RenderMenuBackdrop (src/systems/menu_backdrop.h) draws its Memphis
// geometry behind every menu screen, and RoundEnd is one.
//
// Determinism: no clock, no rand(). Scores come off the cars the round left
// behind and ties break on entity id, so the order is the same every run.
// ----------------------------------------------------------------------------
namespace results {

namespace cs = character_select;
namespace rr = round_rules;
namespace ts = track_select;
using afterhours::Color;

// The mock's own bars: shortest 30, winner 132. A zero score still gets
// bar_min so the kart above it has something to stand on and the row keeps a
// common baseline.
constexpr float bar_min = 30.f;
constexpr float bar_max = 132.f;

// butter with the mock's 12% black over it, precomputed: the stripe has to be
// opaque, because it is painted by the same bg hook as the fill under it.
constexpr Color butter_stripe{211, 204, 81, 255};

// What the bar measures, per mode. The team totals sum the same call, so this
// screen has one definition of "score".
inline int driver_score(const Entity &car) {
  switch (RoundManager::get().active_round_type) {
  case RoundType::Lives:
    return car.has<HasMultipleLives>()
               ? car.get<HasMultipleLives>().num_lives_remaining
               : 0;
  case RoundType::Kills:
    return car.has<HasKillCountTracker>()
               ? car.get<HasKillCountTracker>().kills
               : 0;
  case RoundType::Hippo:
    return car.has<HasHippoCollection>()
               ? car.get<HasHippoCollection>().get_hippo_count()
               : 0;
  case RoundType::TagAndGo:
    return car.has<HasTagAndGoTracking>()
               ? static_cast<int>(
                     car.get<HasTagAndGoTracking>().time_as_not_it)
               : 0;
  }
  return 0;
}

struct Finisher {
  EntityID id{0};
  std::string name;
  Color paint;
  int score{0};
  int team{-1};
};

inline std::vector<Finisher>
standings(const std::vector<OptEntity> &round_players,
          const std::vector<OptEntity> &round_ais) {
  std::vector<Finisher> board;
  board.reserve(round_players.size() + round_ais.size());

  const auto add = [&board](const OptEntity &opt, size_t seat, bool is_ai) {
    if (!opt.has_value())
      return;
    const Entity &car = opt.asE();
    board.push_back({car.id, cs::driver_name(seat, is_ai),
                     car.get<HasColor>().color(), driver_score(car),
                     car.has<TeamID>() ? car.get<TeamID>().team_id : -1});
  };

  for (size_t i = 0; i < round_players.size(); i++)
    add(round_players[i], i, false);
  for (size_t i = 0; i < round_ais.size(); i++)
    add(round_ais[i], i, true);

  // Entity id breaks ties so two drivers on the same score never swap columns
  // between frames.
  std::ranges::sort(board, [](const Finisher &a, const Finisher &b) {
    return a.score != b.score ? a.score > b.score : a.id < b.id;
  });
  return board;
}

inline int team_score(const std::vector<Finisher> &board, int team) {
  int total = 0;
  for (const Finisher &f : board)
    if (f.team == team)
      total += f.score;
  return total;
}

inline std::string headline(const std::vector<Finisher> &board,
                            bool team_mode) {
  if (team_mode) {
    const int a = team_score(board, 0);
    const int b = team_score(board, 1);
    if (a == b)
      return "DEAD HEAT";
    return a > b ? "TEAM A WINS" : "TEAM B WINS";
  }
  // A round where nobody scored has no winner to name, and claiming one on a
  // score of zero reads as a bug.
  if (board.empty() || board[0].score == 0)
    return "NOBODY WINS";
  if (board.size() > 1 && board[1].score == board[0].score)
    return "DEAD HEAT";
  return board[0].name + " WINS";
}

// Restates the rules so a screenshot of this screen explains itself.
inline std::string rules_pill_text() {
  return fmt::format("{} // {} // {}",
                     rr::mode_name(RoundManager::get().active_round_type),
                     rr::clock_or_lives(),
                     ts::track_name(MapManager::get().get_selected_map()));
}

inline void podium_column(UIContext<InputAction> &context, Entity &row,
                          int place, const Finisher &finisher, int top_score,
                          bool is_winner) {
  const std::string dbg = fmt::format("podium_{}", place);

  // +1 leaves index 0 for the leading shoulder, so sibling index order and
  // creation order agree.
  auto column =
      imm::vstack(context, mk(row, place + 1),
                  ComponentConfig{}
                      .with_size(ComponentSize{expand(), percent(1.f)})
                      .with_padding(Padding{.left = ui::w1280(8.f),
                                            .right = ui::w1280(8.f)})
                      .with_transparent_bg()
                      .with_debug_name(dbg));

  // Bars are read against a shared floor, so the column packs from the bottom:
  // everything above the kart is one expanding spacer.
  imm::div(context, mk(column.ent(), 0),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), expand()})
               .with_transparent_bg()
               .with_skip_tabbing(true)
               .with_debug_name(dbg + "_gap"));

  auto kart = ComponentConfig{}
                  .with_size(ComponentSize{percent(1.f), ui::h720(64.f)})
                  .with_transparent_bg()
                  .with_skip_tabbing(true)
                  .with_debug_name(dbg + "_kart");
  if (auto sprite = cs::kart_sprite(finisher.paint))
    kart.with_on_draw_fg(sprite);
  imm::div(context, mk(column.ent(), 1), kart);

  imm::div(context, mk(column.ent(), 2),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), ui::h720(18.f)})
               .with_label(finisher.name)
               .with_transparent_bg()
               .with_custom_text_color(is_winner ? cs::butter : cs::muted)
               .with_font_size(11.f)
               .with_skip_tabbing(true)
               .with_debug_name(dbg + "_name"));

  float grow = 1.0f;
  if (!animation_control::disabled()) {
    afterhours::animation::one_shot(UIKey::RoundEndScore, place, [](auto h) {
      h.from(0.0f).to(1.0f, 0.8f,
                      afterhours::animation::EasingType::EaseOutQuad);
    });
    grow = afterhours::animation::clamp_value(UIKey::RoundEndScore, place, 0.0f,
                                              1.0f);
  }

  const float share = top_score > 0 ? static_cast<float>(finisher.score) /
                                          static_cast<float>(top_score)
                                    : 0.f;
  const float height = bar_min + share * (bar_max - bar_min) * grow;
  const int shown = static_cast<int>(std::round(grow * finisher.score));

  imm::div(
      context, mk(column.ent(), 3),
      ComponentConfig{}
          .with_size(ComponentSize{percent(1.f), pixels(height)})
          .with_label(std::to_string(shown))
          // The fill goes in the bg hook, not with_custom_background: the
          // widget's own fill would paint over it, and the fg hook draws after
          // the label, which would bury the score.
          .with_transparent_bg()
          .with_custom_text_color(cs::ink)
          .with_border(cs::ink, 3.f)
          .with_rounded_corners(RoundedCorners{})
          .with_corner_radius(0.f)
          .with_alignment(TextAlignment::Center)
          .with_font_size(24.f)
          .with_skip_tabbing(true)
          .with_on_draw_bg([is_winner](RectangleType r) {
            raylib::DrawRectangleRec(r, is_winner ? cs::butter : cs::sky);
            if (!is_winner)
              return;
            begin_scissor_mode(static_cast<int>(r.x), static_cast<int>(r.y),
                               static_cast<int>(r.width),
                               static_cast<int>(r.height));
            for (float x = r.x; x < r.x + r.width; x += 16.f)
              raylib::DrawRectangleRec(RectangleType{x, r.y, 8.f, r.height},
                                       butter_stripe);
            end_scissor_mode();
          })
          .with_debug_name(dbg + "_bar"));
}

} // namespace results

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
                      .with_absolute_position(ui_helpers::absolute_x(64.f),
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

  if (cs::chrome_button(context, bottom.ent(), 0,
                        {.label = translation_manager::
                             make_translatable_string(strings::i18n::back)
                                 .get_text(),
                         .fill = cs::well_bg,
                         .text = cs::mint,
                         .debug_name = "btn_back"})) {
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

  if (cs::chrome_button(context, bottom.ent(), 3,
                        {.label = "NEXT",
                         .fill = cs::butter,
                         .text = cs::ink,
                         .debug_name = "btn_next"})) {
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

// ----------------------------------------------------------------------------
// Pause ("TIME OUT") -- see docs/ui-mock.html section 08.
//
// A bordered card over a dimmed arena, rather than three menu buttons drawn
// straight onto live gameplay. The card says what you paused and how much of
// it is left.
//
// The scrim is a flat colour, not with_opacity(): opacity on a transparent
// element paints black (see docs/afterhours_gaps.md).
// ----------------------------------------------------------------------------
namespace pause_menu {

namespace cs = character_select;
namespace rr = round_rules;
using afterhours::Color;

// The mock's rgba(20,10,48,.8), precomputed.
constexpr Color scrim{20, 10, 48, 204};

inline std::string status_line() {
  auto &manager = RoundManager::get();
  const std::string mode = rr::mode_name(manager.active_round_type);
  if (!manager.uses_timer())
    return fmt::format("{} // {}", mode, rr::clock_or_lives());

  const int left =
      std::max(0, static_cast<int>(manager.get_current_round_time()));
  return fmt::format("{} // {}:{:02} LEFT", mode, left / 60, left % 60);
}

} // namespace pause_menu

void SchedulePauseUI::for_each_with(Entity &entity,
                                    UIContext<InputAction> &context, float) {
  namespace pm = pause_menu;
  namespace cs = character_select;

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

  imm::div(context, mk(entity),
           ComponentConfig{}
               .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
               .with_absolute_position()
               .with_custom_background(pm::scrim)
               .with_debug_name("pause_scrim"));

  // OPTIONS hands the screen to ScheduleMainMenuUI, which renders after this
  // system and so draws over the scrim. The card would fight it, so it steps
  // aside until the screen is dismissed.
  if (GameStateManager::get().active_screen != Screen::None)
    return;

  auto card =
      imm::vstack(context, mk(entity, 1),
                  ComponentConfig{}
                      .with_size(ComponentSize{ui::w1280(400.f),
                                               ui::h720(304.f)})
                      .with_absolute_position(ui_helpers::absolute_x(440.f),
                                              ui::h720(208.f))
                      .with_custom_background(cs::panel_bg)
                      .with_border(cs::butter, 3.f)
                      .with_corner_radius(16.f)
                      .with_gap(pixels(12.f))
                      .with_padding(Padding{.top = ui::h720(24.f),
                                            .left = ui::w1280(24.f),
                                            .bottom = ui::h720(24.f),
                                            .right = ui::w1280(24.f)})
                      .with_debug_name("pause_card"));

  imm::div(context, mk(card.ent(), 0),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), ui::h720(52.f)})
               .with_label("TIME OUT")
               .with_font_size(40.f)
               .with_alignment(TextAlignment::Center)
               .with_transparent_bg()
               .with_custom_text_color(cs::butter)
               .with_skip_tabbing(true)
               .with_debug_name("pause_heading"));

  imm::div(context, mk(card.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), ui::h720(22.f)})
               .with_label(pm::status_line())
               .with_font_size(11.f)
               .with_alignment(TextAlignment::Center)
               .with_transparent_bg()
               .with_custom_text_color(cs::muted)
               .with_skip_tabbing(true)
               .with_debug_name("pause_status"));

  if (cs::chrome_button(context, card.ent(), 2,
                        {.label = "BACK TO IT",
                         .fill = cs::butter,
                         .text = cs::ink,
                         .debug_name = "btn_back_to_it",
                         .width = percent(1.f),
                         .height = ui::h720(44.f)})) {
    GameStateManager::get().unpause_game();
  }

  if (cs::chrome_button(context, card.ent(), 3,
                        {.label = translation_manager::
                             make_translatable_string(strings::i18n::settings)
                                 .get_text(),
                         .fill = cs::mint,
                         .text = cs::ink,
                         .debug_name = "btn_pause_options",
                         .width = percent(1.f),
                         .height = ui::h720(44.f)})) {
    navigation::to(GameStateManager::Screen::Settings);
  }

  // Ends the round rather than the process. Quitting the game lives on the
  // main menu, which this is now two clicks from.
  if (cs::chrome_button(context, card.ent(), 4,
                        {.label = "I GIVE UP",
                         .fill = cs::well_bg,
                         .text = cs::mint,
                         .debug_name = "btn_give_up",
                         .width = percent(1.f),
                         .height = ui::h720(44.f)})) {
    GameStateManager::get().end_game();
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
                      .with_absolute_position(ui_helpers::absolute_x(64.f),
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
                      .with_gap(pixels(16.f))
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

  const Size wide = ui::w1280(190.f);

  if (cs::chrome_button(context, bottom.ent(), 0,
                        {.label = translation_manager::
                             make_translatable_string(strings::i18n::back)
                                 .get_text(),
                         .fill = cs::well_bg,
                         .text = cs::mint,
                         .debug_name = "btn_back",
                         .width = wide})) {
    navigation::back();
  }

  imm::div(context, mk(bottom.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{expand(), percent(1.f)})
               .with_transparent_bg()
               .with_skip_tabbing(true)
               .with_debug_name("round_settings_bottom_gap"));

  if (cs::chrome_button(context, bottom.ent(), 2,
                        {.label = translation_manager::
                             make_translatable_string(strings::i18n::select_map)
                                 .get_text(),
                         .fill = cs::butter,
                         .text = cs::ink,
                         .debug_name = "btn_select_map",
                         .width = wide})) {
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
                      .with_absolute_position(ui_helpers::absolute_x(64.f),
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
                              .with_gap(pixels(24.f))
                              .with_margin(Margin{.top = ui::h720(12.f),
                                                  .bottom = ui::h720(12.f)})
                              .with_transparent_bg()
                              .with_no_wrap()
                              .with_debug_name("map_body"));

  auto grid = imm::vstack(context, mk(body.ent(), 0),
                          ComponentConfig{}
                              .with_size(ComponentSize{ui::w1280(396.f),
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
                               .with_gap(pixels(14.f))
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
               .with_label(ts::caption_title(selected))
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

  const Size wide = ui::w1280(190.f);

  if (cs::chrome_button(context, bottom.ent(), 0,
                        {.label = translation_manager::
                             make_translatable_string(strings::i18n::back)
                                 .get_text(),
                         .fill = cs::well_bg,
                         .text = cs::mint,
                         .debug_name = "btn_back",
                         .width = wide})) {
    navigation::back();
  }

  imm::div(context, mk(bottom.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{expand(), percent(1.f)})
               .with_transparent_bg()
               .with_skip_tabbing(true)
               .with_debug_name("map_bottom_gap"));

  // create_map resolves RANDOM itself, so GO is one path either way.
  if (cs::chrome_button(context, bottom.ent(), 2,
                        {.label = "GO",
                         .fill = cs::mint,
                         .text = cs::ink,
                         .debug_name = "btn_go",
                         .width = wide})) {
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
  namespace cs = character_select;
  namespace rr = round_rules;
  namespace op = options;

  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("settings_screen"));

  auto content =
      imm::vstack(context, mk(elem.ent()),
                  ComponentConfig{}
                      .with_size(ComponentSize{screen_pct(0.90f, 1.f),
                                               screen_pct(0.86f, 1.f)})
                      .with_absolute_position(ui_helpers::absolute_x(64.f),
                                              screen_pct(0.07f))
                      .with_transparent_bg()
                      .with_debug_name("settings_content"));

  imm::div(context, mk(content.ent(), 0),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), ui::h720(48.f)})
               .with_label(translation_manager::make_translatable_string(
                               strings::i18n::settings)
                               .get_text())
               .with_font_size(34.f)
               .with_alignment(TextAlignment::Left)
               .with_transparent_bg()
               .with_skip_tabbing(true)
               .with_debug_name("settings_heading"));

  imm::div(context, mk(content.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{ui::w1280(160.f), ui::h720(8.f)})
               .with_custom_background(cs::butter)
               .with_skip_tabbing(true)
               .with_debug_name("settings_rule"));

  auto panels =
      imm::hstack(context, mk(content.ent(), 2),
                  ComponentConfig{}
                      // Tall enough for the four SCREEN rows and no taller:
                      // the mock's panels are sized to their contents, with
                      // the bottom bar pushed down by the gap below them.
                      .with_size(ComponentSize{percent(1.f), ui::h720(204.f)})
                      .with_gap(pixels(16.f))
                      .with_margin(Margin{.top = ui::h720(12.f),
                                          .bottom = ui::h720(12.f)})
                      .with_transparent_bg()
                      .with_no_wrap()
                      .with_debug_name("settings_panels"));

  op::sound_panel(context, panels.ent());

  auto screen_panel =
      imm::vstack(context, mk(panels.ent(), 1),
                  rr::panel_config(rr::orchid, "screen_panel"));

  op::panel_header(context, screen_panel.ent(), "SCREEN",
                   "screen_panel_header");

  if (!resolution_strs.empty() &&
      rr::stepper_row(context, screen_panel.ent(), 1,
                      rr::text_for(strings::i18n::resolution),
                      resolution_strs, resolution_index, "row_size")) {
    // Null in headless: update_resolution_cache fabricates the one entry
    // rather than asking a window manager that isn't there.
    if (resolution_provider)
      resolution_provider->on_data_changed(resolution_index);
  }

  bool fullscreen = Settings::get_fullscreen_enabled();
  if (rr::toggle_row(context, screen_panel.ent(), 2,
                     rr::text_for(strings::i18n::fullscreen), fullscreen,
                     "row_fullscreen")) {
    // Not the flipped local copy: toggle_fullscreen also asks the window to
    // change, which is the half a bare bool cannot do.
    Settings::toggle_fullscreen();
  }

  rr::toggle_row(context, screen_panel.ent(), 3,
                 rr::text_for(strings::i18n::post_processing),
                 Settings::get_post_processing_enabled(), "row_filter");

  {
    static const std::vector<std::string> language_names =
        translation_manager::get_available_languages();
    size_t index = translation_manager::get_language_index(
        translation_manager::get_language());

    if (rr::stepper_row(context, screen_panel.ent(), 4,
                        rr::text_for(strings::i18n::language), language_names,
                        index, "row_language")) {
      const auto language =
          magic_enum::enum_value<translation_manager::Language>(index);
      translation_manager::set_language(language);
      Settings::set_language(language);

      auto &styling_defaults = afterhours::ui::imm::UIStylingDefaults::get();
      styling_defaults.set_default_font(
          get_font_name(translation_manager::get_font_for_language()), 16.f);
    }
  }

  imm::div(context, mk(content.ent(), 3),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), expand()})
               .with_transparent_bg()
               .with_skip_tabbing(true)
               .with_debug_name("settings_filler"));

  auto bottom =
      imm::hstack(context, mk(content.ent(), 4),
                  ComponentConfig{}
                      .with_size(ComponentSize{percent(1.f), ui::h720(48.f)})
                      .with_align_items(AlignItems::Center)
                      .with_transparent_bg()
                      .with_no_wrap()
                      .with_debug_name("settings_bottom_bar"));

  auto back = ComponentConfig{}
                  .with_size(ComponentSize{ui::w1280(150.f), ui::h720(42.f)})
                  .with_label(translation_manager::make_translatable_string(
                                  strings::i18n::back)
                                  .get_text())
                  .with_custom_background(cs::well_bg)
                  .with_custom_text_color(cs::mint)
                  .with_border(cs::ink, 3.f)
                  .with_corner_radius(12.f)
                  .with_debug_name("btn_back");
  cs::keep_visuals(back, 15.f);
  animation_control::apply_slide_in(back);

  if (imm::button(context, mk(bottom.ent(), 0), back)) {
    Settings::update_resolution(
        EntityHelper::get_singleton_cmp<
            window_manager::ProvidesCurrentResolution>()
            ->current_resolution);
    navigation::back();
  }

  // True as of the once-a-second Settings::save_if_changed poll in main.cpp.
  imm::div(context, mk(bottom.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{expand(), percent(1.f)})
               .with_label("SAVED AUTOMATICALLY")
               .with_transparent_bg()
               .with_custom_text_color(cs::mint)
               .with_alignment(TextAlignment::Right)
               .with_font_size(12.f)
               .with_skip_tabbing(true)
               .with_debug_name("settings_saved_note"));

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

Screen ScheduleMainMenuUI::round_end_screen(Entity &entity,
                                            UIContext<InputAction> &context) {
  namespace rs = results;
  namespace cs = character_select;

  // Cars marked for cleanup are the ones the round destroyed; they are still
  // in the world for a frame or two and would show up as ghost columns.
  std::vector<OptEntity> round_players;
  std::vector<OptEntity> round_ais;
  for (Entity &player : EQ(EntityQuery<EQ>::QueryOptions{
                               .ignore_temp_warning = true})
                            .whereHasComponent<PlayerID>()
                            .orderByPlayerID()
                            .gen())
    if (!player.cleanup)
      round_players.push_back(OptEntity{player});
  for (Entity &ai : EQ(EntityQuery<EQ>::QueryOptions{.ignore_temp_warning =
                                                         true})
                        .whereHasComponent<AIControlled>()
                        .gen())
    if (!ai.cleanup)
      round_ais.push_back(OptEntity{ai});

  const bool team_mode =
      RoundManager::get().get_active_settings().team_mode_enabled;
  const std::vector<rs::Finisher> board =
      rs::standings(round_players, round_ais);
  const int top_score = board.empty() ? 0 : board.front().score;

  auto elem =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_absolute_position()
                   .with_debug_name("round_end"));

  auto content =
      imm::vstack(context, mk(elem.ent()),
                  ComponentConfig{}
                      .with_size(ComponentSize{screen_pct(0.90f, 1.f),
                                               screen_pct(0.86f, 1.f)})
                      .with_absolute_position(ui_helpers::absolute_x(64.f),
                                              screen_pct(0.07f))
                      .with_transparent_bg()
                      .with_debug_name("round_end_content"));

  imm::div(context, mk(content.ent(), 0),
           ComponentConfig{}
               .with_size(ComponentSize{percent(1.f), ui::h720(56.f)})
               .with_label(rs::headline(board, team_mode))
               .with_font_size(44.f)
               .with_alignment(TextAlignment::Center)
               .with_transparent_bg()
               .with_custom_text_color(cs::mint)
               .with_skip_tabbing(true)
               .with_debug_name("round_end_headline"));

  // Centred by a spacer either side. A vstack's align_items is untested here
  // and this is one line longer and certain.
  auto pill_row =
      imm::hstack(context, mk(content.ent(), 1),
                  ComponentConfig{}
                      .with_size(ComponentSize{percent(1.f), ui::h720(30.f)})
                      .with_gap(pixels(10.f))
                      .with_transparent_bg()
                      .with_no_wrap()
                      .with_debug_name("round_end_pill_row"));

  const auto pill_gap = [&](int idx) {
    imm::div(context, mk(pill_row.ent(), idx),
             ComponentConfig{}
                 .with_size(ComponentSize{expand(), percent(1.f)})
                 .with_transparent_bg()
                 .with_skip_tabbing(true)
                 .with_debug_name(fmt::format("round_end_pill_gap_{}", idx)));
  };

  pill_gap(0);
  imm::div(context, mk(pill_row.ent(), 1),
           ComponentConfig{}
               .with_size(ComponentSize{ui::w1280(340.f), percent(1.f)})
               .with_label(rs::rules_pill_text())
               .with_custom_background(cs::well_bg)
               .with_custom_text_color(cs::butter)
               .with_corner_radius(15.f)
               .with_font_size(12.f)
               .with_skip_tabbing(true)
               .with_debug_name("round_end_rules_pill"));

  // Team mode has no podium of its own: the bars are still per driver, and
  // this is the only thing the old two-column layout said that they do not.
  if (team_mode)
    imm::div(context, mk(pill_row.ent(), 2),
             ComponentConfig{}
                 .with_size(ComponentSize{ui::w1280(260.f), percent(1.f)})
                 .with_label(fmt::format("TEAM A {} // TEAM B {}",
                                         rs::team_score(board, 0),
                                         rs::team_score(board, 1)))
                 .with_custom_background(cs::well_bg)
                 .with_custom_text_color(cs::mint)
                 .with_corner_radius(15.f)
                 .with_font_size(12.f)
                 .with_skip_tabbing(true)
                 .with_debug_name("round_end_team_pill"));
  pill_gap(3);

  auto podium =
      imm::hstack(context, mk(content.ent(), 2),
                  ComponentConfig{}
                      .with_size(ComponentSize{percent(1.f), expand()})
                      // Column padding rather than a row gap: the shoulders
                      // below are percentages that have to sum to exactly the
                      // row, and a gap is width the percentages cannot see.
                      // No horizontal margin either -- a percent(1.f) row keeps
                      // its full width and a left margin just slides it off the
                      // right edge (measured: x=98 w=1152 inside a 1152 box).
                      .with_margin(Margin{.top = ui::h720(12.f),
                                          .bottom = ui::h720(12.f)})
                      .with_transparent_bg()
                      .with_no_wrap()
                      .with_debug_name("round_end_podium"));

  // The mock's podium is a four-column grid whatever the field size. Columns
  // expand into what is left, so a short field is centred by padding it out to
  // four with a spacer on each side rather than by stretching one driver's bar
  // across the whole screen.
  const float shoulder =
      board.size() < 4
          ? (4.f - static_cast<float>(board.size())) / 8.f
          : 0.f;
  const auto podium_shoulder = [&](int idx) {
    if (shoulder <= 0.f)
      return;
    imm::div(context, mk(podium.ent(), idx),
             ComponentConfig{}
                 .with_size(ComponentSize{percent(shoulder), percent(1.f)})
                 .with_transparent_bg()
                 .with_skip_tabbing(true)
                 .with_debug_name(fmt::format("podium_shoulder_{}", idx)));
  };

  podium_shoulder(0);
  for (size_t place = 0; place < board.size(); place++)
    rs::podium_column(context, podium.ent(), static_cast<int>(place),
                      board[place], top_score,
                      !team_mode && place == 0 && top_score > 0);
  podium_shoulder(static_cast<int>(board.size()) + 1);

  auto bottom =
      imm::hstack(context, mk(content.ent(), 3),
                  ComponentConfig{}
                      .with_size(ComponentSize{percent(1.f), ui::h720(64.f)})
                      .with_align_items(AlignItems::Center)
                      .with_transparent_bg()
                      .with_no_wrap()
                      .with_debug_name("round_end_bottom_bar"));

  if (cs::chrome_button(context, bottom.ent(), 0,
                        {.label = "MAIN MENU",
                         .fill = cs::well_bg,
                         .text = cs::mint,
                         .debug_name = "btn_main_menu"})) {
    navigation::to(GameStateManager::Screen::Main);
  }

  if (cs::chrome_button(context, bottom.ent(), 1,
                        {.label = "CHANGE RULES",
                         .fill = cs::sky,
                         .text = cs::ink,
                         .debug_name = "btn_change_rules",
                         .width = ui::w1280(170.f)})) {
    navigation::to(GameStateManager::Screen::RoundSettings);
  }

  imm::div(context, mk(bottom.ent(), 2),
           ComponentConfig{}
               .with_size(ComponentSize{expand(), percent(1.f)})
               .with_transparent_bg()
               .with_skip_tabbing(true)
               .with_debug_name("round_end_bottom_gap"));

  if (cs::chrome_button(context, bottom.ent(), 3,
                        {.label = "RUN IT BACK",
                         .fill = cs::butter,
                         .text = cs::ink,
                         .debug_name = "btn_run_it_back",
                         .width = ui::w1280(250.f),
                         .height = ui::h720(54.f),
                         .font_px = 22.f})) {
    // Same rules, same drivers, same track -- but a rebuilt arena. Without
    // create_map the hippos picked up last round are still gone.
    MapManager::get().create_map();
    GameStateManager::get().start_game();
  }

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
    // Pause first: its scrim has to be built before whatever screen OPTIONS
    // puts on top of it, and the UI draws in the order the tree was built.
    systems.register_update_system(std::make_unique<SchedulePauseUI>());
    systems.register_update_system(std::make_unique<ScheduleMainMenuUI>());
    systems.register_update_system(std::make_unique<ScheduleDebugUI>());
  }
  ui::register_after_ui_updates<InputAction>(systems);
  systems.register_update_system(
      std::make_unique<ui_game::ApplyInitialSlideInMask<InputAction>>());
  systems.register_update_system(
      std::make_unique<ui_game::UpdateUISlideIn<InputAction>>());
}