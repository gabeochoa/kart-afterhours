#pragma once

#include <string>

enum FontID {
  English,
  Korean,
  Japanese,
  raylibFont,
  SYMBOL_FONT,
};

std::string get_font_name(FontID id);