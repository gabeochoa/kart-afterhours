
#pragma once

#include "components.h"
#include "weapons.h"

void make_explosion_anim(Entity &parent);
void make_poof_anim(Entity &parent, const Weapon &wp, float angle_offset);
void make_bullet(Entity &parent, const Weapon &wp, float angle_offset);

/// Creates a vehicle with a unique @p id.
/// @param id should be a unique number, dictates spawn positioning and
/// labeling.
Entity &make_car(size_t id);

/// Creates a simple rectangular shaped obstacle.
/// @param rect is the location and placement of the obstacle.
/// @param color is the obstacle's rendered color.
/// @param collision_config configures the physics of the obstacle.
Entity &make_obstacle(raylib::Rectangle rect, raylib::Color color,
                      const CollisionConfig &collision_config);

void make_player(input::GamepadID id);
void make_ai();
