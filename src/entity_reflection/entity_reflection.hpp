#pragma once

#include <entt/entt.hpp>
#include <map>
#include <optional>
#include <string>

// Ifdef just to keep the lua parts separated and easy to remove
#ifndef ENTITY_REFLECTION_SKIP_LUA
extern "C" {
    #include <lua.h>
}
#endif

struct EntityReflection
{
  private:
    struct ComponentFunctions
    {
        void (*removeComponent)(entt::registry&, entt::entity);
        std::optional<void*> (*getComponent)(entt::registry&, entt::entity);
        bool (*tryViewOne)(entt::registry&, entt::entity);
        bool (*tryModifyOne)(entt::registry&, entt::entity, bool);
        void (*tryDuplicate)(entt::registry&, entt::entity, entt::entity);
#ifndef ENTITY_REFLECTION_SKIP_LUA
        // TODO: Error handling
        bool (*createFromLua)(lua_State*, entt::registry&, entt::entity);
        void (*pushToLua)(lua_State*, void*);
#endif
    };

    // TODO: use some handle instead of a string
    static std::map<std::string, ComponentFunctions>& ComponentMap()
    {
        static std::map<std::string, ComponentFunctions> instance{};
        return instance;
    }

  public:
    template<typename Component>
    static void Register()
    {
        ComponentMap().insert(std::make_pair(
            Component::NAME,
            ComponentFunctions{
                .removeComponent = Component::RemoveComponent,
                .getComponent = Component::GetComponent,
                .tryViewOne = Component::TryViewOne,
                .tryModifyOne = Component::TryModifyOne,
                .tryDuplicate = Component::TryDuplicate,
#ifndef ENTITY_REFLECTION_SKIP_LUA
                .createFromLua = Component::CreateFromLua,
                .pushToLua = Component::PushToLua,
#endif
            }));
    }

    static void Modify(const char* componentName, entt::registry& registry, entt::entity entity)
    {
        auto entityMap = ComponentMap();

        // emscripten really wants an std::string here, not const char* :(
        auto iter = entityMap.find(std::string(componentName));
        assert(iter != entityMap.end());

        iter->second.tryModifyOne(registry, entity, false);
    }

    template<typename Func>
    static void IfComponentMissing(
        const char* componentName,
        entt::registry& registry,
        entt::entity entity,
        const Func& func)
    {
        auto entityMap = ComponentMap();

        auto iter = entityMap.find(componentName);
        assert(iter != entityMap.end());

        if(!iter->second.getComponent(registry, entity))
            func();
    }

    template<typename Component, typename Func>
    static void IfComponentMissing(entt::registry& registry, entt::entity entity, const Func& func)
    {
        IfComponentMissing(Component::NAME, registry, entity, func);
    }

    static bool HasComponent(
        const char* componentName,
        entt::registry& registry,
        entt::entity entity)
    {
        auto entityMap = ComponentMap();

        auto iter = entityMap.find(componentName);
        assert(iter != entityMap.end());

        return iter->second.getComponent(registry, entity).has_value();
    }

    template<typename Component>
    static bool HasComponent(entt::registry& registry, entt::entity entity)
    {
        return HasComponent(Component::NAME, registry, entity);
    }

    static entt::entity DuplicateEntity(entt::registry& registry, entt::entity entity)
    {
        auto newEntity = registry.create();
        for(const auto& [id, functions] : ComponentMap())
            functions.tryDuplicate(registry, entity, newEntity);
        return newEntity;
    }

    // "Functional" functions
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

    template<typename Func>
    static void ForEachMissing(
        const char* componentName,
        entt::registry& registry,
        entt::entity entity,
        const Func& func)
    {
        auto entityMap = ComponentMap();

        auto iter = entityMap.find(componentName);
        assert(iter != entityMap.end());

        if(!iter->second.getComponent(registry, entity))
            func();
    }

    template<typename Component, typename Func>
    static void ForEachMissing(entt::registry& registry, entt::entity entity, const Func& func)
    {
        ForEachMissing(Component::NAME, registry, entity, func);
    }

    static void RemoveComponent(
        const char* componentName,
        entt::registry* registry,
        entt::entity entity)
    {
        auto entityMap = ComponentMap();

        auto iter = entityMap.find(componentName);
        assert(iter != entityMap.end());

        iter->second.removeComponent(*registry, entity);
    }

    template<typename Component>
    static void RemoveComponent(entt::registry* registry, entt::entity entity)
    {
        RemoveComponent(Component::NAME, registry, entity);
    }

#ifndef ENTITY_REFLECTION_SKIP_LUA
    static bool AddComponentFromLua(
        lua_State* lua,
        const char* componentName,
        entt::registry* registry,
        entt::entity entity)
    {
        auto entityMap = ComponentMap();

        auto iter = entityMap.find(componentName);
        assert(iter != entityMap.end());

        return iter->second.createFromLua(lua, *registry, entity);
    }

    static void PushEntityToLua(lua_State* lua, entt::registry* registry, entt::entity entity)
    {
        lua_pushinteger(lua, (lua_Integer)entity);
        lua_createtable(lua, 0, 0);

        for(const auto& [id, functions] : ComponentMap())
        {
            if(auto componentOpt = functions.getComponent(*registry, entity); componentOpt)
                functions.pushToLua(lua, componentOpt.value());
        }

        lua_settable(lua, -3);
    }
#endif
};
