#pragma once

#include <numbers>
#include <numeric>
#include <vector>

#include <entity_reflection/reflection_entity.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register.hpp>
#include <lua_impl/lua_register_types.hpp>
#include <lua_impl/lua_validator.hpp>

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
            component.rotation.x * RAD2DEG,
            component.rotation.y * RAD2DEG,
            component.rotation.z * RAD2DEG);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component)
    {
        ImGui::DragFloat3("Position", &component.position.x);

        // Use doubles to avoid values changing around too much due to RAD->DEG->RAD
        // Probably overkill, but I've never done it before
        double rot[3]{
            component.rotation.x * (180.0 / std::numbers::pi),
            component.rotation.y * (180.0 / std::numbers::pi),
            component.rotation.z * (180.0 / std::numbers::pi),
        };
        ImGui::DragScalarN("Rotation", ImGuiDataType_Double, rot, 3, 1.0f, NULL, NULL, "%.3f");
        component.rotation.x = rot[0] * (std::numbers::pi / 180);
        component.rotation.y = rot[1] * (std::numbers::pi / 180);
        component.rotation.z = rot[2] * (std::numbers::pi / 180);
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