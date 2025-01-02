
#pragma once

#include "components.h"
#include "query.h"

// TODO at the moment we are half imm mode and have retained
// at some point we should transition to fully imm
struct UISystem : System<ui::AutoLayoutRoot> {
  bool init = false;
  Entity *screen_ptr;

  virtual void once(float) override {
    if (!init || screen_ptr == nullptr)
      init_screen();
  }

  void init_screen() {
    auto &entity = EntityQuery()
                       .whereHasComponent<ui::AutoLayoutRoot>()
                       .gen_first_enforce();
    setup_ui(entity);
    init = true;
  }

  virtual ~UISystem() {}
  virtual void setup_ui(Entity &) = 0;
};

struct RenderMainMenuUI : UISystem {
  RenderMainMenuUI() : UISystem() {}
  virtual ~RenderMainMenuUI() {}

  Entity &setup_settings(Entity &root) {
    using afterhours::ui::children;
    using afterhours::ui::ComponentSize;
    using afterhours::ui::make_button;
    using afterhours::ui::make_div;
    using afterhours::ui::Padding;
    using afterhours::ui::padding_;
    using afterhours::ui::pixels;
    using afterhours::ui::screen_pct;

    auto &screen = EntityHelper::createEntity();
    screen_ptr = &screen;
    {
      screen.addComponent<ui::UIComponentDebug>("settings_screen");
      screen.addComponent<ui::UIComponent>(screen.id)
          .set_desired_width(ui::Size{
              .dim = ui::Dim::ScreenPercent,
              .value = 1.f,
          })
          .set_desired_height(
              ui::Size{.dim = ui::Dim::ScreenPercent, .value = 1.f})
          .set_parent(root)
          .make_absolute();
    }

    auto &div = make_div(screen, {screen_pct(1.f, 1.f), screen_pct(1.f, 1.f)});

    auto &controls = make_div(div, afterhours::ui::children_xy(),
                              Padding{
                                  .top = screen_pct(0.4f),
                                  .left = screen_pct(0.4f),
                                  .bottom = pixels(0.f),
                                  .right = pixels(0.f),
                              });
    controls.addComponent<ui::UIComponentDebug>("controls_group");

    auto &dropdown =
        ui::make_dropdown<window_manager::ProvidesAvailableWindowResolutions>(
            controls);
    dropdown.get<ui::UIComponent>()
        .set_desired_width(ui::Size{
            .dim = ui::Dim::Pixels,
            .value = button_size.x,
        })
        .set_desired_height(ui::Size{
            .dim = ui::Dim::Children,
            .value = button_size.y,
        });
    dropdown.get<ui::HasChildrenComponent>().register_on_child_add(
        [](Entity &child) {
          if (child.is_missing<ui::HasColor>()) {
            child.addComponent<ui::HasColor>(raylib::PURPLE);
          }
        });

    screen.get<ui::UIComponent>().should_hide = true;
    return screen;
  }

  virtual void setup_ui(Entity &root) override {
    using afterhours::ui::children;
    using afterhours::ui::ComponentSize;
    using afterhours::ui::make_button;
    using afterhours::ui::make_div;
    using afterhours::ui::Padding;
    using afterhours::ui::padding_;
    using afterhours::ui::pixels;
    using afterhours::ui::screen_pct;

    auto &settings_root = setup_settings(root);

    auto &screen = EntityHelper::createEntity();
    screen_ptr = &screen;
    {
      screen.addComponent<ui::UIComponentDebug>("main_screen");
      screen.addComponent<ui::UIComponent>(screen.id)
          .set_desired_width(ui::Size{
              .dim = ui::Dim::ScreenPercent,
              .value = 1.f,
          })
          .set_desired_height(
              ui::Size{.dim = ui::Dim::ScreenPercent, .value = 1.f})
          .set_parent(root)
          .make_absolute();
    }

    auto &div = make_div(screen, {screen_pct(1.f, 1.f), screen_pct(1.f, 1.f)});

    auto &buttons = make_div(div, afterhours::ui::children_xy(),
                             Padding{
                                 .top = screen_pct(0.4f),
                                 .left = screen_pct(0.4f),
                                 .bottom = pixels(0.f),
                                 .right = pixels(0.f),
                             });
    buttons.addComponent<ui::UIComponentDebug>("button_group");

    {
      Padding button_padding = {
          .top = pixels(button_size.y / 10.f),
          .left = pixels(0.f),
          .bottom = pixels(button_size.y / 10.f),
          .right = pixels(0.f),
      };

      const auto close_menu = [&div](Entity &) {
        div.get<ui::UIComponent>().should_hide = true;
      };

      const auto open_settings = [&](Entity &) {
        div.get<ui::UIComponent>().should_hide = true;
        settings_root.get<ui::UIComponent>().should_hide = false;
      };
      make_button(buttons, "play", button_size, close_menu, button_padding);
      make_button(buttons, "about", button_size, close_menu, button_padding);
      make_button(buttons, "settings", button_size, open_settings,
                  button_padding);
      make_button(
          buttons, "exit", button_size, [&](Entity &) { running = false; },
          button_padding);
    }
  }
};

using namespace afterhours::ui;
using namespace afterhours::ui::imm;
struct ScheduleDebugUI : System<afterhours::ui::UIContext<InputAction>> {
  bool enabled = false;
  float enableCooldown = 0.f;
  float enableCooldownReset = 0.2f;

  virtual bool should_run(float dt) override {
    enableCooldown -= dt;

    if (enableCooldown < 0) {
      enableCooldown = enableCooldownReset;
      input::PossibleInputCollector<InputAction> inpc =
          input::get_input_collector<InputAction>();
      for (auto &actions_done : inpc.inputs()) {
        if (actions_done.action == InputAction::ToggleUIDebug) {
          enabled = !enabled;
          break;
        }
      }
    }
    return enabled;
  }

  virtual void for_each_with(Entity &entity, UIContext<InputAction> &context,
                             float) override {

    if (!enabled) {
      return;
    }

    auto elem = imm::div(context, mk(entity));
    elem.ent()
        .get<UIComponent>()
        // TODO use config once it exists
        .set_flex_direction(FlexDirection::Row);

    // Max speed
    {
      auto max_speed_label =
          fmt::format("Max Speed\n {:.2f} m/s", config.max_speed.data);
      float pct = config.max_speed.get_pct();
      if (auto result = slider(context, mk(elem.ent()), pct, max_speed_label);
          result) {
        config.max_speed.set_pct(result.as<float>());
      }
    }

    // Skid Threshold
    {
      auto label =
          fmt::format("Skid Threshold \n {:.2f} %", config.skid_threshold.data);
      float pct = config.skid_threshold.get_pct();
      if (auto result = slider(context, mk(elem.ent()), pct, label); result) {
        config.skid_threshold.set_pct(result.as<float>());
      }
    }

    // Steering Sensitivity
    {
      auto label = fmt::format("Steering Sensitivity \n {:.2f} %",
                               config.steering_sensitivity.data);
      float pct = config.steering_sensitivity.get_pct();
      if (auto result = slider(context, mk(elem.ent()), pct, label); result) {
        config.steering_sensitivity.set_pct(result.as<float>());
      }
    }
  }
};
