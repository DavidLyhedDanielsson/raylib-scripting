#pragma once

#include "imgui_entity.hpp"
#include <entity/render.hpp>

#include <assets.hpp>

struct ImguiRender: public ImGuiEntity<ImguiRender, Component::Render, __COUNTER__>
{
    static void Create(entt::registry& registry, entt::entity entity)
    {
        registry.emplace<Component::Render>(
            entity,
            GetAssetName(Asset::Insurgent),
            GetLoadedAsset(Asset::Insurgent));
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
        Asset newAsset = Asset::Last;
        if(ImGui::BeginCombo("Asset", component.assetName))
        {
            for(Asset asset = (Asset)0; (int)asset < (int)Asset::Last;
                asset = (Asset)((int)asset + 1))
            {
                if(ImGui::Selectable(
                       GetAssetName(asset),
                       GetAssetName(asset) == component.assetName))
                {
                    newAsset = asset;
                }
            }
            ImGui::EndCombo();
        }
        if(newAsset != Asset::Last)
        {
            component.assetName = GetAssetName(newAsset);
            component.model = GetLoadedAsset(newAsset);
        }

        if(allowDeletion)
            AddRemoveButton("REMOVE RENDER COMPONENT", registry, entity);
    }
};
static ImguiRender ImguiRenderInstance{};