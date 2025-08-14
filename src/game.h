
#pragma once

#include "std_include.h"

#include "rl.h"

// Owned by main.cpp
extern bool running;
// TODO move into library or somethign
extern raylib::RenderTexture2D mainRT;
// second render texture for compositing tag shader and UI
extern raylib::RenderTexture2D screenRT;
