

#include <iostream>

#define AFTER_HOURS_ENTITY_HELPER
#define AFTER_HOURS_ENTITY_QUERY
#define AFTER_HOURS_SYSTEM
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
  void update(vec2 v) { position = v; }

private:
  vec2 position;
};

struct OncePerTick : System<> {

  void once(float) override { std::cout << "OncePerTick::once" << std::endl; }
};

} // namespace afterhours

void make_entities() {
  using namespace afterhours;

  for (int i = 0; i < 5; i++) {
    EntityHelper::createEntity();
  }
}

int main(int, char **) {
  using namespace afterhours;

  make_entities();

  SystemManager systems;
  systems.register_update_system(
      []() { std::cout << "Anonymous::once" << std::endl; });
  systems.register_update_system(std::make_unique<OncePerTick>());

  for (int i = 0; i < 2; i++) {
    systems.run(1.f);
  }

  return 0;
}
