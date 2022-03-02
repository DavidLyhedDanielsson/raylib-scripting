#include "lua_entt_impl.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <array>
#include <functional>
#include <type_traits>

#include "assets.hpp"
#include "entity/render.hpp"
#include "entity/transform.hpp"
#include "entity/velocity.hpp"

#define DeclareRegistry auto registry = (entt::registry*)lua_touserdata(lua, lua_upvalueindex(1))
#define LuaFunc(Name) extern "C" int Name(lua_State* lua)

LuaFunc(CreateEntity)
{
    DeclareRegistry;
    auto entity = static_cast<lua_Integer>(registry->create());
    lua_pushinteger(lua, entity);
    return 1;
}

LuaFunc(AddRenderComponent)
{
    DeclareRegistry;
    auto entity = (entt::entity)luaL_checkinteger(lua, 1);
    auto assetId = luaL_checkinteger(lua, 2);
    if(assetId < 0 || assetId > (int)Asset::Last)
    {
        luaL_argerror(lua, 2, "Asset must be a value from the `Assets` table");
    }

    auto asset = (Asset)assetId;
    registry->emplace<Component::Render>(entity, GetAssetName(asset), GetLoadedAsset(asset));

    return 0;
}

LuaFunc(AddTransformComponent)
{
    DeclareRegistry;
    auto entity = (entt::entity)luaL_checkinteger(lua, 1);
    registry->emplace<Component::Transform>(
        entity,
        Vector3{0.0f, 0.0f, 0.0f},
        QuaternionIdentity());
    return 0;
}

LuaFunc(AddVelocityComponent)
{
    DeclareRegistry;
    auto entity = (entt::entity)luaL_checkinteger(lua, 1);
    registry->emplace<Component::Velocity>(entity, 0.0f, 0.0f, 0.0f);
    return 0;
}

void register_types(lua_State* lua)
{
    // Available models
    lua_newtable(lua);
    for(int i = 0; i < (int)Asset::Last; ++i)
    {
        lua_pushstring(lua, GetAssetName((Asset)i));
        lua_pushnumber(lua, i);
        lua_settable(lua, -3);
    }
    lua_setglobal(lua, "Asset");
}

// Adds a C function with the entt::registry as an upvalue
#define push_closure(func)                \
    lua_pushlightuserdata(lua, registry); \
    lua_pushcclosure(lua, func, 1);       \
    lua_setglobal(lua, #func);

void register_entt(lua_State* lua, entt::registry* registry)
{
    register_types(lua);

    push_closure(CreateEntity);
    push_closure(AddRenderComponent);
    push_closure(AddTransformComponent);
    push_closure(AddVelocityComponent);
}