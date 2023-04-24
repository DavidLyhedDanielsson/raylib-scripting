#pragma once

#include "reflection_entity.hpp"
#include <entt/entt.hpp>
#include <external/raylib.hpp>
#include <lua/lua_register_types.hpp>

#include <entity/camera.hpp>
#define RComponent Camera
EntityReflectionStruct(RComponent)
{
    ; // This semicolon needs to be here or else clang-format breaks. The benefits of the macro
      // outweigh the weirdness

    static void Create(entt::registry & registry, entt::entity entity) {}

    static void CreateFromLua(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity,
        int stackIndex)
    {
        Component::RComponent component{};

        component.target = LuaRegister::LuaGetFunc<Vector3>(lua, -4);
        component.up = LuaRegister::LuaGetFunc<Vector3>(lua, -3);
        component.fovy = lua_tonumber(lua, -2);
        component.projection = lua_tointeger(lua, -1);

        registry.emplace<Component::RComponent>(entity, component);
    }

    static void GetToLua(entt::registry & registry, entt::entity entity) {}

    static void View(Component::RComponent & component)
    {
        ImGui::Text(
            "Target: %f, %f, %f",
            component.target.x,
            component.target.y,
            component.target.z);
        ImGui::Text("Up: %f, %f, %f", component.up.x, component.up.y, component.up.z);
        ImGui::Text("FoV Vertical: %f", component.fovy);
        if(component.projection == CAMERA_PERSPECTIVE)
            ImGui::Text("Projection: Perspective");
        else
            ImGui::Text("Projection: Orthographic");
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component,
        bool allowDeletion)
    {
        ImGui::InputFloat3("Target", &component.target.x);
        ImGui::InputFloat3("Up", &component.up.x);
        ImGui::DragFloat("FoV Vertical", &component.fovy, 1.0f, 20.0f, 150.0f);
        if(ImGui::RadioButton("Perspective", component.projection == CAMERA_PERSPECTIVE))
            component.projection = CAMERA_PERSPECTIVE;
        ImGui::SameLine();
        if(ImGui::RadioButton("Orthographic", component.projection == CAMERA_ORTHOGRAPHIC))
            component.projection = CAMERA_ORTHOGRAPHIC;

        if(allowDeletion)
            AddRemoveButton("REMOVE CAMERA COMPONENT", registry, entity);
    }

    static void Duplicate(
        entt::registry & registry,
        const Component::RComponent& component,
        entt::entity target)
    {
        // Not yet
        // registry.emplace<Component::Camera>(target, component.x, component.y, component.z);
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent