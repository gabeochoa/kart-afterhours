#include "ui_animation.h"
#include <afterhours/src/singleton.h>

namespace ui_anim {

static float apply_ease(EasingType easing, float t) {
  t = std::clamp(t, 0.f, 1.f);
  switch (easing) {
  case EasingType::Linear:
    return t;
  case EasingType::EaseOutQuad:
    return 1.f - (1.f - t) * (1.f - t);
  case EasingType::Hold:
    return 0.f; // always at start, effectively holding value
  }
  return t;
}

void UIAnimationManager::update(float dt) {
  for (auto &tr : tracks) {
    if (!tr.active)
      continue;

    if (tr.duration <= 0.f) {
      tr.current = tr.to;
      tr.active = false;
    } else {
      tr.elapsed += dt;
      float u = apply_ease(tr.current_easing, tr.elapsed / tr.duration);
      tr.current = std::lerp(tr.from, tr.to, u);
      if (tr.elapsed >= tr.duration) {
        tr.current = tr.to;
        if (!tr.queue.empty()) {
          auto seg = tr.queue.front();
          tr.queue.pop_front();
          tr.from = tr.current;
          tr.to = seg.to_value;
          tr.duration = seg.duration;
          tr.current_easing = seg.easing;
          tr.elapsed = 0.f;
          tr.active = true;
        } else {
          tr.active = false;
          if (tr.on_complete)
            tr.on_complete();
        }
      }
    }
  }
}

AnimTrack &UIAnimationManager::ensure_track(UIAnimKey key) {
  return tracks[static_cast<size_t>(key)];
}

bool UIAnimationManager::is_active(UIAnimKey key) const {
  return tracks[static_cast<size_t>(key)].active;
}

std::optional<float> UIAnimationManager::get_value(UIAnimKey key) const {
  const AnimTrack &tr = tracks[static_cast<size_t>(key)];
  if (!tr.active)
    return std::nullopt;
  return tr.current;
}

AnimHandle &AnimHandle::from(float value) {
  AnimTrack &tr = mgr.ensure_track(key);
  tr.current = value;
  tr.from = value;
  tr.to = value;
  tr.duration = 0.f;
  tr.elapsed = 0.f;
  tr.active = false;
  tr.queue.clear();
  tr.on_complete = nullptr;
  return *this;
}

AnimHandle &AnimHandle::to(float value, float duration, EasingType easing) {
  AnimTrack &tr = mgr.ensure_track(key);
  if (!tr.active && tr.queue.empty()) {
    tr.from = tr.current;
    tr.to = value;
    tr.duration = duration;
    tr.current_easing = easing;
    tr.elapsed = 0.f;
    tr.active = true;
  } else {
    tr.queue.push_back(
        AnimSegment{.to_value = value, .duration = duration, .easing = easing});
  }
  return *this;
}

AnimHandle &AnimHandle::sequence(const std::vector<AnimSegment> &segments) {
  AnimTrack &tr = mgr.ensure_track(key);
  if (segments.empty())
    return *this;
  // Start first if idle
  if (!tr.active && tr.queue.empty()) {
    tr.from = tr.current;
    tr.to = segments[0].to_value;
    tr.duration = segments[0].duration;
    tr.elapsed = 0.f;
    tr.active = true;
    for (size_t i = 1; i < segments.size(); ++i)
      tr.queue.push_back(segments[i]);
  } else {
    for (auto &s : segments)
      tr.queue.push_back(s);
  }
  return *this;
}

AnimHandle &AnimHandle::hold(float duration) {
  AnimTrack &tr = mgr.ensure_track(key);
  // Queue a no-op segment with Hold easing so the value stays constant
  tr.queue.push_back(AnimSegment{.to_value = tr.current,
                                 .duration = duration,
                                 .easing = EasingType::Hold});
  return *this;
}

AnimHandle &AnimHandle::on_complete(std::function<void()> callback) {
  AnimTrack &tr = mgr.ensure_track(key);
  tr.on_complete = std::move(callback);
  return *this;
}

float AnimHandle::value() const {
  auto v = mgr.get_value(key);
  return v.value_or(0.f);
}

bool AnimHandle::is_active() const { return mgr.is_active(key); }

} // namespace ui_anim
