#pragma once

#include <numeric>
#include <vector>

#include "lua/lua_register.hpp"
#include "reflection_entity.hpp"
#include <external/raylib.hpp>
#include <lua/lua_register_types.hpp>
#include <lua/lua_validator.hpp>

#include <entity/transform.hpp>
#define RComponent Transform
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
            .FieldIs<Vector3>("position")
            .FieldIs<Vector3>("rotation");
    }

    static void CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        lua_getfield(lua, -1, "position");
        lua_getfield(lua, -2, "rotation");

        auto stackTop = lua_gettop(lua) + 1;
        registry.emplace<Component::RComponent>(
            entity,
            Component::RComponent{
                .position = LuaRegister::LuaGetFunc<Vector3>(lua, stackTop - 2),
                .rotation = LuaRegister::LuaGetFunc<Vector3>(lua, stackTop - 1),
            });

        lua_pop(lua, 2);
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component)
    {
        lua_pushstring(lua, "position");
        LuaRegister::LuaSetFunc<Vector3>(lua, component.position);
        lua_settable(lua, -3);
        lua_pushstring(lua, "rotation");
        LuaRegister::LuaSetFunc<Vector3>(lua, component.rotation);
        lua_settable(lua, -3);
    }

    static void View(Component::RComponent & component)
    {
        ImGui::Text(
            "Position: %f, %f, %f",
            component.position.x,
            component.position.y,
            component.position.z);

        ImGui::Text(
            "Rotation: %f, %f, %f",
            component.rotation.x,
            component.rotation.y,
            component.rotation.z);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component,
        bool allowDeletion)
    {
        ImGui::DragFloat3("Position", &component.position.x);
        ImGui::SliderFloat3("Rotation", &component.rotation.x, 0.0f, PI * 2.0f);

        if(allowDeletion)
            AddRemoveButton("REMOVE TRANSFORM COMPONENT", registry, entity);
    }

    static void Duplicate(
        entt::registry & registry,
        const Component::RComponent& component,
        entt::entity target)
    {
        registry.emplace<Component::RComponent>(target, component.position, component.rotation);
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent