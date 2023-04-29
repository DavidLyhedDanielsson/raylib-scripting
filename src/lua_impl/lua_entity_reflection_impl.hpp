#pragma once

#include <entt/fwd.hpp>

struct lua_State;

namespace LuaEntityReflection
{
    void Register(lua_State* lua, entt::registry* registry);
}