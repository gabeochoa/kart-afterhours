
#pragma once

#include "components.h"

struct EQ : public EntityQuery<EQ> {
  struct WhereInRange : EntityQuery::Modification {
    vec2 position;
    float range;

    // TODO mess around with the right epsilon here
    explicit WhereInRange(vec2 pos, float r = 0.01f)
        : position(pos), range(r) {}
    bool operator()(const Entity &entity) const override {
      vec2 pos = entity.get<Transform>().pos();
      return distance_sq(position, pos) < (range * range);
    }
  };

  EQ &whereInRange(const vec2 &position, float range) {
    return add_mod(new WhereInRange(position, range));
  }

  EQ &orderByDist(const vec2 &position) {
    return orderByLambda([position](const Entity &a, const Entity &b) {
      float a_dist = distance_sq(a.get<Transform>().pos(), position);
      float b_dist = distance_sq(b.get<Transform>().pos(), position);
      return a_dist < b_dist;
    });
  }

  struct WhereOverlaps : EntityQuery::Modification {
    Rectangle rect;

    explicit WhereOverlaps(Rectangle rect_) : rect(rect_) {}

    bool operator()(const Entity &entity) const override {
      const auto r1 = rect;
      const auto r2 = entity.get<Transform>().rect();
      // Check if the rectangles overlap on the x-axis
      const bool xOverlap = r1.x < r2.x + r2.width && r2.x < r1.x + r1.width;

      // Check if the rectangles overlap on the y-axis
      const bool yOverlap = r1.y < r2.y + r2.height && r2.y < r1.y + r1.height;

      // Return true if both x and y overlap
      return xOverlap && yOverlap;
    }
  };

  EQ &whereOverlaps(const Rectangle r) { return add_mod(new WhereOverlaps(r)); }
};
