#pragma once

#include "reflection_entity.hpp"
#include <assets.hpp>
#include <lua/lua_register_types.hpp>

#include <entity/render.hpp>
#define RComponent Render
EntityReflectionStruct(RComponent)
{
    ; // This semicolon needs to be here or else clang-format breaks. The benefits of the macro
      // outweigh the weirdness

    static void Create(entt::registry & registry, entt::entity entity)
    {
        // registry.emplace<Component::Render>(
        //     entity,
        //     GetAssetName(Asset::Insurgent),
        //     GetLoadedAsset(Asset::Insurgent));
    }

    static void CreateFromLua(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity,
        int stackIndex)
    {
        std::string_view assetName = lua_tostring(lua, stackIndex);
        for(const auto& [key, value] : loadedAssets)
        {
            if(key == assetName)
            {
                registry.emplace<Component::RComponent>(
                    entity,
                    Component::RComponent{.assetName = key.c_str(), .model = value});
                break;
            }
        }
    }

    static void View(Component::RComponent & component)
    {
        ImGui::Text("Asset: %s", component.assetName);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component,
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
        entt::registry & registry,
        const Component::RComponent& component,
        entt::entity target)
    {
        registry.emplace<Component::RComponent>(target, component.assetName, component.model);
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent