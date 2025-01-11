
#pragma once

#include "components.h"
#include "weapons.h"

void make_explosion_anim(Entity &parent);
void make_poof_anim(Entity &parent, const Weapon &wp, float angle_offset);
void make_bullet(Entity &parent, const Weapon &wp, float angle_offset);
Entity &make_car(size_t id);
void make_player(input::GamepadID id);
void make_ai();
