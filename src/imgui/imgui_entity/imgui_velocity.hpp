#pragma once

#include "imgui_entity.hpp"
#include <entity/velocity.hpp>
#include <entt/entt.hpp>
#include <external/raylib.hpp>

struct ImguiVelocity: public ImGuiEntity<ImguiVelocity, Component::Velocity, __COUNTER__>
{
    static void Create(entt::registry& registry, entt::entity entity)
    {
        registry.emplace<Component::Velocity>(entity, 0.0f, 0.0f, 0.0f);
    }

    static void View(Component::Velocity& component)
    {
        ImGui::Text("Velocity: %f, %f, %f", component.x, component.y, component.z);
    }

    static void Modify(
        entt::registry& registry,
        entt::entity entity,
        Component::Velocity& component,
        bool allowDeletion)
    {
        ImGui::DragFloat3("Velocity", &component.x, 0.001f);

        if(allowDeletion)
            AddRemoveButton("REMOVE VELOCITY COMPONENT", registry, entity);
    }

    static void Duplicate(
        entt::registry& registry,
        const Component::Velocity& component,
        entt::entity target)
    {
        registry.emplace<Component::Velocity>(target, component.x, component.y, component.z);
    }
};
static ImguiVelocity ImguiVelocityInstance{};