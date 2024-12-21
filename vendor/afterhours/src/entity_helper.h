#pragma once

#include <algorithm>
#include <set>
#include <vector>
#include <functional>
#include <memory>

#include "entity.h"

using Entities = std::vector<std::shared_ptr<Entity>>;
using RefEntities = std::vector<RefEntity>;

static Entities entities_DO_NOT_USE;
static std::set<int> permanant_ids;

struct EntityHelper {
    struct CreationOptions {
        bool is_permanent;
    };

    static const Entities &get_entities();
    static Entities &get_entities_for_mod();
    static RefEntities get_ref_entities();

    static Entity &createEntity();
    static Entity &createPermanentEntity();
    static Entity &createEntityWithOptions(const CreationOptions &options);

    static void markIDForCleanup(int e_id);
    static void removeEntity(int e_id);
    static void cleanup();
    static void delete_all_entities_NO_REALLY_I_MEAN_ALL();
    static void delete_all_entities(bool include_permanent = false);

    enum ForEachFlow {
        NormalFlow = 0,
        Continue = 1,
        Break = 2,
    };

    static void forEachEntity(const std::function<ForEachFlow(Entity &)> &cb);

    // TODO exists as a conversion for things that need shared_ptr right now
    static std::shared_ptr<Entity> getEntityAsSharedPtr(const Entity &entity) {
        for (std::shared_ptr<Entity> current_entity : get_entities()) {
            if (entity.id == current_entity->id) return current_entity;
        }
        return {};
    }

    static std::shared_ptr<Entity> getEntityAsSharedPtr(OptEntity entity) {
        if (!entity) return {};
        const Entity &e = entity.asE();
        return getEntityAsSharedPtr(e);
    }

    static OptEntity getEntityForID(EntityID id);
};

Entities &EntityHelper::get_entities_for_mod() { return entities_DO_NOT_USE; }
const Entities &EntityHelper::get_entities() { return get_entities_for_mod(); }

RefEntities EntityHelper::get_ref_entities() {
    RefEntities matching;
    for (const auto &e : EntityHelper::get_entities()) {
        if (!e) continue;
        matching.push_back(*e);
    }
    return matching;
}

struct CreationOptions {
    bool is_permanent;
};

Entity &EntityHelper::createEntity() {
    return createEntityWithOptions({.is_permanent = false});
}

Entity &EntityHelper::createPermanentEntity() {
    return createEntityWithOptions({.is_permanent = true});
}

Entity &EntityHelper::createEntityWithOptions(const CreationOptions &options) {
    std::shared_ptr<Entity> e(new Entity());
    get_entities_for_mod().push_back(e);

    if (options.is_permanent) {
        permanant_ids.insert(e->id);
    }

    return *e;
}

void EntityHelper::markIDForCleanup(int e_id) {
    auto &entities = get_entities();
    auto it = entities.begin();
    while (it != get_entities().end()) {
        if ((*it)->id == e_id) {
            (*it)->cleanup = true;
            break;
        }
        it++;
    }
}

void EntityHelper::removeEntity(int e_id) {
    auto &entities = get_entities_for_mod();

    auto newend = std::remove_if(
        entities.begin(), entities.end(),
        [e_id](const auto &entity) { return !entity || entity->id == e_id; });

    entities.erase(newend, entities.end());
}

void EntityHelper::cleanup() {
    // Cleanup entities marked cleanup
    Entities &entities = get_entities_for_mod();

    auto newend = std::remove_if(
        entities.begin(), entities.end(),
        [](const auto &entity) { return !entity || entity->cleanup; });

    entities.erase(newend, entities.end());
}

void EntityHelper::delete_all_entities_NO_REALLY_I_MEAN_ALL() {
    Entities &entities = get_entities_for_mod();
    // just clear the whole thing
    entities.clear();
}

void EntityHelper::delete_all_entities(bool include_permanent) {
    if (include_permanent) {
        delete_all_entities_NO_REALLY_I_MEAN_ALL();
        return;
    }

    // Only delete non perms
    Entities &entities = get_entities_for_mod();

    auto newend = std::remove_if(
        entities.begin(), entities.end(),
        [](const auto &entity) { return !permanant_ids.contains(entity->id); });

    entities.erase(newend, entities.end());
}

enum class ForEachFlow {
    NormalFlow = 0,
    Continue = 1,
    Break = 2,
};

void EntityHelper::forEachEntity(
    const std::function<ForEachFlow(Entity &)> &cb) {
    for (const auto &e : get_entities()) {
        if (!e) continue;
        auto fef = cb(*e);
        if (fef == 1) continue;
        if (fef == 2) break;
    }
}

OptEntity EntityHelper::getEntityForID(EntityID id) {
    if (id == -1) return {};

    for (const auto &e : get_entities()) {
        if (!e) continue;
        if (e->id == id) return *e;
    }
    return {};
}
