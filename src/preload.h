

#pragma once

#include <memory>

#include "library.h"
#include "singleton.h"

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

  void init(int width, int height, const char *title);
};
