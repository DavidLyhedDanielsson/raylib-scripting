#pragma once

#include "reflection_entity.hpp"
#include <entity/render.hpp>

#include <assets.hpp>

struct ImguiRender: public ImGuiEntity<ImguiRender, Component::Render, __COUNTER__>
{
    static void Create(entt::registry& registry, entt::entity entity)
    {
        // registry.emplace<Component::Render>(
        //     entity,
        //     GetAssetName(Asset::Insurgent),
        //     GetLoadedAsset(Asset::Insurgent));
    }

    static void View(Component::Render& component)
    {
        ImGui::Text("Asset: %s", component.assetName);
    }

    static void Modify(
        entt::registry& registry,
        entt::entity entity,
        Component::Render& component,
        bool allowDeletion)
    {
        if(ImGui::BeginCombo("Asset", component.assetName))
        {
            for(const auto& [key, value] : loadedAssets)
            {
                if(ImGui::Selectable(key.c_str(), key == component.assetName))
                {
                    component.assetName = key.c_str(); // Not very safe but fine
                    component.model = value;
                }
            }
            ImGui::EndCombo();
        }

        if(allowDeletion)
            AddRemoveButton("REMOVE RENDER COMPONENT", registry, entity);
    }

    static void Duplicate(
        entt::registry& registry,
        const Component::Render& component,
        entt::entity target)
    {
        registry.emplace<Component::Render>(target, component.assetName, component.model);
    }
};
static ImguiRender ImguiRenderInstance{};