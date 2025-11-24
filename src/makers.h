
#pragma once

#include "components.h"
#include "weapons.h"

void make_explosion_anim(afterhours::Entity &parent);
void make_poof_anim(afterhours::Entity &parent, const Weapon &wp, float angle_offset);
void make_bullet(afterhours::Entity &parent, const Weapon &wp, float angle_offset);

void make_poof_anim(afterhours::Entity &parent, Weapon::FiringDirection dir,
                    float base_angle, float angle_offset);
void make_bullet(afterhours::Entity &parent, const ProjectileConfig &cfg,
                 Weapon::FiringDirection dir, float angle_offset);

/// Creates a vehicle with a unique @p id.
/// @param id should be a unique number, dictates spawn positioning and
/// labeling.
afterhours::Entity &make_car(size_t id);

/// Creates a simple rectangular shaped obstacle.
/// @param rect is the location and placement of the obstacle.
/// @param color is the color the obstacle will have.
/// @param collision_config is the collision configuration affecting how it
/// impacts collisions.
afterhours::Entity &make_obstacle(raylib::Rectangle rect, const raylib::Color color,
                      const CollisionConfig &collision_config);

void make_player(input::GamepadID id);
void make_ai();

/// Creates a hippo item at the specified position.
/// @param position is the location where the hippo item will be spawned.
afterhours::Entity &make_hippo_item(vec2 position);

/// Creates an oil slick area that affects car steering and acceleration.
afterhours::Entity &make_oil_slick(raylib::Rectangle rect, float steering_multiplier,
                       float acceleration_multiplier,
                       float steering_sensitivity_target);

afterhours::Entity &make_default_oil_slick(raylib::Rectangle rect);

/// Creates a sticky goo area that heavily slows and reduces steering.
afterhours::Entity &make_sticky_goo(raylib::Rectangle rect);
