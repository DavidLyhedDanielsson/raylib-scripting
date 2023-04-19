#include "lua_entt_impl.hpp"

// TODO: REMOVE
#include <iostream>

extern "C" {
#include <lua.h>
}
#include "lua_register.hpp"

#include <array>
#include <cstring>
#include <functional>
#include <type_traits>

#include "../assets.hpp"
#include "../entity/render.hpp"
#include "../entity/transform.hpp"
#include "../entity/velocity.hpp"

#include <entt/entt.hpp>

#define DeclareRegistry auto registry = (entt::registry*)lua_touserdata(lua, lua_upvalueindex(1))
#define LuaFunc(Name) extern "C" int Name(lua_State* lua)

namespace LuaEntt
{
    void RegisterTypes(lua_State* lua)
    {
        // Available models
        // lua_newtable(lua);
        // for(int i = 0; i < (int)Asset::Last; ++i)
        // {
        //     lua_pushstring(lua, GetAssetName((Asset)i));
        //     lua_pushnumber(lua, i);
        //     lua_settable(lua, -3);
        // }
        // lua_setglobal(lua, "Asset");
    }

    lua_Integer CreateEntity(entt::registry* registry)
    {
        return static_cast<lua_Integer>(registry->create());
    }

    void AddRenderComponent(entt::registry* registry, lua_Integer entity, const char* assetId)
    {
        if(auto iter = loadedAssets.find(assetId); iter != loadedAssets.end())
            registry->emplace<Component::Render>((entt::entity)entity, assetId, iter->second);
    }

    void AddTransformComponent(entt::registry* registry, lua_Integer entity)
    {
        registry->emplace<Component::Transform>(
            (entt::entity)entity,
            Vector3{0.0f, 0.0f, 0.0f},
            QuaternionIdentity());
    }

    void AddTransformComponentAt(
        entt::registry* registry,
        lua_Integer entity,
        float x,
        float y,
        float z)
    {
        registry->emplace<Component::Transform>(
            (entt::entity)entity,
            Vector3{x, y, z},
            QuaternionIdentity());
    }

    void AddVelocityComponent(entt::registry* registry, lua_Integer entity)
    {
        registry->emplace<Component::Velocity>((entt::entity)entity, 0.0f, 0.0f, 0.0f);
    }

    void Register(lua_State* lua, entt::registry* registry)
    {
        RegisterTypes(lua);

#define QuickRegister(Func) LuaRegister::RegisterMember(lua, #Func, registry, Func);

        QuickRegister(CreateEntity);
        QuickRegister(AddRenderComponent);
        QuickRegister(AddTransformComponent);
        QuickRegister(AddTransformComponentAt);
        QuickRegister(AddVelocityComponent);

        LuaRegister::RegisterMember(
            lua,
            "EnttForEach",
            registry,
            +[](entt::registry* registry,
                lua_State* lua,
                LuaRegister::Placeholder func,
                LuaRegister::Variadic<const char*> var) {
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
    }
}