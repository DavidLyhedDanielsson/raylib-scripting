#pragma once

#include <entity_reflection/reflection_entity.hpp>
#include <entt/entt.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register_types.hpp>

#include <component/velocity.hpp>
#define RComponent Velocity
EntityReflectionStruct(RComponent)
{
    ; // This semicolon needs to be here or else clang-format breaks. The benefits of the macro
      // outweigh the weirdness

    static void Create(entt::registry & registry, entt::entity entity)
    {
        registry.emplace<Component::RComponent>(entity, 0.0f, 0.0f, 0.0f);
    }

    static LuaValidator::LuaValidator GetLuaValidator(lua_State * lua)
    {
        return LuaValidator::LuaValidator(lua)
            .FieldIs<float>("x")
            .FieldIs<float>("y")
            .FieldIs<float>("z");
    }

    static bool CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        Component::RComponent component{};

        int stackIndex = lua_gettop(lua);

        lua_getfield(lua, stackIndex, "x");
        component.x = lua_tonumber(lua, -1);
        lua_getfield(lua, stackIndex, "y");
        component.y = lua_tonumber(lua, -1);
        lua_getfield(lua, stackIndex, "z");
        component.z = lua_tonumber(lua, -1);

        lua_pop(lua, 3);

        registry.emplace<Component::RComponent>(entity, component);
        return true;
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component)
    {
        lua_pushstring(lua, "x");
        lua_pushnumber(lua, component.x);
        lua_settable(lua, -3);
        lua_pushstring(lua, "y");
        lua_pushnumber(lua, component.y);
        lua_settable(lua, -3);
        lua_pushstring(lua, "z");
        lua_pushnumber(lua, component.z);
        lua_settable(lua, -3);
    }

    static void View(Component::RComponent & component)
    {
        ImGui::Text("Velocity: %f, %f, %f", component.x, component.y, component.z);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component)
    {
        ImGui::DragFloat3("Velocity", &component.x, 0.001f);
    }

    static void Duplicate(
        entt::registry & registry,
        const Component::RComponent& component,
        entt::entity target)
    {
        registry.emplace<Component::RComponent>(target, component.x, component.y, component.z);
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent