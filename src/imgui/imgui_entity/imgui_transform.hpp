#pragma once

#include "imgui_entity.hpp"
#include <entity/transform.hpp>
#include <external/raylib.hpp>

struct ImguiTransform: public ImGuiEntity<ImguiTransform, Component::Transform, __COUNTER__>
{
    static void Create(entt::registry& registry, entt::entity entity)
    {
        registry.emplace<Component::Transform>(entity, 0.0f, 0.0f, 0.0f);
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
static ImguiTransform ImguiTransformInstance{};