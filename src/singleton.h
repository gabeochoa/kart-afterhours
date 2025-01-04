#pragma once

#include <memory>

#define SINGLETON_FWD(type)                                                    \
  struct type;                                                                 \
  static std::shared_ptr<type> type##_single;

#define SINGLETON(type)                                                        \
  inline static type &create() {                                               \
    if (!type##_single)                                                        \
      type##_single.reset(new type());                                         \
    return *type##_single;                                                     \
  }                                                                            \
  [[nodiscard]] inline static type &get() { return create(); }

#define SINGLETON_PARAM(type, param_type)                                      \
  inline static type *create(const param_type &pt) {                           \
    if (!type##_single)                                                        \
      type##_single.reset(new type(pt));                                       \
    return type##_single.get();                                                \
  }                                                                            \
  [[nodiscard]] inline static type &get() { return *type##_single; }
