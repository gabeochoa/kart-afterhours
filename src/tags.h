#pragma once

#include <cstdint>

enum struct GameTag : uint8_t {
  MapGenerated = 0,
  FloorOverlay = 1,
  SkipTextureRendering = 2,
  IsLastRoundsWinner = 3,
};
