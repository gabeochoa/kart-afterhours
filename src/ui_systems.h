
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

struct RenderDebugUI : UISystem {
  bool enabled = true;
  float enableCooldown = 0.f;
  float enableCooldownReset = 0.2f;

  RenderDebugUI() : UISystem() {}
  virtual ~RenderDebugUI() {}

  virtual bool should_run(float dt) override {
    enableCooldown -= dt;

    if (enableCooldown < 0) {
      enableCooldown = enableCooldownReset;
      input::PossibleInputCollector<InputAction> inpc =
          input::get_input_collector<InputAction>();
      for (auto &actions_done : inpc.inputs()) {
        if (actions_done.action == InputAction::ToggleUIDebug) {
          enabled = !enabled;
          screen_ptr->get<ui::UIComponent>().should_hide = enabled;
          break;
        }
      }
    }
    return enabled;
  }

  virtual void setup_ui(Entity &root) override {
    using afterhours::ui::children_xy;
    using afterhours::ui::FlexDirection;
    using afterhours::ui::make_div;
    using afterhours::ui::make_slider;
    using afterhours::ui::pixels;
    using afterhours::ui::SliderConfig;

    // TODO just replace all of these with autolayout roots....
    auto &screen = EntityHelper::createEntity();
    screen_ptr = &screen;
    {
      // making a root component to attach the UI to
      screen.addComponent<ui::UIComponentDebug>("debug_screen");
      screen.addComponent<ui::UIComponent>(screen.id)
          .set_desired_width(afterhours::ui::screen_pct(1.f))
          .set_desired_height(afterhours::ui::screen_pct(1.f))
          .set_parent(root)
          .make_absolute();
    }

    auto &div = make_div(screen, children_xy());
    div.get<ui::UIComponent>().flex_direction = FlexDirection::Row;
    {

      make_slider(div, SliderConfig{
                           .size =
                               vec2{
                                   button_size.x * 2.f,
                                   button_size.y / 2.f,
                               },
                           .starting_pct = config.max_speed.get_pct(),
                           .on_slider_changed =
                               [&](const float pct) {
                                 config.max_speed.set_pct(pct);

                                 // TODO id love for this to move into the
                                 // actual make_slider config but we dont yet
                                 // have a way to convert from pct back to value
                                 // inside the ui.h
                                 return std::to_string(config.max_speed.data);
                               },
                           .label = "Max Speed\n {} m/s",
                           .label_is_format_string = true,
                       });

      make_slider(div, SliderConfig{
                           .size =
                               vec2{
                                   button_size.x * 2.f,
                                   button_size.y / 2.f,
                               },
                           .starting_pct = config.max_speed.get_pct(),
                           .on_slider_changed =
                               [&](const float pct) {
                                 config.max_speed.set_pct(pct);

                                 // TODO id love for this to move into the
                                 // actual make_slider config but we dont yet
                                 // have a way to convert from pct back to value
                                 // inside the ui.h
                                 return std::to_string(config.max_speed.data);
                               },
                           .label = "Max Speed\n {} m/s",
                           .label_is_format_string = true,
                       });

      make_slider(div,
                  SliderConfig{
                      .size =
                          vec2{
                              button_size.x * 2.f,
                              button_size.y / 2.f,
                          },
                      .starting_pct = config.skid_threshold.get_pct(),
                      .on_slider_changed =
                          [&](const float pct) {
                            config.skid_threshold.set_pct(pct);

                            // TODO id love for this to move into the
                            // actual make_slider config but we dont yet
                            // have a way to convert from pct back to value
                            // inside the ui.h
                            return std::to_string(config.skid_threshold.data);
                          },
                      .label = "Skid Threshold\n {}%",
                      .label_is_format_string = true,
                  });

      make_slider(div,
                  SliderConfig{
                      .size =
                          vec2{
                              button_size.x * 2.f,
                              button_size.y / 2.f,
                          },
                      .starting_pct = config.steering_sensitivity.get_pct(),
                      .on_slider_changed =
                          [&](const float pct) {
                            config.steering_sensitivity.set_pct(pct);

                            // TODO id love for this to move into the
                            // actual make_slider config but we dont yet
                            // have a way to convert from pct back to value
                            // inside the ui.h
                            return std::to_string(
                                config.steering_sensitivity.data);
                          },
                      .label = "Steering Sensitivity\n {}%",
                      .label_is_format_string = true,
                  });
    }
  }
};
