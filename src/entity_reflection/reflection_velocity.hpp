#pragma once

#include "reflection_entity.hpp"
#include <entt/entt.hpp>
#include <external/raylib.hpp>
#include <lua/lua_register_types.hpp>

#include <entity/velocity.hpp>
#define RComponent Velocity
EntityReflectionStruct(RComponent)
{
    ; // This semicolon needs to be here or else clang-format breaks. The benefits of the macro
      // outweigh the weirdness

    static void Create(entt::registry & registry, entt::entity entity)
    {
        registry.emplace<Component::RComponent>(entity, 0.0f, 0.0f, 0.0f);
    }

    static void CreateFromLua(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity,
        int stackIndex)
    {
        Component::RComponent component{};

        lua_getfield(lua, stackIndex, "x");
        component.x = lua_tonumber(lua, -1);
        lua_getfield(lua, stackIndex, "y");
        component.y = lua_tonumber(lua, -1);
        lua_getfield(lua, stackIndex, "z");
        component.z = lua_tonumber(lua, -1);

        registry.emplace<Component::RComponent>(entity, component);
    }

    static void View(Component::RComponent & component)
    {
        ImGui::Text("Velocity: %f, %f, %f", component.x, component.y, component.z);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component,
        bool allowDeletion)
    {
        ImGui::DragFloat3("Velocity", &component.x, 0.001f);

        if(allowDeletion)
            AddRemoveButton("REMOVE VELOCITY COMPONENT", registry, entity);
    }

    static void Duplicate(
        entt::registry & registry,
        const Component::RComponent& component,
        entt::entity target)
    {
        registry.emplace<Component::RComponent>(target, component.x, component.y, component.z);
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent