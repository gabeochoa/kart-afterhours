
#pragma once

#include "sound_library.h"

// TODO this needs to be converted into a component with SoundAlias's
// in raylib a Sound can only be played once and hitting play again
// will restart it.
//
// we need to make aliases so that each one can play at the same time
struct CarRumble : System<Transform, CanShoot> {
  virtual void for_each_with(const Entity &, const Transform &,
                             const CanShoot &, float) const override {
    SoundLibrary::get().play(SoundFile::Rumble);
  }
};
