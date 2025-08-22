
#pragma once

#include "rl.h"

enum class InputAction {
  None,
  Accel,
  Left,
  Right,
  Brake,
  Boost,
  ShootLeft,
  ShootRight,
  //
  WidgetRight,
  WidgetLeft,
  WidgetNext,
  WidgetPress,
  WidgetMod,
  WidgetBack,
  MenuBack,
  PauseButton,
  ToggleUIDebug,
  ToggleUILayoutDebug,
  Honk,
};

inline int to_int(InputAction action) {
  return static_cast<int>(action);
}

inline InputAction from_int(int value) {
  return static_cast<InputAction>(value);
}

inline bool action_matches(int action, InputAction expected) {
  return from_int(action) == expected;
}

using afterhours::input;

inline auto get_mapping() {
  std::map<int, input::ValidInputs> mapping;

  mapping[to_int(InputAction::Accel)] = {
      raylib::KEY_UP,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_RIGHT_Y,
          .dir = -1,
      },
  };

  mapping[to_int(InputAction::Brake)] = {
      raylib::KEY_DOWN,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_RIGHT_Y,
          .dir = 1,
      },
  };

  mapping[to_int(InputAction::Left)] = {
      raylib::KEY_LEFT,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_LEFT_X,
          .dir = -1,
      },
  };

  mapping[to_int(InputAction::Right)] = {
      raylib::KEY_RIGHT,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_LEFT_X,
          .dir = 1,
      },
  };

  mapping[to_int(InputAction::ShootLeft)] = {
      raylib::KEY_Q,
      raylib::GAMEPAD_BUTTON_LEFT_TRIGGER_1,
  };

  mapping[to_int(InputAction::ShootRight)] = {
      raylib::KEY_E,
      raylib::GAMEPAD_BUTTON_RIGHT_TRIGGER_1,
  };

  mapping[to_int(InputAction::WidgetLeft)] = {
      raylib::KEY_LEFT,
      raylib::GAMEPAD_BUTTON_LEFT_FACE_LEFT,
  };

  mapping[to_int(InputAction::WidgetRight)] = {
      raylib::KEY_RIGHT,
      raylib::GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
  };

  mapping[to_int(InputAction::WidgetBack)] = {
      raylib::GAMEPAD_BUTTON_LEFT_FACE_UP,
      raylib::KEY_UP,
  };

  mapping[to_int(InputAction::WidgetNext)] = {
      raylib::KEY_TAB,
      raylib::GAMEPAD_BUTTON_LEFT_FACE_DOWN,
      raylib::KEY_DOWN,
  };

  mapping[to_int(InputAction::WidgetPress)] = {
      raylib::KEY_ENTER,
      raylib::GAMEPAD_BUTTON_RIGHT_FACE_DOWN,
  };

  mapping[to_int(InputAction::WidgetMod)] = {
      raylib::KEY_LEFT_SHIFT,
  };

  mapping[to_int(InputAction::MenuBack)] = {
      raylib::KEY_ESCAPE,
  };

  mapping[to_int(InputAction::PauseButton)] = {raylib::KEY_ESCAPE,
                                       raylib::GAMEPAD_BUTTON_MIDDLE_RIGHT};

  mapping[to_int(InputAction::ToggleUIDebug)] = {
      raylib::KEY_GRAVE,
  };

  mapping[to_int(InputAction::ToggleUILayoutDebug)] = {
      raylib::KEY_EQUAL,
  };

  mapping[to_int(InputAction::Boost)] = {
      raylib::KEY_SPACE,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_RIGHT_TRIGGER,
          .dir = 1,
      },
  };

  mapping[to_int(InputAction::Honk)] = {
      raylib::KEY_H,
      raylib::GAMEPAD_BUTTON_RIGHT_THUMB,
  };

  return mapping;
}
