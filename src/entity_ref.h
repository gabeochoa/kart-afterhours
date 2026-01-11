#pragma once

#include <afterhours/ah.h>
#include <optional>

struct EntityRef {
    afterhours::EntityID id = -1;
    std::optional<afterhours::EntityHandle> handle{};

    [[nodiscard]] bool has_value() const { return id >= 0; }
    [[nodiscard]] bool empty() const { return !has_value(); }
    
    void clear() {
        id = -1;
        handle.reset();
    }

    void set(afterhours::Entity& e) {
        id = e.id;
        const afterhours::EntityHandle h = afterhours::EntityHelper::handle_for(e);
        if (h.valid()) {
            handle = h;
        } else {
            handle.reset();
        }
    }

    [[nodiscard]] afterhours::OptEntity resolve() const {
        if (handle.has_value()) {
            afterhours::OptEntity opt = afterhours::EntityHelper::resolve(*handle);
            if (opt) return opt;
        }
        if (id >= 0) {
            return afterhours::EntityHelper::getEntityForID(id);
        }
        return {};
    }
};

