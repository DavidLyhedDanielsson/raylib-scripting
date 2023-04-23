#pragma once

#include "lua_register.hpp"
#include <entt/fwd.hpp>

struct lua_State;

namespace LuaRaylib
{
    void Register(lua_State* lua, entt::registry* registry);
}