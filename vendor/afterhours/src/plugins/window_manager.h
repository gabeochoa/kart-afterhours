
#pragma once

#include "../base_component.h"
#include "../developer.h"
#include "../entity_query.h"
#include "../system.h"

namespace afterhours {

struct window_manager : developer::Plugin {

  struct Resolution {
    int width;
    int height;

    friend std::ostream &operator<<(std::ostream &os, const Resolution &rez) {
      os << "(" << rez.width << "," << rez.height << ")";
      return os;
    }

    bool operator==(const Resolution &other) const {
      return width == other.width && height == other.height;
    }

    bool operator<(const Resolution &r) const {
      return width * height < r.width * r.height;
    }

    operator std::string() const {
      std::ostringstream oss;
      oss << "(" << width << "," << height << ")";
      return oss.str();
    }
  };

#ifdef AFTER_HOURS_USE_RAYLIB
  static std::vector<Resolution> fetch_available_resolutions() {
    int count = 0;
    const GLFWvidmode *modes =
        glfwGetVideoModes(glfwGetPrimaryMonitor(), &count);
    std::vector<window_manager::Resolution> resolutions;

    for (int i = 0; i < count; i++) {
      resolutions.push_back(
          window_manager::Resolution{modes[i].width, modes[i].height});
    }
    // Sort the vector
    std::sort(resolutions.begin(), resolutions.end());

    // Remove duplicates
    resolutions.erase(std::unique(resolutions.begin(), resolutions.end()),
                      resolutions.end());

    return resolutions;
  }
  static Resolution fetch_current_resolution() {
    return Resolution{.width = raylib::GetRenderWidth(),
                      .height = raylib::GetRenderHeight()};
  }

  static void set_window_size(int width, int height) {
    raylib::SetWindowSize(width, height);
  }
#else
  static std::vector<Resolution> fetch_available_resolutions() {
    return std::vector<Resolution>{{
        Resolution{.width = 1280, .height = 720},
        Resolution{.width = 1920, .height = 1080},
    }};
  }
  static Resolution fetch_current_resolution() {
    return Resolution{.width = 1280, .height = 720};
  }
  static void set_window_size(int, int) {}
#endif

  struct ProvidesCurrentResolution : public BaseComponent {
    bool should_refetch = true;
    Resolution current_resolution;
    ProvidesCurrentResolution() {}
    ProvidesCurrentResolution(const Resolution &rez) : current_resolution(rez) {
      should_refetch = false;
    }
    [[nodiscard]] int width() const { return current_resolution.width; }
    [[nodiscard]] int height() const { return current_resolution.height; }
  };

  struct CollectCurrentResolution : System<ProvidesCurrentResolution> {

    virtual void for_each_with(Entity &, ProvidesCurrentResolution &pCR,
                               float) override {
      if (pCR.should_refetch) {
        pCR.current_resolution = fetch_current_resolution();
        pCR.should_refetch = false;
      }
    }
  };

  struct ProvidesTargetFPS : public BaseComponent {
    int fps;
    ProvidesTargetFPS(int f) : fps(f) {}
  };

  struct ProvidesAvailableWindowResolutions : BaseComponent {
    bool should_refetch = true;
    std::vector<Resolution> available_resolutions;
    ProvidesAvailableWindowResolutions() {}
    ProvidesAvailableWindowResolutions(const std::vector<Resolution> &rez)
        : available_resolutions(rez) {
      should_refetch = false;
    }

    const std::vector<Resolution> &fetch_data() const {
      return available_resolutions;
    }

    void on_data_changed(size_t index) {
      auto &entity = EntityQuery()
                         .whereHasComponent<ProvidesCurrentResolution>()
                         .gen_first_enforce();
      ProvidesCurrentResolution &pcr = entity.get<ProvidesCurrentResolution>();
      pcr.current_resolution = available_resolutions[index];
      set_window_size(pcr.current_resolution.width,
                      pcr.current_resolution.height);
    }
  };

  struct CollectAvailableResolutions
      : System<ProvidesAvailableWindowResolutions> {

    virtual void for_each_with(Entity &,
                               ProvidesAvailableWindowResolutions &pAWR,
                               float) override {
      if (pAWR.should_refetch) {
        pAWR.available_resolutions = fetch_available_resolutions();
        pAWR.should_refetch = false;
      }
    }
  };

  static void add_singleton_components(Entity &entity, int target_fps) {
    entity.addComponent<ProvidesTargetFPS>(target_fps);
    entity.addComponent<ProvidesCurrentResolution>();
    entity.addComponent<ProvidesAvailableWindowResolutions>();
  }

  static void add_singleton_components(Entity &entity, const Resolution &rez,
                                       int target_fps) {
    entity.addComponent<ProvidesTargetFPS>(target_fps);
    entity.addComponent<ProvidesCurrentResolution>(rez);
    entity.addComponent<ProvidesAvailableWindowResolutions>();
  }

  static void add_singleton_components(
      Entity &entity, const Resolution &rez, int target_fps,
      const std::vector<Resolution> &available_resolutions) {
    entity.addComponent<ProvidesTargetFPS>(target_fps);
    entity.addComponent<ProvidesCurrentResolution>(rez);
    entity.addComponent<ProvidesAvailableWindowResolutions>(
        available_resolutions);
  }

  static void enforce_singletons(SystemManager &sm) {
    sm.register_update_system(
        std::make_unique<
            developer::EnforceSingleton<ProvidesCurrentResolution>>());
    sm.register_update_system(
        std::make_unique<developer::EnforceSingleton<ProvidesTargetFPS>>());
    sm.register_update_system(
        std::make_unique<
            developer::EnforceSingleton<ProvidesAvailableWindowResolutions>>());
  }

  static void register_update_systems(SystemManager &sm) {
    sm.register_update_system(std::make_unique<CollectCurrentResolution>());
    sm.register_update_system(std::make_unique<CollectAvailableResolutions>());
  }
};
} // namespace afterhours
