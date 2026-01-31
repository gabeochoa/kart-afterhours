#pragma once

#include "e2e_commands.h"
#include "game.h"
#include "input_mapping.h"
#include <afterhours/src/plugins/e2e_testing/e2e_testing.h>

namespace e2e_integration {

using namespace afterhours;
using namespace afterhours::testing;

namespace detail {
inline bool enabled = false;
inline std::unique_ptr<E2ERunner> runner;
}

inline void init(const std::string& script_dir = "tests/e2e/") {
    detail::enabled = true;
    test_input::detail::test_mode = true;
    
    detail::runner = std::make_unique<E2ERunner>();
    detail::runner->set_timeout(30.0f);
    
    detail::runner->set_screenshot_callback([](const std::string& name) {
        std::string path = "screenshots/" + name + ".png";
        raylib::Image img = raylib::LoadImageFromScreen();
        raylib::ExportImage(img, path.c_str());
        raylib::UnloadImage(img);
        log_info("E2E: Screenshot saved to {}", path);
    });
    
    detail::runner->set_reset_callback([]() {
        test_input::reset_all();
        VisibleTextRegistry::instance().clear();
        
        auto *ctx = EntityHelper::get_singleton_cmp<ui::UIContext<InputAction>>();
        if (ctx) {
            ctx->reset();
        }
    });
    
    detail::runner->load_scripts_from_directory(script_dir);
    
    if (detail::runner->has_commands()) {
        log_info("E2E: Loaded scripts from {}", script_dir);
    } else {
        log_warn("E2E: No scripts found in {}", script_dir);
    }
}

inline bool is_enabled() {
    return detail::enabled;
}

inline void register_systems(SystemManager& sm) {
    if (!detail::enabled) return;
    
    register_builtin_handlers(sm);
    
    sm.register_update_system(
        std::make_unique<HandleScreenshotCommand>([](const std::string& name) {
            std::string path = "screenshots/" + name + ".png";
            raylib::Image img = raylib::LoadImageFromScreen();
            raylib::ExportImage(img, path.c_str());
            raylib::UnloadImage(img);
            log_info("E2E: Screenshot saved to {}", path);
        }));
    
    sm.register_update_system(
        std::make_unique<HandleResetTestStateCommand>([]() {
            test_input::reset_all();
            VisibleTextRegistry::instance().clear();
        }));
    
    e2e_commands::register_app_commands(sm);
    
    register_unknown_handler(sm);
    register_cleanup(sm);
}

inline bool tick(float dt) {
    if (!detail::enabled) return true;
    
    test_input::reset_frame();
    
    if (detail::runner) {
        detail::runner->tick(dt);
        EntityHelper::get_default_collection().merge_entity_arrays();
    }
    
    return true;
}

inline void post_render(float) {
}

inline bool should_exit() {
    if (!detail::enabled || !detail::runner) return false;
    return detail::runner->is_finished();
}

inline bool has_failed() {
    if (!detail::runner) return false;
    return detail::runner->has_failed();
}

inline void print_results() {
    if (!detail::runner) return;
    detail::runner->print_results();
}

}
