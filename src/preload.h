

#pragma once

#include <memory>

#include "afterhours/src/singleton.h"
#include "library.h"

enum FontID {
  EQPro,
  raylibFont,
};

std::string get_font_name(FontID id);

SINGLETON_FWD(Preload)
struct Preload {
  SINGLETON(Preload)

  Preload();
  ~Preload();

  Preload(const Preload &) = delete;
  void operator=(const Preload &) = delete;

  Preload &init(int width, int height, const char *title);
  Preload &make_singleton();
};
