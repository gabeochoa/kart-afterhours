#pragma once

#include "components.h"
#include "game_state_manager.h"
#include "query.h"
#include "round_settings.h"
#include "sound_library.h"
#include <afterhours/ah.h>
#include <fmt/format.h>

struct UpdateRoundCountdown : PausableSystem<> {
  virtual void once(float dt) override {
    if (!RoundManager::get().uses_timer()) {
      return;
    }

    if (!GameStateManager::get().is_game_active()) {
      return;
    }

    auto &settings = RoundManager::get().get_active_settings();
    if (settings.state != RoundSettings::GameState::Countdown) {
      return;
    }

    settings.countdown_before_start -= dt;

    if (settings.countdown_before_start < 0.05f &&
        settings.countdown_before_start > 0.03f) {
      auto opt = EntityQuery({.force_merge = true})
                     .whereHasComponent<SoundEmitter>()
                     .gen_first();
      if (opt.valid()) {
        auto &ent = opt.asE();
        auto &req = ent.addComponentIfMissing<PlaySoundRequest>();
        req.policy = PlaySoundRequest::Policy::Enum;
        req.file = SoundFile::Round_Start;
      }
    }

    if (settings.countdown_before_start > 0) {
      return;
    }

    settings.countdown_before_start = 0;
    settings.state = RoundSettings::GameState::InGame;
  }
};

struct RenderRoundTimer : System<window_manager::ProvidesCurrentResolution> {
  virtual void for_each_with(const Entity &,
                             const window_manager::ProvidesCurrentResolution &,
                             float) const override {
    if (!RoundManager::get().uses_timer()) {
      return;
    }
    if (!GameStateManager::get().is_game_active()) {
      return;
    }
    const int screen_width = raylib::GetScreenWidth();
    const int screen_height = raylib::GetScreenHeight();
    const float timer_x = screen_width * 0.5f;
    const float timer_y = screen_height * 0.07f;
    const float text_size = screen_height * 0.033f;
    const auto &settings = RoundManager::get().get_active_settings();

    if (settings.state == RoundSettings::GameState::Countdown &&
        settings.show_countdown_timer && settings.countdown_before_start > 0) {
      std::string countdown_text =
          fmt::format("Get Ready! {:.0f}", settings.countdown_before_start);
      const float text_width = raylib::MeasureText(countdown_text.c_str(),
                                                   static_cast<int>(text_size));
      raylib::DrawText(countdown_text.c_str(),
                       static_cast<int>(timer_x - text_width / 2.0f),
                       static_cast<int>(timer_y + screen_height * 0.056f),
                       static_cast<int>(text_size), raylib::YELLOW);
      return;
    }

    float current_time = RoundManager::get().get_current_round_time();
    if (current_time <= 0) {
      return;
    }
    std::string timer_text;
    if (current_time >= 60.0f) {
      int minutes = static_cast<int>(current_time) / 60;
      int seconds = static_cast<int>(current_time) % 60;
      timer_text = fmt::format("{}:{:02d}", minutes, seconds);
    } else {
      timer_text = fmt::format("{:.1f}s", current_time);
    }
    const float text_width =
        raylib::MeasureText(timer_text.c_str(), static_cast<int>(text_size));
    raylib::DrawText(
        timer_text.c_str(), static_cast<int>(timer_x - text_width / 2.0f),
        static_cast<int>(timer_y), static_cast<int>(text_size), raylib::WHITE);
  }
};
