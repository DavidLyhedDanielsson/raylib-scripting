#pragma once

#include "reflection_entity.hpp"
#include <entity/transform.hpp>
#include <external/raylib.hpp>
#include <lua/lua_register_types.hpp>

static const char transformReflection[] = "transform";
struct TransformReflection
    : public ReflectionComponent<TransformReflection, Component::Transform, transformReflection>
{
    static void Create(entt::registry& registry, entt::entity entity)
    {
        registry.emplace<Component::Transform>(entity, 0.0f, 0.0f, 0.0f);
    }

    static void CreateFromLua(
        lua_State* lua,
        entt::registry& registry,
        entt::entity entity,
        int stackIndex)
    {
        Component::Transform component{};

        lua_getfield(lua, stackIndex, "position");

        component.position = LuaRegister::LuaGetFunc<Vector3>(lua, stackIndex + 1);
        // TODO
        // component.rotation = LuaRegister::LuaGetFunc<Vector3>(lua, -1);

        registry.emplace<Component::Transform>(entity, component);
    }

    static void View(Component::Transform& component)
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
        entt::registry& registry,
        entt::entity entity,
        Component::Transform& component,
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
        entt::registry& registry,
        const Component::Transform& component,
        entt::entity target)
    {
        registry.emplace<Component::Transform>(target, component.position, component.rotation);
    }
};
// Quick hack to call constructor and register self
static TransformReflection ImguiTransformInstance{};