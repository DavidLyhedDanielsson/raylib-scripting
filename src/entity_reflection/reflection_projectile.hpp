#pragma once

#include <entity_reflection/reflection_entity.hpp>
#include <external/imgui.hpp>
#include <external/lua.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register_types.hpp>

#include <component/projectile.hpp>
#define RComponent Projectile
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
        return LuaValidator::LuaValidator(lua).FieldIs<float>("damage");
    }

    static void CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        lua_getfield(lua, -1, "damage");

        registry.emplace<Component::RComponent>(
            entity,
            Component::RComponent{.damage = (float)lua_tonumber(lua, -1)});

        lua_pop(lua, 1);
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component)
    {
        lua_pushstring(lua, "damage");
        lua_pushnumber(lua, component.damage);
        lua_settable(lua, -3);
    }

    static void View(Component::RComponent & component)
    {
        ImGui::Text("Damage %f", component.damage);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component)
    {
        ImGui::InputFloat("Damage", &component.damage);
    }

    static void Duplicate(
        entt::registry & registry,
        Component::RComponent & component,
        entt::entity target)
    {
        registry.emplace<Component::RComponent>(
            target,
            Component::RComponent{.damage = component.damage});
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent