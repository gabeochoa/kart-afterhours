#pragma once

#include "../vendor/afterhours/src/plugins/animation.h"
#include <algorithm>
#include <cstddef>
#include <optional>
#include <unordered_set>

enum struct UIKey : size_t {
  MapShuffle,
  MapCard,
  MapCardPulse,
  MapPreviewFade,
  RoundEndCard,
  RoundEndWinnerPulse,
  RoundEndScore,
  SlideInAll,
};

namespace ui_anims {

// Helpers bridging kart's old animation::anim/get_value/clamp_value/one_shot
// usage onto afterhours::motion tracks, preserving the old observable
// semantics:
//
// - Old get_value() returned nullopt when a track was not *active* (never
//   started, or finished). motion::Track keeps its final value after it
//   finishes, so active_value() re-imposes the old rule: only an active
//   track has a value.
// - Old clamp_value() used the max as the fallback for an inactive track,
//   then clamped. clamp_value() below does the same.
// - Old one_shot() ran its builder once per key for the whole process
//   (a function-local static set that was never cleared). one_shot()
//   below keeps that once-ever rule, keyed the same way as track_for().
// - motion::anim(key) alone cannot distinguish per-index tracks: it salts
//   only by the key type. Indexed tracks therefore go through the root
//   entity's HasTracks map under a composite key, mirroring the old
//   CompositeKey manager. Per-entity tracks (slide-in, wiggle) keep
//   using motion::anim(key, entity_id), which is distinct per entity.

inline size_t composite_key(UIKey key, size_t index) {
  size_t h = (static_cast<size_t>(key) + 1) * 1469598103934665603ull;
  h ^= index + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
  return h;
}

inline afterhours::motion::Track<float> &track_for(UIKey key) {
  return afterhours::motion::anim(key);
}

inline afterhours::motion::Track<float> &track_for(UIKey key, size_t index) {
  return afterhours::motion::root_entity()
      .addComponentIfMissing<afterhours::motion::HasTracks>()
      .track<float>(composite_key(key, index));
}

inline std::optional<float>
active_value(afterhours::motion::Track<float> &track) {
  if (!track.active())
    return std::nullopt;
  return track.value();
}

inline float clamp_value(afterhours::motion::Track<float> &track, float mn,
                         float mx) {
  float x = track.active() ? track.value() : mx;
  return std::clamp(x, mn, mx);
}

template <typename Fn> void one_shot(UIKey key, size_t index, Fn &&fn) {
  static std::unordered_set<size_t> started;
  const size_t k = composite_key(key, index);
  if (started.contains(k))
    return;
  started.insert(k);
  fn(track_for(key, index));
}

// A single eased step: progress 0 -> 1 over `duration`, eased with
// `curve` (nullptr = linear). motion::Timeline interpolates progress
// between the step's start and target values, not values themselves,
// so a multi-value sequence is a chain of these steps.
inline afterhours::motion::Timeline eased_step(float duration,
                                                float (*curve)(float)) {
  return afterhours::motion::Timeline{.keys = {{0.f, 0.f}, {duration, 1.f}},
                                      .curve = curve};
}

inline auto make_map_card_slide(size_t i) {
  return [i](auto &h) {
    float delay = 0.04f * static_cast<float>(i);
    h.from(0.0f)
        .to(0.0f, eased_step(delay, nullptr))
        .then(1.1f, eased_step(0.18f, afterhours::motion::curves::ease_out_quad))
        .then(1.0f, eased_step(0.08f, afterhours::motion::curves::ease_out_quad));
  };
}

inline auto make_round_end_card_stagger(size_t i) {
  return [i](auto &h) {
    float delay = 0.05f * static_cast<float>(i);
    h.from(0.0f)
        .to(0.0f, eased_step(delay, nullptr))
        .then(1.0f, eased_step(0.25f, afterhours::motion::curves::ease_out_quad));
  };
}

} // namespace ui_anims
