

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
#if defined(__has_include)
#if __has_include(<raylib.h>)
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#elif __has_include("raylib/raylib.h")
#include "raylib/raylib.h"
#include "raylib/raymath.h"
#include "raylib/rlgl.h"
#else
#error "raylib headers not found"
#endif
#else
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#endif

#if defined(BUILT_WITH_XMAKE)
#else

// Add missing Vector2 operators
inline Vector2 operator+(const Vector2& a, const Vector2& b) {
    return Vector2{a.x + b.x, a.y + b.y};
}

inline Vector2 operator-(const Vector2& a, const Vector2& b) {
    return Vector2{a.x - b.x, a.y - b.y};
}

inline Vector2 operator/(const Vector2& a, float s) {
    return Vector2{a.x / s, a.y / s};
}

inline Vector2& operator+=(Vector2& a, const Vector2& b) {
    a.x += b.x;
    a.y += b.y;
    return a;
}

inline Vector2& operator-=(Vector2& a, const Vector2& b) {
    a.x -= b.x;
    a.y -= b.y;
    return a;
}
#endif

inline Vector2 operator*(const float s, const Vector2& a) { return Vector2Scale(a, s); }
inline Vector3 operator*(const float s, const Vector3& a) { return Vector3Scale(a, s); }

// Add vector * scalar operators for convenience
inline Vector2 operator*(const Vector2& a, const float s) { return Vector2Scale(a, s); }
inline Vector3 operator*(const Vector3& a, const float s) { return Vector3Scale(a, s); }

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
#include <afterhours/ah.h>
#include <afterhours/src/developer.h>
#include <afterhours/src/plugins/input_system.h>
#include <afterhours/src/plugins/texture_manager.h>
#include <afterhours/src/plugins/window_manager.h>
#include <cassert>

typedef raylib::Vector2 vec2;
typedef raylib::Vector3 vec3;
typedef raylib::Vector4 vec4;
using raylib::Rectangle;

#include <afterhours/src/plugins/autolayout.h>
#include <afterhours/src/plugins/ui.h>

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
