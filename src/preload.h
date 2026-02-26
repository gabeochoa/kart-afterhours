

#pragma once

#include <memory>

#include <afterhours/src/library.h>
#include <afterhours/src/singleton.h>
#include <afterhours/src/graphics.h>

SINGLETON_FWD(Preload)
struct Preload {
  SINGLETON(Preload)

  Preload();
  ~Preload();

  Preload(const Preload &) = delete;
  void operator=(const Preload &) = delete;

  Preload &init(const char *title,
                afterhours::graphics::DisplayMode mode = afterhours::graphics::DisplayMode::Windowed);
  Preload &make_singleton();
};
