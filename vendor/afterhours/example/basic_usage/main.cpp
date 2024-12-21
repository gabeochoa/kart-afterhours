

#include <iostream>

#include "../../src/entity_helper.h"
#include "../../src/entity.h"

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

struct OtherComponent : public BaseComponent {
  virtual ~OtherComponent() {}
};
} // namespace afterhours

int main(int, char **) {
  using namespace afterhours;

  auto &entity = EntityHelper::createEntity();
  entity.addComponent<Transform>(0, 10);
  std::cout << "Creating a component with only a transform component" << "\n";

  std::cout << std::boolalpha;
  std::cout << "has transform? " << entity.has<Transform>() << "\n";
  std::cout << "has other component? " << entity.has<OtherComponent>() << "\n";

  return 0;
}
