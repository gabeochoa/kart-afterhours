
#pragma once

#include "std_include.h"
#include <optional>

#include "rl.h"

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

constexpr float distance_sq(const vec2 a, const vec2 b) {
  return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}

float vec_dot(const vec2 &a, const vec2 &b) { return a.x * b.x + a.y * b.y; }
float vec_cross(const vec2 &a, const vec2 &b) { return a.x * b.y - a.y * b.x; }

float vec_mag(vec2 v) { return (float)sqrt(v.x * v.x + v.y * v.y); }
vec2 vec_norm(vec2 v) {
  float mag = vec_mag(v);
  if (mag == 0)
    return v;
  return vec2{
      v.x / mag,
      v.y / mag,
  };
}

constexpr static float to_radians(float angle) {
  return angle * (float)(M_PI / 180.f);
}

constexpr static float to_degrees(float radians) {
  return radians * 180.f / (float)M_PI;
}

static bool is_point_inside(const vec2 point, const RectangleType &rect) {
  return point.x >= rect.x && point.x <= rect.x + rect.width &&
         point.y >= rect.y && point.y <= rect.y + rect.height;
}
vec2 rect_center(const Rectangle &rect) {
  return vec2{
      rect.x + (rect.width / 2.f),
      rect.y + (rect.height / 2.f),
  };
}

vec2 calc(const Rectangle &rect, const vec2 &point) {
  vec2 center = rect_center(rect);
  float s = (point.y - center.y) / (point.x - center.x);
  float h = rect.height;
  float w = rect.width;
  float h2 = h / 2.f;
  float w2 = w / 2.f;
  float h2s = h2 / s;

  if ((-h2 <= s * w2) && (s * w2 <= h2)) {
    float dir = (point.x > center.x) ? 1.f : -1.f;
    return {center.x + (dir * w2), center.y + dir * (s * w2)};
  }
  if ((-w2 <= h2s) && (h2s <= w2)) {
    float dir = (point.y > center.y) ? 1.f : -1.f;
    return {center.x + dir * h2s, center.y + (dir * h2)};
  }
  return {0, 0};
}