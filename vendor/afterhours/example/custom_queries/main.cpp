

#include <iostream>

#define AFTER_HOURS_ENTITY_HELPER
#define AFTER_HOURS_ENTITY_QUERY
#include "../../ah.h"
#include "../shared/vector.h"

namespace afterhours {
// Make some components :)
struct Transform : public BaseComponent {
  Transform(float x, float y) {
    position.x = x;
    position.y = y;
  }

  virtual ~Transform() {}

  [[nodiscard]] vec2 pos() const { return position; }

private:
  vec2 position;
};

// make some entities
void make_entities() {
  for (int i = 0; i < 10; i++) {
    auto &entity = EntityHelper::createEntity();
    entity.addComponent<Transform>(i, i);
  }
}

// Add new type of query based on our new component
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
};
} // namespace afterhours

int main(int, char **) {
  using namespace afterhours;

  make_entities();

  auto all_entities = EQ().gen();
  std::cout << "All Ents: " << all_entities.size() << "\n";
  auto all_with_transforms = EQ().whereHasComponent<Transform>().gen();
  std::cout << "Ents with Transform: " << all_with_transforms.size() << "\n";
  auto close_entities = EQ().whereInRange(vec2{0, 0}, 2).gen();
  std::cout << "Close Ents: " << close_entities.size() << "\n";

  return 0;
}
