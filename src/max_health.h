
#pragma once

#include "log.h"

// 12 gives us these options:
// 1,2,3,4,6,12
constexpr static int MAX_HEALTH = 120;

constexpr static int kill_shots_to_base_dmg(int num_shots) {
  if (!(num_shots == 1 || num_shots == 2 || num_shots == 3 || num_shots == 4 ||
        num_shots == 6 || num_shots == 12)) {
    log_error("You are setting a non divisible number of shots: {}", num_shots);
  }
  return MAX_HEALTH / num_shots;
}
