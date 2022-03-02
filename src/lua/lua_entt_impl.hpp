#pragma once

extern "C" {
#include <lua.h>
}
#include <entt/entt.hpp>

// Make sure entt and lua can play nice.
// As long as entt::entity is backed by an integer that is smaller or the same
// size as a lua_Integer it can simply be cast and pushed to the lua stack
static_assert(std::is_integral<lua_Integer>::value, "lua_Integer is not an integer");
static_assert(
    std::is_integral<std::underlying_type<entt::entity>::type>::value,
    "entt::entity is not an integer");
static_assert(
    sizeof(lua_Integer) >= sizeof(entt::entity),
    "Cannot convert from entt::entity to lua_Integer without narrowing");

namespace LuaEntt
{
    void RegisterTypes(lua_State* lua);
    void Register(lua_State* lua, entt::registry* registry);
}
