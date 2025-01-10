
#pragma once

#include <bitset>
#include <functional>
#include <random>

namespace bitset_utils {

template <typename T, typename E> void set(T &bitset, E enum_value) {
  bitset.set(static_cast<int>(enum_value));
}

template <typename T, typename E> void reset(T &bitset, E enum_value) {
  bitset.reset(static_cast<int>(enum_value));
}

template <typename T, typename E> bool test(const T &bitset, E enum_value) {
  return bitset.test(static_cast<int>(enum_value));
}

template <typename T> int index_of_nth_set_bit(const T &bitset, int n) {
  int count = 0;
  for (size_t i = 0; i < bitset.size(); ++i) {
    if (bitset.test(i)) {
      if (++count == n) {
        return static_cast<int>(i);
      }
    }
  }
  return -1; // Return -1 if the nth set bit is not found
}

template <size_t N>
int get_random_boolean_bit(const std::bitset<N> &bitset,
                           std::mt19937 &generator, size_t max_value = N,
                           bool value = true) {
  std::vector<int> enabled_indices;
  for (size_t i = 0; i < max_value; ++i) {
    if (bitset.test(i) == value) {
      enabled_indices.push_back(static_cast<int>(i));
    }
  }

  if (enabled_indices.empty()) {
    // No bits are enabled, return -1 or handle the error as needed.
    return -1;
  }
  int random_index = generator() % (static_cast<int>(enabled_indices.size()));
  return enabled_indices[random_index];
}

template <size_t N>
int get_random_enabled_bit(const std::bitset<N> &bitset,
                           std::mt19937 &generator, size_t max_value = N) {
  return get_random_boolean_bit(bitset, generator, max_value, true);
}
template <size_t N>
int get_random_disabled_bit(const std::bitset<N> &bitset,
                            std::mt19937 &generator, size_t max_value = N) {
  return get_random_boolean_bit(bitset, generator, max_value, false);
}

template <size_t N>
int get_first_boolean_bit(const std::bitset<N> &bitset, bool value = true) {
  for (size_t i = 0; i < bitset.size(); ++i) {
    if (bitset.test(i) == value) {
      return (int)i;
    }
  }
  return -1;
}

template <size_t N> int get_first_enabled_bit(const std::bitset<N> &bitset) {
  return get_first_boolean_bit(bitset, true);
}

template <size_t N> int get_first_disabled_bit(const std::bitset<N> &bitset) {
  return get_first_boolean_bit(bitset, false);
}

}; // namespace bitset_utils
