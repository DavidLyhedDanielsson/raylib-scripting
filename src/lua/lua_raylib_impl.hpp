#pragma once

#include <entt/entt.hpp>

struct lua_State;

namespace LuaRaylib
{
    void Register(lua_State* lua, entt::registry* registry);
}