

#pragma once

#include <memory>

#include "library.h"
#include <afterhours/src/singleton.h>

enum FontID {
  EQPro,
  raylibFont,
  CJK,
  SYMBOL_FONT,
};

std::string get_font_name(FontID id);

SINGLETON_FWD(Preload)
struct Preload {
  SINGLETON(Preload)

  Preload();
  ~Preload();

  Preload(const Preload &) = delete;
  void operator=(const Preload &) = delete;

  Preload &init(const char *title);
  Preload &make_singleton();
};
