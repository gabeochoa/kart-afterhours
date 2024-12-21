
#pragma once

#include <map>
#include <variant>

#include "../base_component.h"
#include "../developer.h"
#include "../entity_query.h"
#include "../system.h"

namespace afterhours {

struct input : developer::Plugin {

  static constexpr float DEADZONE = 0.25f;
  static constexpr int MAX_GAMEPAD_ID = 8;

  using MouseButton = int;
#ifdef AFTER_HOURS_USE_RAYLIB
  using MousePosition = raylib::Vector2;
  using KeyCode = int;
  using GamepadID = int;
  using GamepadAxis = raylib::GamepadAxis;
  using GamepadButton = raylib::GamepadButton;

  static MousePosition get_mouse_position() {
    return raylib::GetMousePosition();
  }
  static bool is_mouse_button_up(MouseButton button) {
    return raylib::IsMouseButtonUp(button);
  }
  static bool is_mouse_button_down(MouseButton button) {
    return raylib::IsMouseButtonDown(button);
  }
  static bool is_mouse_button_pressed(MouseButton button) {
    return raylib::IsMouseButtonPressed(button);
  }
  static bool is_mouse_button_released(MouseButton button) {
    return raylib::IsMouseButtonReleased(button);
  }
  static bool is_gamepad_available(GamepadID id) {
    return raylib::IsGamepadAvailable(id);
  }
  static bool is_key_pressed(KeyCode keycode) {
    return raylib::IsKeyPressed(keycode);
  }
  static bool is_key_down(KeyCode keycode) {
    return raylib::IsKeyDown(keycode);
  }
  static float get_gamepad_axis_mvt(GamepadID gamepad_id, GamepadAxis axis) {
    return raylib::GetGamepadAxisMovement(gamepad_id, axis);
  }
  static bool is_gamepad_button_pressed(GamepadID gamepad_id,
                                        GamepadButton button) {
    return raylib::IsGamepadButtonPressed(gamepad_id, button);
  }
  static bool is_gamepad_button_down(GamepadID gamepad_id,
                                     GamepadButton button) {
    return raylib::IsGamepadButtonDown(gamepad_id, button);
  }

  static void draw_text(const std::string &text, int x, int y, int fontSize) {
    raylib::DrawText(text.c_str(), x, y, fontSize, raylib::RED);
  }

  static void set_gamepad_mappings(const char *mappings) {
    raylib::SetGamepadMappings(mappings);
  }

#else
  using MousePosition = std::pair<int, int>;
  using KeyCode = int;
  using GamepadID = int;
  using GamepadAxis = int;
  using GamepadButton = int;

  // TODO good luck ;)
  static MousePosition get_mouse_position() { return {0, 0}; }
  static bool is_mouse_button_up(MouseButton) { return false; }
  static bool is_mouse_button_down(MouseButton) { return false; }
  static bool is_mouse_button_pressed(MouseButton) { return false; }
  static bool is_mouse_button_released(MouseButton) { return false; }
  static

      bool
      is_key_pressed(KeyCode) {
    return false;
  }
  static bool is_key_down(KeyCode) { return false; }
  static bool is_gamepad_available(GamepadID) { return false; }
  static float get_gamepad_axis_mvt(GamepadID, GamepadAxis) { return 0.f; }
  static bool is_gamepad_button_pressed(GamepadID, GamepadButton) {
    return false;
  }
  static bool is_gamepad_button_down(GamepadID, GamepadButton) { return false; }
  static void draw_text(const std::string &, int, int, int) {}
  static void set_gamepad_mappings(const char *) {}
#endif

  enum DeviceMedium {
    None,
    Keyboard,
    Gamepad,
  };

  template <typename T> struct ActionDone {
    DeviceMedium medium;
    GamepadID id = -1;
    T action;
    float amount_pressed;
    float length_pressed;
  };

  struct GamepadAxisWithDir {
    GamepadAxis axis;
    int dir = -1;
  };

  using AnyInput = std::variant<KeyCode, GamepadAxisWithDir, GamepadButton>;
  using ValidInputs = std::vector<AnyInput>;

  static float visit_key(int keycode) {
    return is_key_pressed(keycode) ? 1.f : 0.f;
  }
  static float visit_key_down(int keycode) {
    return is_key_down(keycode) ? 1.f : 0.f;
  }

  static float visit_axis(GamepadID id, GamepadAxisWithDir axis_with_dir) {
    // Note: this one is a bit more complex because we have to check if you
    // are pushing in the right direction while also checking the magnitude
    float mvt = get_gamepad_axis_mvt(id, axis_with_dir.axis);
    // Note: The 0.25 is how big the deadzone is
    // TODO consider making the deadzone configurable?
    if (util::sgn(mvt) == axis_with_dir.dir && abs(mvt) > DEADZONE) {
      return abs(mvt);
    }
    return 0.f;
  }

  static float visit_button(GamepadID id, GamepadButton button) {
    return is_gamepad_button_pressed(id, button) ? 1.f : 0.f;
  }

  static float visit_button_down(GamepadID id, GamepadButton button) {
    return is_gamepad_button_down(id, button) ? 1.f : 0.f;
  }

  template <typename Action> struct InputCollector : public BaseComponent {
    std::vector<input::ActionDone<Action>> inputs;
    std::vector<input::ActionDone<Action>> inputs_pressed;
    float since_last_input = 0.f;
  };

  struct ProvidesMaxGamepadID : public BaseComponent {
    int max_gamepad_id_available = 1;
  };

  template <typename Action>
  struct ProvidesInputMapping : public BaseComponent {
    using GameMapping = std::map<Action, input::ValidInputs>;
    GameMapping mapping;
    ProvidesInputMapping(GameMapping start_mapping) : mapping(start_mapping) {}
  };

  struct RenderConnectedGamepads : System<input::ProvidesMaxGamepadID> {

    virtual void for_each_with(const Entity &,
                               const input::ProvidesMaxGamepadID &mxGamepadID,
                               float) const override {
      draw_text(("Gamepads connected: " +
                 std::to_string(mxGamepadID.max_gamepad_id_available + 1)),
                400, 60, 20);
    }
  };

  template <typename Action> struct PossibleInputCollector {
    std::optional<std::reference_wrapper<InputCollector<Action>>> data;
    PossibleInputCollector(
        std::optional<std::reference_wrapper<InputCollector<Action>>> d)
        : data(d) {}
    PossibleInputCollector(InputCollector<Action> &d) : data(d) {}
    PossibleInputCollector() : data({}) {}
    bool has_value() const { return data.has_value(); }
    bool valid() const { return has_value(); }

    [[nodiscard]] std::vector<input::ActionDone<Action>> &inputs() {
      return data.value().get().inputs;
    }

    [[nodiscard]] std::vector<input::ActionDone<Action>> &inputs_pressed() {
      return data.value().get().inputs_pressed;
    }

    [[nodiscard]] float since_last_input() {
      return data.value().get().since_last_input;
    }
  };

  template <typename Action>
  static auto get_input_collector() -> PossibleInputCollector<Action> {

    OptEntity opt_collector =
        EntityQuery().whereHasComponent<InputCollector<Action>>().gen_first();
    if (!opt_collector.valid())
      return {};
    Entity &collector = opt_collector.asE();
    return collector.get<InputCollector<Action>>();
  }

  // TODO i would like to move this out of input namespace
  // at some point
  template <typename Action>
  struct InputSystem : System<InputCollector<Action>, ProvidesMaxGamepadID,
                              ProvidesInputMapping<Action>> {

    int fetch_max_gampad_id() {
      int result = -1;
      int i = 0;
      while (i < ::afterhours::input::MAX_GAMEPAD_ID) {
        bool avail = is_gamepad_available(i);
        if (!avail) {
          result = i - 1;
          break;
        }
        i++;
      }
      return result;
    }

    std::pair<DeviceMedium, float>
    check_single_action_pressed(GamepadID id, input::ValidInputs valid_inputs) {
      DeviceMedium medium;
      float value = 0.f;
      for (auto &input : valid_inputs) {
        DeviceMedium temp_medium = None;
        float temp =             //
            std::visit(          //
                util::overloaded{//
                                 [&](int keycode) {
                                   temp_medium = Keyboard;
                                   return is_key_pressed(keycode) ? 1.f : 0.f;
                                 },
                                 [&](GamepadAxisWithDir axis_with_dir) {
                                   temp_medium = Gamepad;
                                   return visit_axis(id, axis_with_dir);
                                 },
                                 [&](GamepadButton button) {
                                   temp_medium = Gamepad;
                                   return is_gamepad_button_pressed(id, button)
                                              ? 1.f
                                              : 0.f;
                                 },
                                 [](auto) {}},
                input);
        if (temp > value) {
          value = temp;
          medium = temp_medium;
        }
      }
      return {medium, value};
    }

    std::pair<DeviceMedium, float>
    check_single_action_down(GamepadID id, input::ValidInputs valid_inputs) {
      DeviceMedium medium;
      float value = 0.f;
      for (auto &input : valid_inputs) {
        DeviceMedium temp_medium = None;
        float temp =             //
            std::visit(          //
                util::overloaded{//
                                 [&](int keycode) {
                                   temp_medium = Keyboard;
                                   return visit_key_down(keycode);
                                 },
                                 [&](GamepadAxisWithDir axis_with_dir) {
                                   temp_medium = Gamepad;
                                   return visit_axis(id, axis_with_dir);
                                 },
                                 [&](GamepadButton button) {
                                   temp_medium = Gamepad;
                                   return visit_button_down(id, button);
                                 },
                                 [](auto) {}},
                input);
        if (temp > value) {
          value = temp;
          medium = temp_medium;
        }
      }
      return {medium, value};
    }

    virtual void for_each_with(Entity &, InputCollector<Action> &collector,
                               ProvidesMaxGamepadID &mxGamepadID,
                               ProvidesInputMapping<Action> &input_mapper,
                               float dt) override {
      mxGamepadID.max_gamepad_id_available =
          std::max(-1, fetch_max_gampad_id());
      collector.inputs.clear();
      collector.inputs_pressed.clear();

      for (auto &kv : input_mapper.mapping) {
        Action action = kv.first;
        ValidInputs vis = kv.second;

        int i = 0;
        do {
          // down
          {
            auto [medium, amount] = check_single_action_down(i, vis);
            if (amount > 0.f) {
              collector.inputs.push_back(ActionDone{.medium = medium,
                                                    .id = i,
                                                    .action = action,
                                                    .amount_pressed = amount,
                                                    .length_pressed = dt});
            }
          }
          // pressed
          {
            auto [medium, amount] = check_single_action_pressed(i, vis);
            if (amount > 0.f) {
              collector.inputs_pressed.push_back(
                  ActionDone{.medium = medium,
                             .id = i,
                             .action = action,
                             .amount_pressed = amount,
                             .length_pressed = dt});
            }
          }
          i++;
        } while (i <= mxGamepadID.max_gamepad_id_available);
      }

      if (collector.inputs.size() == 0) {
        collector.since_last_input += dt;
      } else {
        collector.since_last_input = 0;
      }
    }
  };

  template <typename Action>
  static void add_singleton_components(
      Entity &entity, typename input::ProvidesInputMapping<Action>::GameMapping
                          inital_mapping) {
    entity.addComponent<InputCollector<Action>>();
    entity.addComponent<input::ProvidesMaxGamepadID>();
    entity.addComponent<input::ProvidesInputMapping<Action>>(inital_mapping);
  }

  template <typename Action> static void enforce_singletons(SystemManager &sm) {
    sm.register_update_system(
        std::make_unique<
            developer::EnforceSingleton<InputCollector<Action>>>());
    sm.register_update_system(
        std::make_unique<developer::EnforceSingleton<ProvidesMaxGamepadID>>());
    sm.register_update_system(
        std::make_unique<
            developer::EnforceSingleton<ProvidesInputMapping<Action>>>());
  }

  template <typename Action>
  static void register_update_systems(SystemManager &sm) {
    sm.register_update_system(
        std::make_unique<afterhours::input::InputSystem<Action>>());
  }

  // Renderer Systems:
  // RenderConnectedGamepads
};
} // namespace afterhours
