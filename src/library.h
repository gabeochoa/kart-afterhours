
#pragma once

#include <expected>
#include <map>
#include <utility>

#include "afterhours/src/type_name.h"
#include "log.h"

template <typename T> struct Library {
  enum struct Error {
    DUPLICATE_NAME,
    NO_MATCH,
  };

  using Storage = std::map<std::string, T>;
  using iterator = typename Storage::iterator;
  using const_iterator = typename Storage::const_iterator;

  Storage storage;

  [[nodiscard]] auto size() const { return storage.size(); }
  [[nodiscard]] auto begin() const { return storage.begin(); }
  [[nodiscard]] auto end() const { return storage.end(); }
  [[nodiscard]] auto begin() { return storage.begin(); }
  [[nodiscard]] auto end() { return storage.end(); }
  [[nodiscard]] auto rbegin() const { return storage.rbegin(); }
  [[nodiscard]] auto rend() const { return storage.rend(); }
  [[nodiscard]] auto rbegin() { return storage.rbegin(); }
  [[nodiscard]] auto rend() { return storage.rend(); }
  [[nodiscard]] auto empty() const { return storage.empty(); }

  std::expected<std::string, Error> add(const char *name, const T &item) {
    log_trace("adding {} to the library", name);
    if (storage.find(name) != storage.end()) {
      return std::unexpected(Error::DUPLICATE_NAME);
    }
    storage[name] = item;
    return name;
  }

  [[nodiscard]] T &get(const std::string &name) {
    if (!this->contains(name)) {
      log_warn("asking for item: {} but nothing has been loaded with that "
               "name yet {}",
               name, type_name<T>());
    }
    return storage.at(name);
  }

  [[nodiscard]] const T &get(const std::string &name) const {
    if (!this->contains(name)) {
      log_warn("asking for item: {} but nothing has been loaded with that "
               "name yet for {}",
               name, type_name<T>());
    }
    return storage.at(name);
  }

  [[nodiscard]] bool contains(const std::string &name) const {
    return (storage.find(name) != storage.end());
  }

  virtual void load(const char *filename, const char *name) {
    log_trace("Loading {}: {} from {}", type_name<T>(), name, filename);
    this->add(name, convert_filename_to_object(name, filename));
  }

  virtual T convert_filename_to_object(const char *name,
                                       const char *filename) = 0;

  void unload_all() {
    log_info("Library<{}> loaded {} items", type_name<T>(), storage.size());
    for (auto kv : storage) {
      unload(kv.second);
    }
  }
  virtual void unload(T) = 0;

  [[nodiscard]] std::expected<T, Error>
  get_random_match(const std::string &key) const {
    auto matches = lookup(key);
    size_t num_matches = std::distance(matches.first, matches.second);
    if (num_matches == 0) {
      log_warn("got no matches for your prefix search: {}", key);
      return std::unexpected(Error::NO_MATCH);
    }

    // TODO add random generator
    // int idx = RandomEngine::get().get_int(0, static_cast<int>(num_matches) -
    // 1);
    int idx = 0;
    const_iterator start(matches.first);
    std::advance(start, idx);
    return start->second;
  }

  [[nodiscard]] auto lookup(const std::string &key) const
      -> std::pair<const_iterator, const_iterator> {
    auto p = storage.lower_bound(key);
    auto q = storage.end();
    if (p != q && p->first == key) {
      return std::make_pair(p, std::next(p));
    } else {
      auto r = p;
      while (r != q && r->first.compare(0, key.size(), key) == 0) {
        ++r;
      }
      return std::make_pair(p, r);
    }
  }
};
