#pragma once

#include <entity_reflection/reflection_entity.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register_types.hpp>

#include <entity/enemy_goal.hpp>
#define RComponent EnemyGoal
EntityReflectionStruct(RComponent)
{
    ; // This semicolon needs to be here or else clang-format breaks. The benefits of the macro
      // outweigh the weirdness
    static void Create(entt::registry & registry, entt::entity entity)
    {
        registry.emplace<Component::RComponent>(entity, Component::RComponent{.id = 0});
    }

    static LuaValidator::LuaValidator GetLuaValidator(lua_State * lua)
    {
        return LuaValidator::LuaValidator(lua).FieldIs<int>("id");
    }

    static void CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        lua_getfield(lua, -1, "id");

        registry.emplace<Component::RComponent>(
            entity,
            Component::RComponent{.id = (int)lua_tointeger(lua, -1)});

        lua_pop(lua, 1);
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component)
    {
        lua_pushstring(lua, "id");
        lua_pushinteger(lua, component.id);
        lua_settable(lua, -3);
    }

    static void View(Component::RComponent component)
    {
        ImGui::Text("ID: %i", component.id);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component)
    {
        ImGui::InputInt("ID", &component.id);
    }

    static void Duplicate(
        entt::registry & registry,
        const Component::RComponent& component,
        entt::entity target)
    {
        registry.emplace<Component::RComponent>(target, Component::RComponent{.id = component.id});
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent