

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

struct HasName : public BaseComponent {
  std::string name;
  HasName(std::string s) : name(s) {}
  virtual ~HasName() {}
};

struct Moves : System<Transform> {
  virtual ~Moves() {}
  // notice how the system<> template class is given here
  // so we will have access to the underlying
  virtual void for_each_with(Entity &entity, Transform &transform,
                             float) override {
    auto p = transform.pos() + vec2{1, 0};
    transform.update(p);
    std::cout << " updating for entity " << entity.id << " "
              << transform.pos().x << std::endl;
  }
};

struct MovesAndHasName : System<Transform, HasName> {
  virtual ~MovesAndHasName() {}
  virtual void for_each_with(Entity &entity, Transform &transform,
                             HasName &hasName, float) override {
    auto p = transform.pos() + vec2{1, 0};
    transform.update(p);
    std::cout << " updating for " << hasName.name << " with " << entity.id
              << " " << transform.pos().x << std::endl;
  }
};

} // namespace afterhours

void make_entities() {
  using namespace afterhours;

  for (int i = 0; i < 5; i++) {
    auto &entity = EntityHelper::createEntity();
    entity.addComponent<Transform>(0, 10);
  }

  for (int i = 0; i < 5; i++) {
    auto &entity = EntityHelper::createEntity();
    entity.addComponent<Transform>(0, 10);
    entity.addComponent<HasName>("my name");
  }
}

int main(int, char **) {
  using namespace afterhours;

  make_entities();

  SystemManager systems;
  systems.register_update_system(std::make_unique<Moves>());
  systems.register_update_system(std::make_unique<MovesAndHasName>());

  for (int i = 0; i < 2; i++) {
    systems.run(1.f);
  }

  return 0;
}
