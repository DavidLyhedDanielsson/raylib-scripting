#pragma once

#include <entity_reflection/reflection_entity.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register_types.hpp>

#include <entity/nav_gate.hpp>
#define RComponent NavGate
EntityReflectionStruct(RComponent)
{
    ; // This semicolon needs to be here or else clang-format breaks. The benefits of the macro
      // outweigh the weirdness
    static void Create(entt::registry & registry, entt::entity entity)
    {
        registry.emplace<Component::RComponent>(entity, Component::RComponent{.allowedGoalId = 0});
    }

    static LuaValidator::LuaValidator GetLuaValidator(lua_State * lua)
    {
        return LuaValidator::LuaValidator(lua).FieldIs<int>("allowedGoalId");
    }

    static void CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        lua_getfield(lua, -1, "allowedGoalId");

        registry.emplace<Component::RComponent>(
            entity,
            Component::RComponent{.allowedGoalId = (uint32_t)lua_tointeger(lua, -1)});

        lua_pop(lua, 1);
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component)
    {
        lua_pushstring(lua, "allowedGoalId");
        lua_pushinteger(lua, component.allowedGoalId);
        lua_settable(lua, -3);
    }

    static void View(Component::RComponent component)
    {
        ImGui::Text("Allowed goal ID: %i", component.allowedGoalId);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component)
    {
        int32_t val = (int32_t)component.allowedGoalId;
        ImGui::InputInt("Allowed goal ID", &val);
        if(val < 0)
            val = 0;
        component.allowedGoalId = (uint32_t)val;
    }

    static void Duplicate(
        entt::registry & registry,
        const Component::RComponent& component,
        entt::entity target)
    {
        registry.emplace<Component::RComponent>(
            target,
            Component::RComponent{.allowedGoalId = component.allowedGoalId});
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent