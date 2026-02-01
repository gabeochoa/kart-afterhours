#pragma once

#include "components.h"
#include "query.h"

inline float affector_steering_multiplier(const Transform &transform) {
  float multiplier = 1.f;
  auto affectors = EQ().whereHasComponent<SteeringAffector>()
                       .whereOverlaps(transform.rect())
                       .gen_as<SteeringAffector>();
  for (SteeringAffector &affector : affectors)
    multiplier *= affector.multiplier;
  return multiplier;
}

inline float affector_acceleration_multiplier(const Transform &transform) {
  float multiplier = 1.f;
  auto affectors = EQ().whereHasComponent<AccelerationAffector>()
                       .whereOverlaps(transform.rect())
                       .gen_as<AccelerationAffector>();
  for (AccelerationAffector &affector : affectors)
    multiplier *= affector.multiplier;
  return multiplier;
}

inline float
affector_steering_sensitivity_additive(const Transform &transform) {
  float sensitivity = 0.f;
  auto affectors = EQ().whereHasComponent<SteeringIncrementor>()
                       .whereOverlaps(transform.rect())
                       .gen_as<SteeringIncrementor>();
  for (SteeringIncrementor &affector : affectors)
    sensitivity += affector.target_sensitivity;
  return sensitivity;
}

inline float affector_speed_multiplier(const Transform &transform) {
  float multiplier = 1.f;
  auto affectors = EQ().whereHasComponent<SpeedAffector>()
                       .whereOverlaps(transform.rect())
                       .gen_as<SpeedAffector>();
  for (SpeedAffector &affector : affectors)
    multiplier *= affector.multiplier;
  return multiplier;
}
