
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

    void operator=(const ValueInRange<T> &new_value) { set(new_value.data); }

    void set(const T &nv) { data = std::min(max, std::max(min, nv)); }

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
  ValueInRange<float> breaking_acceleration{1.f, 1.75f, 10.f};
  ValueInRange<float> forward_acceleration{4.f, 1.f, 10.f};
  ValueInRange<float> reverse_acceleration{1.75f, 1.f, 10.f};
  ValueInRange<float> boost_acceleration{5.f, 2.f, 50.f};
  ValueInRange<float> boost_decay_percent{1.f, 0.01f, 10.f};
  ValueInRange<float> skid_threshold{98.5f, 0.f, 100.f};
  ValueInRange<float> steering_sensitivity{1.1f, .1f, 2.f};
  ValueInRange<float> minimum_steering_radius{10.f, 1.f, 50.f};
  ValueInRange<float> maximum_steering_radius{300.f, 50.f, 300.f};
  ValueInRange<float> machine_gun_fire_rate{25.f, 5.f, 100.f};
  ValueInRange<float> collision_scalar{250.f, 1.f, 1000.f};
};
