#pragma once

#include <numeric>
#include <vector>

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
        return LuaValidator::LuaValidator(lua).FieldIs<Vector3>("position");
    }

    static void CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        Component::RComponent component{};

        auto stackIndex = lua_gettop(lua);

        lua_getfield(lua, stackIndex - 2, "position");

        component.position = LuaRegister::LuaGetFunc<Vector3>(lua, stackIndex - 2);
        // TODO
        // component.rotation = LuaRegister::LuaGetFunc<Vector3>(lua, -1);

        registry.emplace<Component::RComponent>(entity, component);
    }

    static void View(Component::RComponent & component)
    {
        ImGui::Text(
            "Position: %f, %f, %f",
            component.position.x,
            component.position.y,
            component.position.z);

        Vector3 eulerAngles = QuaternionToEuler(component.rotation);
        ImGui::Text("Rotation: %f, %f, %f", eulerAngles.x, eulerAngles.y, eulerAngles.z);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component,
        bool allowDeletion)
    {
        ImGui::DragFloat3("Position", &component.position.x);

        Vector3 eulerAngles = QuaternionToEuler(component.rotation);
        ImGui::DragFloat3("Rotation", &eulerAngles.x);
        component.rotation = QuaternionFromEuler(eulerAngles.x, eulerAngles.y, eulerAngles.z);

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