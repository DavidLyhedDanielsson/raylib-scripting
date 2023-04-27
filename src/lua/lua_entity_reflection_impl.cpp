#include "lua/lua_register.hpp"
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
            "Modify",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* componentName,
                lua_Integer entity) {
                EntityReflection::Modify(componentName, *registry, (entt::entity)entity);
            });

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
            "IfComponentMissing",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* component,
                lua_Integer entity,
                Placeholder callback) {
                return EntityReflection::IfComponentMissing(
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
                const char* componentName,
                lua_Integer entity,
                Placeholder component) {
                if(EntityReflection::AddComponentFromLua(
                       lua,
                       componentName,
                       registry,
                       (entt::entity)entity))
                {
                    lua_pushnil(lua);
                }
                return Placeholder{};
            });

        RegisterMember(
            lua,
            "RemoveComponent",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* componentName,
                lua_Integer entity) {
                EntityReflection::RemoveComponent(componentName, registry, (entt::entity)entity);
            });

        RegisterMember(
            lua,
            "GetEntity",
            registry,
            +[](entt::registry* registry, lua_State* lua, lua_Integer entity) -> Placeholder {
                lua_createtable(lua, 1, 0);
                registry->each([&](entt::entity entity) {
                    EntityReflection::PushEntityToLua(lua, registry, entity);
                });
                lua_geti(lua, -1, entity);
                // Pop table
                lua_rotate(lua, lua_gettop(lua) - 1, 1);
                lua_pop(lua, 1);
                return {};
            });

        RegisterMember(
            lua,
            "DumpEntities",
            registry,
            +[](entt::registry* registry, lua_State* lua) -> Placeholder {
                lua_createtable(lua, registry->alive(), 0);
                registry->each([&](entt::entity entity) {
                    EntityReflection::PushEntityToLua(lua, registry, entity);
                });
                return {};
            });
    }
}