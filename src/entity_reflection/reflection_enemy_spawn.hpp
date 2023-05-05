#pragma once

#include <entity_reflection/reflection_entity.hpp>
#include <external/lua.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register_types.hpp>

#include <entity/enemy_spawn.hpp>
#define RComponent EnemySpawn
EntityReflectionStruct(RComponent)
{
    ; // This semicolon needs to be here or else clang-format breaks. The benefits of the macro
      // outweigh the weirdness
    static void Create(entt::registry & registry, entt::entity entity)
    {
        registry.emplace<Component::RComponent>(entity, Component::RComponent{.targetGoal = 0});
    }

    static LuaValidator::LuaValidator GetLuaValidator(lua_State * lua)
    {
        return LuaValidator::LuaValidator(lua).FieldIs<int>("targetGoal");
    }

    static void CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        lua_getfield(lua, -1, "targetGoal");

        registry.emplace<Component::RComponent>(
            entity,
            Component::RComponent{.targetGoal = (int)lua_tointeger(lua, -1)});

        lua_pop(lua, 1);
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component)
    {
        lua_pushstring(lua, "targetGoal");
        lua_pushinteger(lua, component.targetGoal);
        lua_settable(lua, -3);
    }

    static void View(Component::RComponent component)
    {
        ImGui::Text("Target goal: %i", component.targetGoal);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component)
    {
        ImGui::InputInt("Target goal", &component.targetGoal);
    }

    static void Duplicate(
        entt::registry & registry,
        const Component::RComponent& component,
        entt::entity target)
    {
        registry.emplace<Component::RComponent>(
            target,
            Component::RComponent{.targetGoal = component.targetGoal});
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent