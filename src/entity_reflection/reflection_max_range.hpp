#pragma once

#include <entity_reflection/reflection_entity.hpp>
#include <external/imgui.hpp>
#include <external/lua.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register.hpp>
#include <lua_impl/lua_register_types.hpp>

#include <entity/max_range.hpp>
#define RComponent MaxRange
EntityReflectionStruct(RComponent)
{
    ; // This semicolon needs to be here or else clang-format breaks. The benefits of the macro
      // outweigh the weirdness
    static void Create(entt::registry & registry, entt::entity entity)
    {
        registry.emplace<Component::RComponent>(entity);
    }

    static LuaValidator::LuaValidator GetLuaValidator(lua_State * lua)
    {
        return LuaValidator::LuaValidator(lua)
            .FieldIs<float>("maxDistance")
            .FieldIs<Vector3>("distanceFrom");
    }

    static void CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        lua_getfield(lua, -1, "maxDistance");
        lua_getfield(lua, -2, "distanceFrom");

        registry.emplace<Component::RComponent>(
            entity,
            Component::RComponent{
                .maxDistance = (float)lua_tonumber(lua, -2),
                .distanceFrom = LuaRegister::LuaGetFunc<Vector3>(lua, lua_gettop(lua)),
            });

        lua_pop(lua, 2);
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component)
    {
        lua_pushstring(lua, "range");
        lua_pushnumber(lua, component.maxDistance);
        lua_settable(lua, -3);

        lua_pushstring(lua, "distanceFrom");
        LuaRegister::LuaSetFunc<Vector3>(lua, component.distanceFrom);
        lua_settable(lua, -3);
    }

    static void View(Component::RComponent & component)
    {
        ImGui::Text("Max Distance: %f", component.maxDistance);
        ImGui::Text(
            "Distance from: %f, %f, %f",
            component.distanceFrom.x,
            component.distanceFrom.y,
            component.distanceFrom.z);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component)
    {
        ImGui::InputFloat("Max Distance", &component.maxDistance);
        ImGui::InputFloat3("Distance from", &component.distanceFrom.x);
    }

    static void Duplicate(
        entt::registry & registry,
        Component::RComponent & component,
        entt::entity target)
    {
        registry.emplace<Component::RComponent>(
            target,
            Component::RComponent{
                .maxDistance = component.maxDistance,
                .distanceFrom = component.distanceFrom});
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent