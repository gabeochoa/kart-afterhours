

#pragma once

#include "std_include.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang diagnostic ignored "-Wdeprecated-volatile"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdangling-reference"
#endif

namespace raylib {
#include "raylib/raylib.h"
#include "raylib/raymath.h"
//
// #include "RaylibOpOverloads.h"

inline Vector2 &operator-(Vector2 &a) {
  a = Vector2Negate(a);
  return a;
}

inline void DrawSplineSegmentLinear(Vector2 p1, Vector2 p2, float thick,
                                    Color color) {
  // NOTE: For the linear spline we don't use subdivisions, just a single quad

  Vector2 delta = {p2.x - p1.x, p2.y - p1.y};
  float length = sqrtf(delta.x * delta.x + delta.y * delta.y);

  if ((length > 0) && (thick > 0)) {
    float scale = thick / (2 * length);

    Vector2 radius = {-scale * delta.y, scale * delta.x};
    Vector2 strip[4] = {{p1.x - radius.x, p1.y - radius.y},
                        {p1.x + radius.x, p1.y + radius.y},
                        {p2.x - radius.x, p2.y - radius.y},
                        {p2.x + radius.x, p2.y + radius.y}};

    DrawTriangleStrip(strip, 4, color);
  }
}

inline void DrawSplineLinear(const Vector2 *points, int pointCount, float thick,
                             Color color) {
  if (pointCount < 2)
    return;

#if defined(SUPPORT_SPLINE_MITERS)
  Vector2 prevNormal =
      (Vector2){-(points[1].y - points[0].y), (points[1].x - points[0].x)};
  float prevLength =
      sqrtf(prevNormal.x * prevNormal.x + prevNormal.y * prevNormal.y);

  if (prevLength > 0.0f) {
    prevNormal.x /= prevLength;
    prevNormal.y /= prevLength;
  } else {
    prevNormal.x = 0.0f;
    prevNormal.y = 0.0f;
  }

  Vector2 prevRadius = {0.5f * thick * prevNormal.x,
                        0.5f * thick * prevNormal.y};

  for (int i = 0; i < pointCount - 1; i++) {
    Vector2 normal = {0};

    if (i < pointCount - 2) {
      normal = (Vector2){-(points[i + 2].y - points[i + 1].y),
                         (points[i + 2].x - points[i + 1].x)};
      float normalLength = sqrtf(normal.x * normal.x + normal.y * normal.y);

      if (normalLength > 0.0f) {
        normal.x /= normalLength;
        normal.y /= normalLength;
      } else {
        normal.x = 0.0f;
        normal.y = 0.0f;
      }
    } else {
      normal = prevNormal;
    }

    Vector2 radius = {prevNormal.x + normal.x, prevNormal.y + normal.y};
    float radiusLength = sqrtf(radius.x * radius.x + radius.y * radius.y);

    if (radiusLength > 0.0f) {
      radius.x /= radiusLength;
      radius.y /= radiusLength;
    } else {
      radius.x = 0.0f;
      radius.y = 0.0f;
    }

    float cosTheta = radius.x * normal.x + radius.y * normal.y;

    if (cosTheta != 0.0f) {
      radius.x *= (thick * 0.5f / cosTheta);
      radius.y *= (thick * 0.5f / cosTheta);
    } else {
      radius.x = 0.0f;
      radius.y = 0.0f;
    }

    Vector2 strip[4] = {
        {points[i].x - prevRadius.x, points[i].y - prevRadius.y},
        {points[i].x + prevRadius.x, points[i].y + prevRadius.y},
        {points[i + 1].x - radius.x, points[i + 1].y - radius.y},
        {points[i + 1].x + radius.x, points[i + 1].y + radius.y}};

    DrawTriangleStrip(strip, 4, color);

    prevRadius = radius;
    prevNormal = normal;
  }

#else // !SUPPORT_SPLINE_MITERS

  Vector2 delta = {0};
  float scale = 0.0f;

  for (int i = 0; i < pointCount - 1; i++) {
    delta =
        (Vector2){points[i + 1].x - points[i].x, points[i + 1].y - points[i].y};
    float length = sqrtf(delta.x * delta.x + delta.y * delta.y);

    if (length > 0)
      scale = thick / (2 * length);

    Vector2 radius = {-scale * delta.y, scale * delta.x};
    Vector2 strip[4] = {
        {points[i].x - radius.x, points[i].y - radius.y},
        {points[i].x + radius.x, points[i].y + radius.y},
        {points[i + 1].x - radius.x, points[i + 1].y - radius.y},
        {points[i + 1].x + radius.x, points[i + 1].y + radius.y}};

    DrawTriangleStrip(strip, 4, color);
  }
#endif
}

inline std::ostream &operator<<(std::ostream &os, Vector2 a) {
  os << "(" << a.x << "," << a.y << ")";
  return os;
}

inline std::ostream &operator<<(std::ostream &os, Vector3 a) {
  os << "(" << a.x << "," << a.y << "," << a.z << ")";
  return os;
}

inline std::ostream &operator<<(std::ostream &os, Vector4 a) {
  os << "(" << a.x << "," << a.y << "," << a.z << "," << a.w << ")";
  return os;
}

inline std::ostream &operator<<(std::ostream &os, Color c) {
  os << "(" << (unsigned int)c.r << "," << (unsigned int)c.g << ","
     << (unsigned int)c.b << "," << (unsigned int)c.a << ")";
  return os;
}

} // namespace raylib

#include <GLFW/glfw3.h>

// We redefine the max here because the max keyboardkey is in the 300s
#undef MAGIC_ENUM_RANGE_MAX
#define MAGIC_ENUM_RANGE_MAX 400
#include <magic_enum/magic_enum.hpp>

#include "log.h"

#define AFTER_HOURS_INPUT_VALIDATION_ASSERT
#define AFTER_HOURS_INCLUDE_DERIVED_CHILDREN
#define AFTER_HOURS_ENTITY_HELPER
#define AFTER_HOURS_ENTITY_QUERY
#define AFTER_HOURS_SYSTEM
#define AFTER_HOURS_IMM_UI
#define AFTER_HOURS_USE_RAYLIB

#define RectangleType raylib::Rectangle
#define Vector2Type raylib::Vector2
#define TextureType raylib::Texture2D
#include "afterhours/ah.h"
#include "afterhours/src/developer.h"
#include "afterhours/src/plugins/input_system.h"
#include "afterhours/src/plugins/texture_manager.h"
#include "afterhours/src/plugins/window_manager.h"
#include <cassert>

typedef raylib::Vector2 vec2;
typedef raylib::Vector3 vec3;
typedef raylib::Vector4 vec4;
using raylib::Rectangle;

#include "afterhours/src/plugins/autolayout.h"
#include "afterhours/src/plugins/ui.h"

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
