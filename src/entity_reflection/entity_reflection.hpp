#pragma once

#include "reflection_entity.hpp"
#include <entt/entt.hpp>
#include <map>

struct EntityReflection
{
  private:
    struct Data
    {
        std::optional<void*> (*getComponent)(entt::registry&, entt::entity);
        bool (*tryViewOne)(entt::registry&, entt::entity);
        bool (*tryModifyOne)(entt::registry&, entt::entity, bool);
        void (*tryDuplicate)(entt::registry&, entt::entity, entt::entity);
    };

    static std::map<uint32_t, Data>& EntityMap()
    {
        static std::map<uint32_t, Data> instance{};
        return instance;
    }

  public:
    template<typename T>
    static void Register()
    {
        EntityMap().insert(std::make_pair(
            T::ID,
            Data{
                .getComponent = T::GetComponent,
                .tryViewOne = T::TryViewOne,
                .tryModifyOne = T::TryModifyOne,
                .tryDuplicate = T::TryDuplicate,
            }));
    }

    template<typename Func>
    static void ModifyEntityOrElse(
        entt::registry& registry,
        entt::entity entity,
        const Func& ifNoneFound)
    {
        bool found = false;
        for(const auto& [key, value] : EntityMap())
            found = value.tryModifyOne(registry, entity, true) || found;

        if(!found)
            ifNoneFound();
    }

    template<typename T, typename Func>
    static void IfMissing(entt::registry& registry, entt::entity entity, const Func& func)
    {
        auto entityMap = EntityMap();

        auto iter = entityMap.find(T::ID);
        assert(iter != entityMap.end());

        if(!iter->second.getComponent(registry, entity))
            func();
    }

    template<typename T>
    static bool IsMissing(entt::registry& registry, entt::entity entity)
    {
        auto entityMap = EntityMap();

        auto iter = entityMap.find(T::ID);
        assert(iter != entityMap.end());

        return !iter->second.getComponent(registry, entity);
    }

    template<typename T, typename Func>
    static void ForEachMissing(entt::registry& registry, entt::entity entity, const Func& func)
    {
        auto entityMap = EntityMap();

        auto iter = entityMap.find(T::ID);
        assert(iter != entityMap.end());

        if(!iter->second.getComponent(registry, entity))
            func();
    }

    static entt::entity DuplicateEntity(entt::registry& registry, entt::entity entity)
    {
        auto newEntity = registry.create();
        for(const auto& [id, functions] : EntityMap())
            functions.tryDuplicate(registry, entity, newEntity);
        return newEntity;
    }
};
