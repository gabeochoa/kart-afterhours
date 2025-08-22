#include "../config.h"
#include "../ui_systems.h"

using namespace afterhours;

bool ScheduleDebugUI::should_run(float dt) {
  enableCooldown -= dt;

  if (enableCooldown < 0) {
    enableCooldown = enableCooldownReset;
      input::PossibleInputCollector inpc =
      input::get_input_collector();

    bool debug_pressed =
        std::ranges::any_of(inpc.inputs(), [](const auto &actions_done) {
          return action_matches(actions_done.action, InputAction::ToggleUIDebug);
        });
    if (debug_pressed) {
      enabled = !enabled;
    }
  }
  return enabled;
}

void ScheduleDebugUI::for_each_with(Entity &entity,
                                    UIContext<InputAction> &context, float) {

  if (!enabled) {
    return;
  }

  struct SliderSpec {
    const char *debug_name;
    std::function<std::string()> make_label;
    std::function<float()> get_pct;
    std::function<void(float)> set_pct;
  };

  const std::array<SliderSpec, 11> all_specs{
      SliderSpec{"max_speed",
                 []() {
                   return fmt::format("Max Speed\n {:.2f} m/s",
                                      Config::get().max_speed.data);
                 },
                 []() { return Config::get().max_speed.get_pct(); },
                 [](float value) { Config::get().max_speed.set_pct(value); }},
      SliderSpec{"breaking_acceleration",
                 []() {
                   return fmt::format("Breaking \nPower \n -{:.2f} m/s^2",
                                      Config::get().breaking_acceleration.data);
                 },
                 []() { return Config::get().breaking_acceleration.get_pct(); },
                 [](float value) {
                   Config::get().breaking_acceleration.set_pct(value);
                 }},
      SliderSpec{"forward_acceleration",
                 []() {
                   return fmt::format("Forward \nAcceleration \n {:.2f} m/s^2",
                                      Config::get().forward_acceleration.data);
                 },
                 []() { return Config::get().forward_acceleration.get_pct(); },
                 [](float value) {
                   Config::get().forward_acceleration.set_pct(value);
                 }},
      SliderSpec{"reverse_acceleration",
                 []() {
                   return fmt::format("Reverse \nAcceleration \n {:.2f} m/s^2",
                                      Config::get().reverse_acceleration.data);
                 },
                 []() { return Config::get().reverse_acceleration.get_pct(); },
                 [](float value) {
                   Config::get().reverse_acceleration.set_pct(value);
                 }},
      SliderSpec{
          "boost_acceleration",
          []() {
            return fmt::format("Boost \nAcceleration \n {:.2f} m/s^2",
                               Config::get().boost_acceleration.data);
          },
          []() { return Config::get().boost_acceleration.get_pct(); },
          [](float value) { Config::get().boost_acceleration.set_pct(value); }},
      SliderSpec{"boost_decay_percent",
                 []() {
                   return fmt::format("Boost \nDecay \n {:.2f} decay%/frame",
                                      Config::get().boost_decay_percent.data);
                 },
                 []() { return Config::get().boost_decay_percent.get_pct(); },
                 [](float value) {
                   Config::get().boost_decay_percent.set_pct(value);
                 }},
      SliderSpec{
          "skid_threshold",
          []() {
            return fmt::format("Skid \nThreshold \n {:.2f} %",
                               Config::get().skid_threshold.data);
          },
          []() { return Config::get().skid_threshold.get_pct(); },
          [](float value) { Config::get().skid_threshold.set_pct(value); }},
      SliderSpec{"steering_sensitivity",
                 []() {
                   return fmt::format("Steering \nSensitivity \n {:.2f} %",
                                      Config::get().steering_sensitivity.data);
                 },
                 []() { return Config::get().steering_sensitivity.get_pct(); },
                 [](float value) {
                   Config::get().steering_sensitivity.set_pct(value);
                 }},
      SliderSpec{
          "minimum_steering_radius",
          []() {
            return fmt::format("Min Steering \nSensitivity \n {:.2f} m",
                               Config::get().minimum_steering_radius.data);
          },
          []() { return Config::get().minimum_steering_radius.get_pct(); },
          [](float value) {
            Config::get().minimum_steering_radius.set_pct(value);
          }},
      SliderSpec{
          "maximum_steering_radius",
          []() {
            return fmt::format("Max Steering \nSensitivity \n {:.2f} m",
                               Config::get().maximum_steering_radius.data);
          },
          []() { return Config::get().maximum_steering_radius.get_pct(); },
          [](float value) {
            Config::get().maximum_steering_radius.set_pct(value);
          }},
      SliderSpec{
          "collision_scalar",
          []() {
            return fmt::format("Collision \nScalar \n {:.4f}",
                               Config::get().collision_scalar.data);
          },
          []() { return Config::get().collision_scalar.get_pct(); },
          [](float value) { Config::get().collision_scalar.set_pct(value); }},
  };

  auto screen_container =
      imm::div(context, mk(entity),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(0.5f)})
                   .with_absolute_position()
                   .with_debug_name("debug_screen_container"));

  const int items_per_row = 3;
  const int num_rows =
      static_cast<int>((all_specs.size() + items_per_row - 1) / items_per_row);
  for (int row = 0; row < num_rows; ++row) {
    const int start = row * items_per_row;
    const int remaining = static_cast<int>(all_specs.size()) - start;
    if (remaining <= 0)
      break;
    const int count_in_row = std::min(items_per_row, remaining);
    const float row_height = 1.f / static_cast<float>(num_rows);

    auto row_elem = imm::div(
        context, mk(screen_container.ent(), row),
        ComponentConfig{}
            .with_size(ComponentSize{percent(1.f), percent(row_height)})
            .with_flex_direction(FlexDirection::Row));

    for (int j = 0; j < count_in_row; ++j) {
      const auto &spec = all_specs[start + j];
      float pct = spec.get_pct();
      auto label = spec.make_label();
      if (auto result =
              slider(context, mk(row_elem.ent(), row * items_per_row + j), pct,
                     ComponentConfig{}
                         .with_size(ComponentSize{pixels(200.f), pixels(50.f)})
                         .with_label(std::move(label))
                         .with_skip_tabbing(true));
          result) {
        spec.set_pct(result.as<float>());
      }
    }
  }
}
