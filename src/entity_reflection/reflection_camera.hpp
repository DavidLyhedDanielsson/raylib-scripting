#pragma once

#include "reflection_entity.hpp"
#include <entity/camera.hpp>
#include <entt/entt.hpp>
#include <external/raylib.hpp>

struct CameraReflection
    : public ReflectionComponent<CameraReflection, Component::Camera, __COUNTER__>
{
    static void Create(entt::registry& registry, entt::entity entity)
    {
        // registry.emplace<Component::Camera>(entity, 0.0f, 0.0f, 0.0f);
    }

    static void View(Component::Camera& component)
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
        entt::registry& registry,
        entt::entity entity,
        Component::Camera& component,
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
        entt::registry& registry,
        const Component::Camera& component,
        entt::entity target)
    {
        // Not yet
        // registry.emplace<Component::Camera>(target, component.x, component.y, component.z);
    }
};
// Quick hack to call constructor and register self
static CameraReflection ImguiCameraInstance{};