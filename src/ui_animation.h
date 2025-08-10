#pragma once

#include "std_include.h"
#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

#include <afterhours/src/singleton.h>

namespace ui_anim {

enum struct EasingType { Linear, EaseOutQuad, Hold };

enum struct UIAnimKey : size_t {
  MapShuffle,
  Count,
};

struct AnimSegment {
  float to_value = 0.f;
  float duration = 0.f;
  EasingType easing = EasingType::Linear;
};

struct AnimTrack {
  float current = 0.f;
  float from = 0.f;
  float to = 0.f;
  float duration = 0.f;
  float elapsed = 0.f;
  bool active = false;
  EasingType current_easing = EasingType::Linear;
  std::deque<AnimSegment> queue;
  std::function<void()> on_complete;
};

SINGLETON_FWD(UIAnimationManager)
struct UIAnimationManager {
  SINGLETON(UIAnimationManager)

  void update(float dt);
  AnimTrack &ensure_track(UIAnimKey key);
  bool is_active(UIAnimKey key) const;
  std::optional<float> get_value(UIAnimKey key) const;

private:
  std::array<AnimTrack, static_cast<size_t>(UIAnimKey::Count)> tracks;
};

struct AnimHandle {
  UIAnimKey key;
  UIAnimationManager &mgr;

  AnimHandle &from(float value);
  AnimHandle &to(float value, float duration, EasingType easing);
  AnimHandle &sequence(const std::vector<AnimSegment> &segments);
  AnimHandle &hold(float duration);
  AnimHandle &on_complete(std::function<void()> callback);
  float value() const;
  bool is_active() const;
};

inline AnimHandle anim(UIAnimKey key) {
  return AnimHandle{key, UIAnimationManager::get()};
}

} // namespace ui_anim
