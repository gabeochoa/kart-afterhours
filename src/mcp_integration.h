#pragma once

#ifdef AFTER_HOURS_ENABLE_MCP

#include "game.h"
#include "input_mapping.h"
#include "rl.h"
#include "settings.h"
#include <afterhours/src/plugins/input_system.h>
#include <afterhours/src/plugins/mcp_server.h>
#include <afterhours/src/plugins/ui/components.h>
#include <set>
#include <sstream>

namespace mcp_integration {

namespace detail {
inline bool enabled = false;
inline std::set<int> keys_down;
inline std::set<int> keys_pressed_this_frame;
inline std::set<int> keys_released_this_frame;
inline raylib::Vector2 mouse_position = {0, 0};
inline bool mouse_clicked = false;
inline int mouse_button_clicked = 0;
inline std::map<int, afterhours::input::ValidInputs> action_mapping;

inline std::vector<uint8_t> capture_screenshot() {
    raylib::Image img = raylib::LoadImageFromTexture(screenRT.texture);
    raylib::ImageFlipVertical(&img);
    
    int file_size = 0;
    unsigned char* png_data = raylib::ExportImageToMemory(img, "png", &file_size);
    
    std::vector<uint8_t> result;
    if (png_data && file_size > 0) {
        result.assign(png_data, png_data + file_size);
        raylib::MemFree(png_data);
    }
    
    raylib::UnloadImage(img);
    return result;
}

inline std::pair<int, int> get_screen_size() {
    return {Settings::get_screen_width(), Settings::get_screen_height()};
}

inline std::string dump_ui_tree() {
    std::ostringstream ss;
    ss << "UI Tree Dump:\n";
    
    auto entities = afterhours::EntityQuery()
        .whereHasComponent<afterhours::ui::UIComponent>()
        .gen();
    
    for (auto& entity : entities) {
        auto& uic = entity.get().get<afterhours::ui::UIComponent>();
        if (!uic.was_rendered_to_screen) continue;
        
        auto r = uic.rect();
        ss << "  Entity " << entity.get().id << ": ";
        ss << "rect(" << r.x << "," << r.y << "," << r.width << "," << r.height << ")";
        
        if (entity.get().has<afterhours::ui::HasLabel>()) {
            auto& label = entity.get().get<afterhours::ui::HasLabel>();
            ss << " label=\"" << label.label << "\"";
        }
        
        ss << "\n";
    }
    
    return ss.str();
}

inline int find_action_for_key(int keycode) {
    for (const auto& [action, inputs] : action_mapping) {
        for (const auto& input : inputs) {
            if (input.index() == 0 && std::get<0>(input) == keycode) {
                return action;
            }
        }
    }
    return -1;
}

inline afterhours::mcp::MCPConfig create_config() {
    afterhours::mcp::MCPConfig config;
    
    config.get_screen_size = get_screen_size;
    config.capture_screenshot = capture_screenshot;
    config.dump_ui_tree = dump_ui_tree;
    
    config.mouse_move = [](int x, int y) {
        mouse_position = {static_cast<float>(x), static_cast<float>(y)};
    };
    
    config.mouse_click = [](int x, int y, int button) {
        mouse_position = {static_cast<float>(x), static_cast<float>(y)};
        mouse_clicked = true;
        mouse_button_clicked = button;
    };
    
    config.key_down = [](int keycode) {
        if (keys_down.count(keycode) == 0) {
            keys_pressed_this_frame.insert(keycode);
        }
        keys_down.insert(keycode);
    };
    
    config.key_up = [](int keycode) {
        keys_down.erase(keycode);
        keys_released_this_frame.insert(keycode);
    };
    
    return config;
}

struct InjectInputSystem : afterhours::System<afterhours::input::InputCollector> {
    virtual void for_each_with(afterhours::Entity&, 
                               afterhours::input::InputCollector& collector,
                               float dt) override {
        if (!enabled) return;
        
        for (int keycode : keys_down) {
            int action = find_action_for_key(keycode);
            if (action >= 0) {
                collector.inputs.push_back(
                    afterhours::input::ActionDone(
                        afterhours::input::DeviceMedium::Keyboard, 
                        0, action, 1.0f, dt));
            }
        }
        
        for (int keycode : keys_pressed_this_frame) {
            int action = find_action_for_key(keycode);
            if (action >= 0) {
                collector.inputs_pressed.push_back(
                    afterhours::input::ActionDone(
                        afterhours::input::DeviceMedium::Keyboard, 
                        0, action, 1.0f, dt));
            }
        }
    }
};
}

inline void init() {
    detail::enabled = true;
    detail::action_mapping = get_mapping();
    afterhours::mcp::init(detail::create_config());
}

inline void register_systems(afterhours::SystemManager& systems) {
    systems.register_update_system(std::make_unique<detail::InjectInputSystem>());
}

inline void update() {
    if (!detail::enabled) return;
    afterhours::mcp::update();
}

inline void clear_frame_state() {
    if (!detail::enabled) return;
    detail::keys_pressed_this_frame.clear();
    detail::keys_released_this_frame.clear();
    detail::mouse_clicked = false;
}

inline void shutdown() {
    if (!detail::enabled) return;
    afterhours::mcp::shutdown();
    detail::enabled = false;
}

inline bool exit_requested() {
    if (!detail::enabled) return false;
    return afterhours::mcp::exit_requested();
}

}

#else

namespace mcp_integration {
inline void init() {}
inline void register_systems(afterhours::SystemManager&) {}
inline void update() {}
inline void clear_frame_state() {}
inline void shutdown() {}
inline bool exit_requested() { return false; }
}

#endif
