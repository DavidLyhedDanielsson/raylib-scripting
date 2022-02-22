extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <entt/entt.hpp>

#include "entity/render.hpp"
#include "entity/transform.hpp"
#include "entity/velocity.hpp"
#include "assets.hpp"

#include <type_traits>
#include <array>

// Make sure entt and lua can play nice.
// As long as entt::entity is backed by an integer that is smaller or the same
// size as a lua_Integer it can simply be cast and pushed to the lua stack
static_assert(std::is_integral<lua_Integer>::value, "lua_Integer is not an integer");
static_assert(std::is_integral<std::underlying_type<entt::entity>::type>::value, "entt::entity is not an integer");
static_assert(sizeof(lua_Integer) >= sizeof(entt::entity), "Cannot convert from entt::entity to lua_Integer without narrowing");

#define DeclareRegistry \
    auto registry = (entt::registry *)lua_touserdata(lua, lua_upvalueindex(1))

extern "C" int CreateEntity(lua_State *lua)
{
    DeclareRegistry;
    auto entity = static_cast<lua_Integer>(registry->create());
    lua_pushinteger(lua, entity);
    return 1;
}

extern "C" int AddRenderComponent(lua_State *lua)
{
    DeclareRegistry;
    auto entity = (entt::entity)luaL_checkinteger(lua, 1);
    auto assetId = luaL_checkinteger(lua, 2);
    if (assetId < 0 || assetId > (int)Asset::Last)
    {
        luaL_argerror(lua, 2, "Asset must be a value from the `Assets` table");
    }

    auto asset = (Asset)assetId;
    registry->emplace<Component::Render>(entity, GetAssetName(asset), GetLoadedAsset(asset));

    return 0;
}

extern "C" int AddTransformComponent(lua_State *lua)
{
    DeclareRegistry;
    auto entity = (entt::entity)luaL_checkinteger(lua, 1);
    registry->emplace<Component::Transform>(entity, Vector3{0.0f, 0.0f, 0.0f}, QuaternionIdentity());
    return 0;
}

extern "C" int AddVelocityComponent(lua_State *lua)
{
    DeclareRegistry;
    auto entity = (entt::entity)luaL_checkinteger(lua, 1);
    registry->emplace<Component::Velocity>(entity, 0.0f, 0.0f, 0.0f);
    return 0;
}

extern "C" void register_types(lua_State *lua)
{
    // Available models
    lua_newtable(lua);
    for (int i = 0; i < (int)Asset::Last; ++i)
    {
        lua_pushstring(lua, GetAssetName((Asset)i));
        lua_pushnumber(lua, i);
        lua_settable(lua, -3);
    }
    lua_setglobal(lua, "Asset");
}

void push_closure(lua_State *lua, entt::registry *registry, lua_CFunction func, const char *name)
{
    lua_pushlightuserdata(lua, registry);
    lua_pushcclosure(lua, func, 1);
    lua_setglobal(lua, name);
}

void register_entt(lua_State *lua, entt::registry *registry)
{
    register_types(lua);

    push_closure(lua, registry, CreateEntity, "CreateEntity");
    push_closure(lua, registry, AddRenderComponent, "AddRenderComponent");
    push_closure(lua, registry, AddTransformComponent, "AddTransformComponent");
    push_closure(lua, registry, AddVelocityComponent, "AddVelocityComponent");
}
