#include <entity_reflection/entity_reflection.hpp>
#include <lua_impl/lua_imgui_impl.hpp>
#include <lua_impl/lua_register.hpp>
#include <lua_impl/lua_register_types.hpp>

namespace LuaEntityReflection
{
    void Register(lua_State* lua, entt::registry* registry)
    {
        using namespace LuaRegister;

        GlobalMemberRegister(
            lua,
            "Modify",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* componentName,
                lua_Integer entity) {
                EntityReflection::Modify(componentName, *registry, (entt::entity)entity);
            });

        GlobalMemberRegister(
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

        GlobalMemberRegister(
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

        GlobalMemberRegister(
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

        GlobalMemberRegister(
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

        GlobalMemberRegister(
            lua,
            "DuplicateEntity",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                lua_Integer entity,
                Placeholder callback) {
                EntityReflection::DuplicateEntity(*registry, (entt::entity)entity);
            });

        GlobalMemberRegister(
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

        GlobalMemberRegister(
            lua,
            "RemoveComponent",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* componentName,
                lua_Integer entity) {
                EntityReflection::RemoveComponent(componentName, registry, (entt::entity)entity);
            });

        GlobalMemberRegister(
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

        GlobalMemberRegister(
            lua,
            "GetAllEntitiesWithComponent",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* componentName) -> Placeholder {
                lua_createtable(lua, 0, 0);

                EntityReflection::PushAllEntitiesToLua(lua, componentName, registry);

                return {};
            });

        GlobalMemberRegister(
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