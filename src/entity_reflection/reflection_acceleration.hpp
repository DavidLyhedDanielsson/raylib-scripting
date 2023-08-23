#pragma once

#include <numbers>
#include <numeric>
#include <vector>

#include <entity_reflection/reflection_entity.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register.hpp>
#include <lua_impl/lua_register_types.hpp>
#include <lua_impl/lua_validator.hpp>

#include <component/acceleration.hpp>
#define RComponent Acceleration
EntityReflectionStruct(RComponent)
{
    ; // This semicolon needs to be here or else clang-format breaks. The benefits of the macro
      // outweigh the weirdness

    static void Create(entt::registry & registry, entt::entity entity)
    {
        registry.emplace<Component::RComponent>(
            entity,
            Component::RComponent{Vector3{0.0f, 0.0f, 0.0f}});
    }

    static LuaValidator::LuaValidator GetLuaValidator(lua_State * lua)
    {
        return LuaValidator::LuaValidator(lua).FieldIs<Vector3>("acceleration");
    }

    static void CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        lua_getfield(lua, -1, "acceleration");

        auto stackTop = lua_gettop(lua) + 1;
        registry.emplace<Component::RComponent>(
            entity,
            Component::RComponent{
                .acceleration = LuaRegister::LuaGetFunc<Vector3>(lua, stackTop - 1),
            });

        lua_pop(lua, 1);
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component)
    {
        lua_pushstring(lua, "acceleration");
        LuaRegister::LuaSetFunc<Vector3>(lua, component.acceleration);
        lua_settable(lua, -3);
    }

    static void View(Component::RComponent & component)
    {
        ImGui::Text(
            "Acceleration: %f, %f, %f",
            component.acceleration.x,
            component.acceleration.y,
            component.acceleration.z);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component)
    {
        ImGui::DragFloat3("Acceleration", &component.acceleration.x);
    }

    static void Duplicate(
        entt::registry & registry,
        const Component::RComponent& component,
        entt::entity target)
    {
        registry.emplace<Component::RComponent>(target, component.acceleration);
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent