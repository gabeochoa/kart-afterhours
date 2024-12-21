

#include <iostream>

#define AFTER_HOURS_ENTITY_HELPER
#define AFTER_HOURS_ENTITY_QUERY
#define AFTER_HOURS_SYSTEM
#include "../../ah.h"
#include "../shared/vector.h"

/*

   See comment in system.h about Not<>


namespace afterhours {


// Make some components :)
struct Exists : public BaseComponent {};
struct SkipMe : public BaseComponent {};

struct SkipsSkippable : System<Not<SkipMe>> {
virtual void for_each_with(Entity &entity, Not<SkipMe> &, float) override {
std::cout << "entity " << entity.id << std::endl;
}
};

} // namespace afterhours

void make_entities() {
using namespace afterhours;

for (int i = 0; i < 5; i++) {
auto &entity = EntityHelper::createEntity();
entity.addComponent<Exists>();
if (i % 2 == 0)
  entity.addComponent<SkipMe>();
}
}

int main(int, char **) {
using namespace afterhours;

make_entities();

SystemManager systems;
systems.register_update_system(std::make_unique<SkipsSkippable>());

systems.run(1.f);

return 0;
}
*/
int main(int, char **) {}
