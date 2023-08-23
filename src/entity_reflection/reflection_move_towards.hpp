#pragma once

#include <external/lua.hpp>
#include <numeric>
#include <vector>

#include <entity_reflection/reflection_entity.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register.hpp>
#include <lua_impl/lua_register_types.hpp>
#include <lua_impl/lua_validator.hpp>

#include <component/move_towards.hpp>
#define RComponent MoveTowards
EntityReflectionStruct(RComponent)
{
    ; // This semicolon needs to be here or else clang-format breaks. The benefits of the macro
      // outweigh the weirdness

    static void Create(entt::registry & registry, entt::entity entity)
    {
        // TODO: emplace the component object
        registry.emplace<Component::RComponent>(
            entity,
            Component::RComponent{.vectorFieldId = 0, .speed = 1.0f});
    }

    static LuaValidator::LuaValidator GetLuaValidator(lua_State * lua)
    {
        return LuaValidator::LuaValidator(lua)
            .FieldIs<uint32_t>("vectorFieldId")
            .FieldIs<float>("speed");
    }

    static void CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        lua_getfield(lua, -1, "vectorFieldId");
        lua_getfield(lua, -2, "speed");

        auto stackTop = lua_gettop(lua) + 1;
        registry.emplace<Component::RComponent>(
            entity,
            Component::RComponent{
                .vectorFieldId = LuaRegister::LuaGetFunc<uint32_t>(lua, stackTop - 2),
                .speed = (float)lua_tonumber(lua, stackTop - 1),
            });

        lua_pop(lua, 2);
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component)
    {
        lua_pushstring(lua, "vectorFieldId");
        LuaRegister::LuaSetFunc<uint32_t>(lua, component.vectorFieldId);
        lua_settable(lua, -3);
        lua_pushstring(lua, "speed");
        lua_pushnumber(lua, component.speed);
        lua_settable(lua, -3);
    }

    static void View(Component::RComponent & component)
    {
        ImGui::Text("Field ID: %ui", component.vectorFieldId);
        ImGui::Text("Speed: %f", component.speed);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component)
    {
        int32_t val = (int32_t)component.vectorFieldId;
        ImGui::InputInt("Field ID", &val);
        if(val < 0)
            val = 0;
        component.vectorFieldId = (uint32_t)val;
        ImGui::DragFloat("Speed", &component.speed, 0.01f, 0.0f);
    }

    static void Duplicate(
        entt::registry & registry,
        const Component::RComponent& component,
        entt::entity target)
    {
        registry.emplace<Component::RComponent>(
            target,
            Component::RComponent{
                .vectorFieldId = component.vectorFieldId,
                .speed = component.speed});
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent