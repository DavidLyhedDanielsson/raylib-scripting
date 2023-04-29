#pragma once

#include <entt/fwd.hpp>
#include <lua_impl/lua_register.hpp>

struct lua_State;

namespace LuaRaylib
{
    void Register(lua_State* lua, entt::registry* registry);
}