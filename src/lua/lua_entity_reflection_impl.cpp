#include "lua_imgui_impl.hpp"

#include "lua_register_types.hpp"
#include <entity_reflection/entity_reflection.hpp>

namespace LuaEntityReflection
{
    void Register(lua_State* lua, entt::registry* registry)
    {
        using namespace LuaRegister;

        RegisterMember(
            lua,
            "ModifyEntityOrElse",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                lua_Integer entity,
                Placeholder callback) {
                EntityReflection::ModifyEntityOrElse(*registry, (entt::entity)entity, [&]() {
                    lua_pcall(lua, 0, 0, callback.stackIndex);
                });
            });

        RegisterMember(
            lua,
            "IfMissing",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* component,
                lua_Integer entity,
                Placeholder callback) {
                return EntityReflection::IfMissing(
                    component,
                    *registry,
                    (entt::entity)entity,
                    [&]() { lua_pcall(lua, 0, 0, callback.stackIndex); });
            });

        RegisterMember(
            lua,
            "HasComponent",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* component,
                lua_Integer entity,
                Placeholder callback) {
                return EntityReflection::HasComponent(component, *registry, (entt::entity)entity);
            });

        RegisterMember(
            lua,
            "ForEachMissing",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* component,
                lua_Integer entity,
                Placeholder callback) {
                EntityReflection::ForEachMissing(component, *registry, (entt::entity)entity, [&]() {
                    lua_pcall(lua, 0, 0, callback.stackIndex);
                });
            });

        RegisterMember(
            lua,
            "DuplicateEntity",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                lua_Integer entity,
                Placeholder callback) {
                EntityReflection::DuplicateEntity(*registry, (entt::entity)entity);
            });

        RegisterMember(
            lua,
            "AddComponent",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                lua_Integer entity,
                const char* componentName,
                Placeholder component) {
                EntityReflection::AddComponentFromLua(
                    lua,
                    componentName,
                    registry,
                    (entt::entity)entity,
                    component.stackIndex);
            });
    }
}