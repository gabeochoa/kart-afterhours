
#pragma once

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <vector>

#include "entity.h"
#include "entity_helper.h"

template <typename Derived = void> //
struct EntityQuery {
  using TReturn =
      std::conditional_t<std::is_same_v<Derived, void>, EntityQuery, Derived>;

  struct Modification {
    virtual ~Modification() = default;
    virtual bool operator()(const Entity &) const = 0;
  };

  TReturn &add_mod(Modification *mod) {
    mods.push_back(std::unique_ptr<Modification>(mod));
    return static_cast<TReturn &>(*this);
  }

  struct Not : Modification {
    std::unique_ptr<Modification> mod;

    explicit Not(Modification *m) : mod(m) {}

    bool operator()(const Entity &entity) const override {
      return !((*mod)(entity));
    }
  };
  // TODO i would love to just have an api like
  // .not(whereHasComponent<Example>())
  // but   ^ doesnt have a type we can pass
  // we could do something like
  // .not(new WhereHasComponent<Example>())
  // but that would exclude most of the helper fns

  struct Limit : Modification {
    int amount;
    mutable int amount_taken;
    explicit Limit(int amt) : amount(amt), amount_taken(0) {}

    bool operator()(const Entity &) const override {
      if (amount_taken > amount)
        return false;
      amount_taken++;
      return true;
    }
  };
  TReturn &take(int amount) { return add_mod(new Limit(amount)); }
  TReturn &first() { return take(1); }

  struct WhereID : Modification {
    int id;
    explicit WhereID(int idIn) : id(idIn) {}
    bool operator()(const Entity &entity) const override {
      return entity.id == id;
    }
  };
  TReturn &whereID(int id) { return add_mod(new WhereID(id)); }
  TReturn &whereNotID(int id) { return add_mod(new Not(new WhereID(id))); }

  struct WhereMarkedForCleanup : Modification {
    bool operator()(const Entity &entity) const override {
      return entity.cleanup;
    }
  };

  TReturn &whereMarkedForCleanup() {
    return add_mod(new WhereMarkedForCleanup());
  }
  TReturn &whereNotMarkedForCleanup() {
    return add_mod(new Not(new WhereMarkedForCleanup()));
  }

  template <typename T> struct WhereHasComponent : Modification {
    bool operator()(const Entity &entity) const override {
      return entity.has<T>();
    }
  };
  template <typename T> auto &whereHasComponent() {
    return add_mod(new WhereHasComponent<T>());
  }
  template <typename T> auto &whereMissingComponent() {
    return add_mod(new Not(new WhereHasComponent<T>()));
  }

  struct WhereLambda : Modification {
    std::function<bool(const Entity &)> filter;
    explicit WhereLambda(const std::function<bool(const Entity &)> &cb)
        : filter(cb) {}
    bool operator()(const Entity &entity) const override {
      return filter(entity);
    }
  };
  TReturn &whereLambda(const std::function<bool(const Entity &)> &fn) {
    return add_mod(new WhereLambda(fn));
  }
  TReturn &
  whereLambdaExistsAndTrue(const std::function<bool(const Entity &)> &fn) {
    if (fn)
      return add_mod(new WhereLambda(fn));
    return static_cast<TReturn &>(*this);
  }

  /////////

  // TODO add support for converting Entities to other Entities

  using OrderByFn = std::function<bool(const Entity &, const Entity &)>;
  struct OrderBy {
    virtual ~OrderBy() {}
    virtual bool operator()(const Entity &a, const Entity &b) = 0;
  };

  struct OrderByLambda : OrderBy {
    OrderByFn sortFn;
    explicit OrderByLambda(const OrderByFn &sortFnIn) : sortFn(sortFnIn) {}

    bool operator()(const Entity &a, const Entity &b) override {
      return sortFn(a, b);
    }
  };

  TReturn &orderByLambda(const OrderByFn &sortfn) {
    return static_cast<TReturn &>(set_order_by(new OrderByLambda(sortfn)));
  }

  /////////
  struct UnderlyingOptions {
    bool stop_on_first = false;
  };

  [[nodiscard]] bool has_values() const {
    return !run_query({.stop_on_first = true}).empty();
  }

  [[nodiscard]] bool is_empty() const {
    return run_query({.stop_on_first = true}).empty();
  }

  [[nodiscard]] RefEntities
  values_ignore_cache(UnderlyingOptions options) const {
    ents = run_query(options);
    return ents;
  }

  [[nodiscard]] RefEntities gen() const {
    if (!ran_query)
      return values_ignore_cache({});
    return ents;
  }

  [[nodiscard]] RefEntities gen_with_options(UnderlyingOptions options) const {
    if (!ran_query)
      return values_ignore_cache(options);
    return ents;
  }

  [[nodiscard]] OptEntity gen_first() const {
    if (has_values()) {
      auto values = gen_with_options({.stop_on_first = true});
      if (values.empty()) {
        log_error("we expected to find a value but didnt...");
      }
      return values[0];
    }
    return {};
  }

  [[nodiscard]] Entity &gen_first_enforce() const {
    if (!has_values()) {
      log_error("tried to use gen enforce, but found no values");
    }
    auto values = gen_with_options({.stop_on_first = true});
    if (values.empty()) {
      log_error("we expected to find a value but didnt...");
    }
    return values[0];
  }

  [[nodiscard]] std::optional<int> gen_first_id() const {
    if (!has_values())
      return {};
    return gen_with_options({.stop_on_first = true})[0].get().id;
  }

  [[nodiscard]] size_t gen_count() const {
    if (!ran_query)
      return values_ignore_cache({}).size();
    return ents.size();
  }

  [[nodiscard]] std::vector<int> gen_ids() const {
    const auto results = ran_query ? ents : values_ignore_cache({});
    std::vector<int> ids;
    std::transform(results.begin(), results.end(), std::back_inserter(ids),
                   [](const Entity &ent) -> int { return ent.id; });
    return ids;
  }

  EntityQuery() : entities(EntityHelper::get_entities()) {}
  explicit EntityQuery(const Entities &entsIn) : entities(entsIn) {
    entities = entsIn;
  }

  TReturn &include_store_entities(bool include = true) {
    _include_store_entities = include;
    return static_cast<TReturn &>(*this);
  }

private:
  Entities entities;

  std::unique_ptr<OrderBy> orderby;
  std::vector<std::unique_ptr<Modification>> mods;
  mutable RefEntities ents;
  mutable bool ran_query = false;

  bool _include_store_entities = false;

  EntityQuery &set_order_by(OrderBy *ob) {
    if (orderby) {
      log_error("We only apply the first order by in a query at the moment");
      return static_cast<TReturn &>(*this);
    }
    orderby = std::unique_ptr<OrderBy>(ob);
    return static_cast<TReturn &>(*this);
  }

  RefEntities filter_mod(const RefEntities &in,
                         const std::unique_ptr<Modification> &mod) const {
    RefEntities out;
    out.reserve(in.size());
    for (auto &entity : in) {
      if ((*mod)(entity)) {
        out.push_back(entity);
      }
    }
    return out;
  }

  RefEntities run_query(UnderlyingOptions) const {
    RefEntities out;
    out.reserve(entities.size());

    for (const auto &e_ptr : entities) {
      if (!e_ptr)
        continue;
      Entity &e = *e_ptr;
      out.push_back(e);
    }

    auto it = out.end();
    for (auto &mod : mods) {
      it = std::partition(out.begin(), it, [&mod](const auto &entity) {
        return (*mod)(entity);
      });
    }

    out.erase(it, out.end());

    if (out.size() == 1) {
      return out;
    }

    // TODO :SPEED: if there is only one item no need to sort
    // TODO :SPEED: if we are doing gen_first() then partial sort?
    // Now run any order bys
    if (orderby) {
      std::sort(out.begin(), out.end(), [&](const Entity &a, const Entity &b) {
        return (*orderby)(a, b);
      });
    }

    // ran_query = true;
    return out;
  }
};
