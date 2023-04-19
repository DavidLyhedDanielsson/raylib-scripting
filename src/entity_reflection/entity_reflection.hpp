#pragma once

#include "reflection_entity.hpp"
#include <entt/entt.hpp>
#include <map>

struct EntityReflection
{
  private:
    struct ComponentFunctions
    {
        std::optional<void*> (*getComponent)(entt::registry&, entt::entity);
        bool (*tryViewOne)(entt::registry&, entt::entity);
        bool (*tryModifyOne)(entt::registry&, entt::entity, bool);
        void (*tryDuplicate)(entt::registry&, entt::entity, entt::entity);
    };

    static std::map<uint32_t, ComponentFunctions>& ComponentMap()
    {
        static std::map<uint32_t, ComponentFunctions> instance{};
        return instance;
    }

  public:
    template<typename Component>
    static void Register()
    {
        ComponentMap().insert(std::make_pair(
            Component::ID,
            ComponentFunctions{
                .getComponent = Component::GetComponent,
                .tryViewOne = Component::TryViewOne,
                .tryModifyOne = Component::TryModifyOne,
                .tryDuplicate = Component::TryDuplicate,
            }));
    }

    template<typename Func>
    static void ModifyEntityOrElse(
        entt::registry& registry,
        entt::entity entity,
        const Func& ifNoneFound)
    {
        bool found = false;
        for(const auto& [key, value] : ComponentMap())
            found = value.tryModifyOne(registry, entity, true) || found;

        if(!found)
            ifNoneFound();
    }

    template<typename Component, typename Func>
    static void IfMissing(entt::registry& registry, entt::entity entity, const Func& func)
    {
        auto entityMap = ComponentMap();

        auto iter = entityMap.find(Component::ID);
        assert(iter != entityMap.end());

        if(!iter->second.getComponent(registry, entity))
            func();
    }

    template<typename Component>
    static bool IsMissing(entt::registry& registry, entt::entity entity)
    {
        auto entityMap = ComponentMap();

        auto iter = entityMap.find(Component::ID);
        assert(iter != entityMap.end());

        return !iter->second.getComponent(registry, entity);
    }

    template<typename Component, typename Func>
    static void ForEachMissing(entt::registry& registry, entt::entity entity, const Func& func)
    {
        auto entityMap = ComponentMap();

        auto iter = entityMap.find(Component::ID);
        assert(iter != entityMap.end());

        if(!iter->second.getComponent(registry, entity))
            func();
    }

    static entt::entity DuplicateEntity(entt::registry& registry, entt::entity entity)
    {
        auto newEntity = registry.create();
        for(const auto& [id, functions] : ComponentMap())
            functions.tryDuplicate(registry, entity, newEntity);
        return newEntity;
    }
};
