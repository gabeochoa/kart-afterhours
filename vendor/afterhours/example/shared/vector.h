
#pragma once

struct vec2 {
  float x;
  float y;

  vec2 operator+(const vec2 &other) const {
    return vec2{x + other.x, y + other.y};
  }
  vec2 operator-(const vec2 &other) const {
    return vec2{x - other.x, y - other.y};
  }
};

constexpr float distance_sq(const vec2 a, const vec2 b) {
  return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}
