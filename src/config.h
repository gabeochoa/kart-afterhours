
#pragma once

#include "afterhours/src/singleton.h"

SINGLETON_FWD(Config)
struct Config {
  SINGLETON(Config)

  template <typename T> struct ValueInRange {
    T data;
    T min;
    T max;

    ValueInRange(T default_, T min_, T max_)
        : data(default_), min(min_), max(max_) {}

    void operator=(ValueInRange<T> &new_value) { set(new_value.data); }

    void set(T &nv) { data = std::min(max, std::max(min, nv)); }

    void set_pct(const float &pct) {
      // lerp
      T nv = min + pct * (max - min);
      set(nv);
    }

    float get_pct() const { return (data - min) / (max - min); }

    operator T() { return data; }

    operator T() const { return data; }
    operator T &() { return data; }
  };

  ValueInRange<float> max_speed{10.f, 1.f, 20.f};
  ValueInRange<float> skid_threshold{98.5f, 0.f, 100.f};
  ValueInRange<float> steering_sensitivity{2.f, 1.f, 10.f};
};
