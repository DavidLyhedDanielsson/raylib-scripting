#include "lua_entt_impl.hpp"

#include <array>
#include <cstring>
#include <functional>
#include <iostream> // TODO: REMOVE
#include <type_traits>

#include <assets.hpp>
#include <component/area_tracker.hpp>
#include <component/render.hpp>
#include <component/tile.hpp>
#include <component/transform.hpp>
#include <component/velocity.hpp>
#include <entity_reflection/entity_reflection.hpp>
#include <entt/entt.hpp>
#include <external/lua.hpp>
#include <lua_impl/lua_register.hpp>
#include <lua_impl/lua_register_types.hpp>
#include <profiling.hpp>

#define QuickRegister(Func) LuaRegister::PushRegisterMember(lua, #Func, registry, Func);

namespace LuaEntt
{
    void Register(lua_State* lua, entt::registry* registry)
    {
        lua_createtable(lua, 0, 0);

        LuaRegister::PushRegisterMember(
            lua,
            "Create",
            registry,
            +[](entt::registry* registry, lua_State* lua) {
                return (lua_Integer)registry->create();
            });

        LuaRegister::PushRegisterMember(
            lua,
            "Destroy",
            registry,
            +[](entt::registry* registry, lua_State* lua, lua_Integer entity) {
                registry->destroy((entt::entity)entity);
            });

        LuaRegister::PushRegisterMember(
            lua,
            "IsValid",
            registry,
            +[](entt::registry* registry, lua_State* lua, lua_Integer entity) {
                return registry->valid((entt::entity)entity);
            });

        LuaRegister::PushRegisterMember(
            lua,
            "ClearRegistry",
            registry,
            +[](entt::registry* registry, lua_State* lua) { registry->clear(); });

        LuaRegister::PushRegisterMember(
            lua,
            "Each",
            registry,
            +[](entt::registry* registry, lua_State* lua, LuaRegister::Placeholder callback) {
                registry->each([&](entt::entity entity) {
                    lua_pushnil(lua);
                    lua_copy(lua, callback.stackIndex, lua_gettop(lua));

                    lua_pushinteger(lua, (lua_Integer)entity);
                    if(lua_pcall(lua, 1, 0, 0) != LUA_OK)
                    {
                        auto err = lua_tostring(lua, -1);
                        std::cerr << "Error when calling each" << std::endl;
                        std::cerr << err << std::endl;
                    }
                });
            });

        LuaRegister::PushRegisterMember(
            lua,
            "View",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                LuaRegister::Placeholder func,
                LuaRegister::Variadic<const char*> var) {
                // Old function
                entt::runtime_view view;

                const auto& render = registry->storage<Component::Render>();
                const auto& transform = registry->storage<Component::Transform>();
                const auto& velocity = registry->storage<Component::Velocity>();

                for(int i = 0; i < var.count; ++i)
                {
                    if(std::strcmp(var.arr[i], "render") == 0)
                        view.iterate(render);
                    else if(std::strcmp(var.arr[i], "transform") == 0)
                        view.iterate(transform);
                    else if(std::strcmp(var.arr[i], "velocity") == 0)
                        view.iterate(velocity);
                }

                view.each([=](const auto entity) {
                    lua_pushvalue(lua, func.stackIndex);
                    lua_pushinteger(lua, (lua_Integer)entity);
                    if(lua_pcall(lua, 1, 0, 0) != LUA_OK)
                    {
                        std::cerr << lua_tostring(lua, -1);
                    }
                });
            });

        LuaRegister::PushRegisterMember(
            lua,
            "TrackerHasEntities",
            registry,
            +[](entt::registry* registry, lua_State* lua, lua_Integer entity) {
                return !registry->get<Component::AreaTracker>((entt::entity)entity)
                            .entitiesInside.empty();
            });

        LuaRegister::PushRegisterMember(
            lua,
            "GetForwardVector",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                lua_Integer entity) -> LuaRegister::Placeholder {
                const auto target = registry->get<Component::Transform>((entt::entity)entity);

                Matrix mat = MatrixRotateZYX(target.rotation);
                Vector3 forward = Vector3Transform(Vector3{0.0f, 0.0f, 1.0f}, mat);

                LuaRegister::LuaSetFunc<Vector3>(lua, forward);

                return {};
            });

        LuaRegister::PushRegisterMember(
            lua,
            "TransformTo",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                lua_Integer transformeeEntity,
                lua_Integer targetEntity) {
                auto& transformee =
                    registry->get<Component::Transform>((entt::entity)transformeeEntity);
                const auto target = registry->get<Component::Transform>((entt::entity)targetEntity);

                Matrix mat = MatrixRotateZYX(target.rotation);
                Vector3 newPosition = Vector3Transform(transformee.position, mat);

                transformee.rotation = target.rotation;
                transformee.position = Vector3Add(newPosition, target.position);
            });

        LuaRegister::PushRegisterMember(
            lua,
            "Duplicate",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                lua_Integer entity,
                LuaRegister::Placeholder callback) {
                EntityReflection::DuplicateEntity(*registry, (entt::entity)entity);
            });

        LuaRegister::PushRegisterMember(
            lua,
            "Get",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                lua_Integer entity) -> LuaRegister::Placeholder {
                PROFILE_SCOPE("Entity.Get");

                if(!registry->valid((entt::entity)entity))
                {
                    lua_pushnil(lua);
                    return {};
                }

                lua_createtable(lua, 1, 0);

                EntityReflection::PushEntityToLua(lua, registry, (entt::entity)entity);

                lua_geti(lua, -1, entity);
                // Pop table
                lua_rotate(lua, lua_gettop(lua) - 1, 1);
                lua_pop(lua, 1);
                return {};
            });

        LuaRegister::PushRegisterMember(
            lua,
            "GetAllWithComponent",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* componentName) -> LuaRegister::Placeholder {
                lua_createtable(lua, 0, 0);

                EntityReflection::PushAllEntitiesToLua(lua, componentName, registry);

                return {};
            });

        LuaRegister::PushRegisterMember(
            lua,
            "DumpAll",
            registry,
            +[](entt::registry* registry, lua_State* lua) -> LuaRegister::Placeholder {
                lua_createtable(lua, registry->alive(), 0);
                registry->each([&](entt::entity entity) {
                    EntityReflection::PushEntityToLua(lua, registry, entity);
                });
                return {};
            });

        // Component control
        LuaRegister::PushRegisterMember(
            lua,
            "AddComponent",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* componentName,
                lua_Integer entity,
                LuaRegister::Placeholder component) {
                PROFILE_SCOPE("Entity.AddComponent");
                if(EntityReflection::AddComponentFromLua(
                       lua,
                       componentName,
                       registry,
                       (entt::entity)entity))
                {
                    lua_pushnil(lua);
                }
                return LuaRegister::Placeholder{};
            });

        LuaRegister::PushRegisterMember(
            lua,
            "ReplaceComponent",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* componentName,
                lua_Integer entity,
                LuaRegister::Placeholder component) {
                EntityReflection::RemoveComponent(componentName, registry, (entt::entity)entity);
                if(EntityReflection::AddComponentFromLua(
                       lua,
                       componentName,
                       registry,
                       (entt::entity)entity))
                {
                    lua_pushnil(lua);
                }
                return LuaRegister::Placeholder{};
            });

        LuaRegister::PushRegisterMember(
            lua,
            "RemoveComponent",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* componentName,
                lua_Integer entity) {
                EntityReflection::RemoveComponent(componentName, registry, (entt::entity)entity);
            });

        LuaRegister::PushRegisterMember(
            lua,
            "ComponentCount",
            registry,
            +[](entt::registry* registry, lua_State* lua, const char* componentName) {
                return EntityReflection::GetComponentCount(componentName, *registry);
            });

        LuaRegister::PushRegisterMember(
            lua,
            "ImGuiModify",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* componentName,
                lua_Integer entity) {
                EntityReflection::Modify(componentName, *registry, (entt::entity)entity);
            });

        LuaRegister::PushRegisterMember(
            lua,
            "ImGuiModifyEntityOrElse",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                lua_Integer entity,
                LuaRegister::Placeholder callback) {
                EntityReflection::ModifyEntityOrElse(*registry, (entt::entity)entity, [&]() {
                    lua_pcall(lua, 0, 0, callback.stackIndex);
                });
            });

        LuaRegister::PushRegisterMember(
            lua,
            "HasComponentOrElse",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* component,
                lua_Integer entity,
                LuaRegister::Placeholder callback) {
                return EntityReflection::IfComponentMissing(
                    component,
                    *registry,
                    (entt::entity)entity,
                    [&]() { lua_pcall(lua, 0, 0, callback.stackIndex); });
            });

        LuaRegister::PushRegisterMember(
            lua,
            "HasComponent",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* component,
                lua_Integer entity,
                LuaRegister::Placeholder callback) {
                return EntityReflection::HasComponent(component, *registry, (entt::entity)entity);
            });

        LuaRegister::PushRegisterMember(
            lua,
            "ForEachMissingComponent",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* component,
                lua_Integer entity,
                LuaRegister::Placeholder callback) {
                EntityReflection::ForEachMissing(component, *registry, (entt::entity)entity, [&]() {
                    lua_pcall(lua, 0, 0, callback.stackIndex);
                });
            });

        LuaRegister::PushRegisterMember(
            lua,
            "ForEachWithComponent",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                const char* component,
                LuaRegister::Placeholder callback) {
                EntityReflection::ForEachWith(lua, component, *registry, (int)callback.stackIndex);
            });

        lua_setglobal(lua, "Entity");
    }
}