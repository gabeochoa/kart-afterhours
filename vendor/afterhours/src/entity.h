
#pragma once

#include <atomic>
#include <bitset>
#include <map>
#include <optional>
#include <utility>

#include "base_component.h"
#include "type_name.h"

template <typename Base, typename Derived> bool child_of(Derived *derived) {
  return dynamic_cast<Base *>(derived) != nullptr;
}

#if !defined(AFTER_HOURS_REPLACE_LOGGING)
// TODO eventually implement these
// TODO move to a log.h file and include them in the other parts of the library
inline void log_trace(...) {}
inline void log_info(...) {}
inline void log_warn(...) {}
inline void log_error(...) {}
#endif

#if !defined(AFTER_HOURS_REPLACE_VALIDATE)
inline void VALIDATE(...) {}
#endif

using ComponentBitSet = std::bitset<max_num_components>;
// originally this was a std::array<BaseComponent*, max_num_components> but i
// cant seem to serialize this so lets try map
using ComponentArray = std::map<ComponentID, std::unique_ptr<BaseComponent>>;
using EntityID = int;

static std::atomic_int ENTITY_ID_GEN = 0;

struct Entity {
  EntityID id;
  int entity_type = 0;

  ComponentBitSet componentSet;
  ComponentArray componentArray;

  bool cleanup = false;

  Entity() : id(ENTITY_ID_GEN++) {}
  Entity(const Entity &) = delete;
  Entity(Entity &&other) noexcept = default;

  virtual ~Entity() { componentArray.clear(); }

  // These two functions can be used to validate than an entity has all of the
  // matching components that are needed for this system to run
  template <typename T> [[nodiscard]] bool has() const {
    log_trace("checking component {} {} on entity {}",
              components::get_type_id<T>(), type_name<T>(), id);
    log_trace("your set is now {}", componentSet);
    bool result = componentSet[components::get_type_id<T>()];
    log_trace("and the result was {}", result);
    return result;
  }

  template <typename T> [[nodiscard]] bool has_child_of() const {
    log_trace("checking for child components {} {} on entity {}",
              components::get_type_id<T>(), type_name<T>(), id);
    for (const auto &pair : componentArray) {
      const auto &component = pair.second;
      if (child_of<T>(component.get())) {
        return true;
      }
    }
    return false;
  }

  template <typename A, typename B, typename... Rest> bool has() const {
    return has<A>() && has<B, Rest...>();
  }

  template <typename T> [[nodiscard]] bool is_missing() const {
    return !has<T>();
  }

  template <typename A> bool is_missing_any() const { return is_missing<A>(); }

  template <typename A, typename B, typename... Rest>
  bool is_missing_any() const {
    return is_missing<A>() || is_missing_any<B, Rest...>();
  }

  template <typename T> void removeComponent() {
    log_trace("removing component_id:{} {} to entity_id: {}",
              components::get_type_id<T>(), type_name<T>(), id);
    if (!this->has<T>()) {
      log_error("trying to remove but this entity {} doesnt have the "
                "component attached {} {}",
                id, components::get_type_id<T>(), type_name<T>());
    }
    componentSet[components::get_type_id<T>()] = false;
    componentArray.erase(components::get_type_id<T>());
  }

  template <typename T, typename... TArgs> T &addComponent(TArgs &&...args) {
    log_trace("adding component_id:{} {} to entity_id: {}",
              components::get_type_id<T>(), type_name<T>(), id);

    // checks for duplicates
    if (this->has<T>()) {
      log_warn("This entity {} already has this component attached id: "
               "{}, "
               "component {}",
               id, components::get_type_id<T>(), type_name<T>());

      VALIDATE(false, "duplicate component");
      // Commented out on purpose because the assert is gonna kill the
      // program anyway at some point we should stop enforcing it to avoid
      // crashing the game when people are playing
      //
      // return this->get<T>();
    }

    auto component = std::make_unique<T>(std::forward<TArgs>(args)...);
    ComponentID component_id = components::get_type_id<T>();
    componentArray[component_id] = std::move(component);
    componentSet[component_id] = true;

    log_trace("your set is now {}", componentSet);

    return get<T>();
  }

  template <typename T, typename... TArgs>
  T &addComponentIfMissing(TArgs &&...args) {
    if (this->has<T>())
      return this->get<T>();
    return addComponent<T>(std::forward<TArgs>(args)...);
  }

  template <typename T> void removeComponentIfExists() {
    if (this->is_missing<T>())
      return;
    return removeComponent<T>();
  }

  template <typename A> void addAll() { addComponent<A>(); }

  template <typename A, typename B, typename... Rest> void addAll() {
    addComponent<A>();
    addAll<B, Rest...>();
  }

  template <typename T> void warnIfMissingComponent() const {
    if (this->is_missing<T>()) {
      log_warn("This entity {} is missing id: {}, "
               "component {}",
               id, components::get_type_id<T>(), type_name<T>());
    }
  }

  template <typename T> [[nodiscard]] T &get_with_child() {
    log_trace("fetching for child components {} {} on entity {}",
              components::get_type_id<T>(), type_name<T>(), id);
    for (const auto &pair : componentArray) {
      const auto &component = pair.second;
      if (child_of<T>(component.get())) {
        return static_cast<T &>(*pair.second);
      }
    }
    warnIfMissingComponent<T>();
    return get<T>();
  }

  template <typename T> [[nodiscard]] const T &get_with_child() const {
    log_trace("fetching for child components {} {} on entity {}",
              components::get_type_id<T>(), type_name<T>(), id);
    for (const auto &pair : componentArray) {
      const auto &component = pair.second;
      if (child_of<T>(component.get())) {
        return static_cast<const T &>(*pair.second);
      }
    }
    return get<T>();
  }

  template <typename T> [[nodiscard]] T &get() {
    warnIfMissingComponent<T>();
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-stack-address"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-local-addr"
#endif
    return static_cast<T &>(
        *componentArray.at(components::get_type_id<T>()).get());
  }

  template <typename T> [[nodiscard]] const T &get() const {
    warnIfMissingComponent<T>();

    return static_cast<const T &>(
        *componentArray.at(components::get_type_id<T>()).get());
#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  }
};

using RefEntity = std::reference_wrapper<Entity>;
using OptEntityType = std::optional<std::reference_wrapper<Entity>>;

struct OptEntity {
  OptEntityType data;

  OptEntity() {}
  OptEntity(OptEntityType opt_e) : data(opt_e) {}
  OptEntity(RefEntity _e) : data(_e) {}
  OptEntity(Entity &_e) : data(_e) {}

  bool has_value() const { return data.has_value(); }
  bool valid() const { return has_value(); }

  Entity *value() { return &(data.value().get()); }
  Entity *operator*() { return value(); }
  Entity *operator->() { return value(); }

  const Entity *value() const { return &(data.value().get()); }
  const Entity *operator*() const { return value(); }
  // TODO look into switching this to using functional monadic stuff
  // i think optional.transform can take and handle calling functions on an
  // optional without having to expose the underlying value or existance of
  // value
  const Entity *operator->() const { return value(); }

  Entity &asE() { return data.value(); }
  const Entity &asE() const { return data.value(); }

  operator RefEntity() { return data.value(); }
  operator RefEntity() const { return data.value(); }
  operator bool() const { return valid(); }
};
