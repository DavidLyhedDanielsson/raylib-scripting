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
        registry.emplace<Component::RComponent>(
            entity,
            Component::RComponent{.id = 0, .goalId = 0});
    }

    static LuaValidator::LuaValidator GetLuaValidator(lua_State * lua)
    {
        return LuaValidator::LuaValidator(lua).FieldIs<int>("id").FieldIs<int>("goalId");
    }

    static void CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        lua_getfield(lua, -1, "id");
        int id = lua_tonumber(lua, -1);
        lua_getfield(lua, -2, "goalId");
        int goalId = lua_tonumber(lua, -1);

        registry.emplace<Component::RComponent>(
            entity,
            Component::RComponent{.id = id, .goalId = goalId});

        lua_pop(lua, 2);
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component)
    {
        lua_pushinteger(lua, component.id);
        lua_setfield(lua, -2, "id");
        lua_pushinteger(lua, component.goalId);
        lua_setfield(lua, -2, "goalId");
    }

    static void View(Component::RComponent component)
    {
        ImGui::Text("ID: %i", component.id);
        ImGui::Text("Target goal: %i", component.goalId);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component)
    {
        ImGui::InputInt("ID", &component.id);
        ImGui::InputInt("Target goal", &component.goalId);
    }

    static void Duplicate(
        entt::registry & registry,
        const Component::RComponent& component,
        entt::entity target)
    {
        registry.emplace<Component::RComponent>(
            target,
            Component::RComponent{.id = component.id, .goalId = component.goalId});
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent