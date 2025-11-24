#pragma once

#include "components.h"
#include "query.h"

inline float affector_steering_multiplier(const Transform &transform) {
  float multiplier = 1.f;
  auto entities = EQ().whereHasComponent<SteeringAffector>()
                      .whereOverlaps(transform.rect())
                      .gen();
  for (afterhours::Entity &entity : entities)
    multiplier *= entity.get<SteeringAffector>().multiplier;
  return multiplier;
}

inline float affector_acceleration_multiplier(const Transform &transform) {
  float multiplier = 1.f;
  auto entities = EQ().whereHasComponent<AccelerationAffector>()
                      .whereOverlaps(transform.rect())
                      .gen();
  for (afterhours::Entity &entity : entities)
    multiplier *= entity.get<AccelerationAffector>().multiplier;
  return multiplier;
}

inline float
affector_steering_sensitivity_additive(const Transform &transform) {
  float sensitivity = 0.f;
  auto entities = EQ().whereHasComponent<SteeringIncrementor>()
                      .whereOverlaps(transform.rect())
                      .gen();
  for (afterhours::Entity &entity : entities)
    sensitivity += entity.get<SteeringIncrementor>().target_sensitivity;
  return sensitivity;
}

inline float affector_speed_multiplier(const Transform &transform) {
  float multiplier = 1.f;
  auto entities = EQ().whereHasComponent<SpeedAffector>()
                      .whereOverlaps(transform.rect())
                      .gen();
  for (afterhours::Entity &entity : entities)
    multiplier *= entity.get<SpeedAffector>().multiplier;

  return multiplier;
}
