
#pragma once

#include "rl.h"

enum class InputAction {
  None,
  Accel,
  Left,
  Right,
  Brake,
  ShootLeft,
  ShootRight,
  //
  WidgetNext,
  WidgetPress,
  WidgetMod,
  WidgetBack,
  ToggleUIDebug,
  ToggleUILayoutDebug,
};

using afterhours::input;

inline auto get_mapping() {
  std::map<InputAction, input::ValidInputs> mapping;

  mapping[InputAction::Accel] = {
      raylib::KEY_UP,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_RIGHT_Y,
          .dir = -1,
      },
  };

  mapping[InputAction::Brake] = {
      raylib::KEY_DOWN,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_RIGHT_Y,
          .dir = 1,
      },
  };

  mapping[InputAction::Left] = {
      raylib::KEY_LEFT,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_LEFT_X,
          .dir = -1,
      },
  };

  mapping[InputAction::Right] = {
      raylib::KEY_RIGHT,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_LEFT_X,
          .dir = 1,
      },
  };

  mapping[InputAction::ShootLeft] = {
      raylib::KEY_Q,
      raylib::GAMEPAD_BUTTON_LEFT_TRIGGER_1,
  };

  mapping[InputAction::ShootRight] = {
      raylib::KEY_E,
      raylib::GAMEPAD_BUTTON_RIGHT_TRIGGER_1,
  };

  mapping[InputAction::WidgetBack] = {
      raylib::GAMEPAD_BUTTON_LEFT_FACE_UP,
      raylib::KEY_UP,
  };

  mapping[InputAction::WidgetNext] = {
      raylib::KEY_TAB,
      raylib::GAMEPAD_BUTTON_LEFT_FACE_DOWN,
      raylib::KEY_DOWN,
  };

  mapping[InputAction::WidgetPress] = {
      raylib::KEY_ENTER,
      raylib::GAMEPAD_BUTTON_RIGHT_FACE_DOWN,
  };

  mapping[InputAction::WidgetMod] = {
      raylib::KEY_LEFT_SHIFT,
  };

  mapping[InputAction::ToggleUIDebug] = {
      raylib::KEY_GRAVE,
  };

  mapping[InputAction::ToggleUILayoutDebug] = {
      raylib::KEY_EQUAL,
  };

  return mapping;
}