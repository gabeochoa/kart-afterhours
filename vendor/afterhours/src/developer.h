
#pragma once

#include <cassert>
#include <iostream>

#include "base_component.h"
#include "entity_query.h"
#include "system.h"
#include "type_name.h"

namespace afterhours {

// TODO move into a dedicated file?
namespace util {
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }
} // namespace util

namespace developer {

template <typename Component> struct EnforceSingleton : System<Component> {

  bool saw_one;

  virtual void once(float) override { saw_one = false; }

  virtual void for_each_with(Entity &, Component &, float) override {

    if (saw_one) {
      std::cerr << "Enforcing only one entity with " << type_name<Component>()
                << std::endl;
      assert(false);
    }
    saw_one = true;
  }
};

struct Plugin {
  // If you are writing a plugin you should implement these functions
  // or some set/subset or these names with new parameters

  static void add_singleton_components(Entity &entity);
  static void enforce_singletons(SystemManager &sm);
  static void register_update_systems(SystemManager &sm);
};
} // namespace developer

} // namespace afterhours
