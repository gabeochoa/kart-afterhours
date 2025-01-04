
#pragma once

#include <memory>

#include "afterhours/src/plugins/window_manager.h"

struct S_Data;

struct Settings {
  std::unique_ptr<S_Data> data;

  void reset();
  void refresh_settings();
};
