#pragma once

#include <assets.hpp>
#include <entity_reflection/reflection_entity.hpp>
#include <external/lua.hpp>
#include <lua_impl/lua_register_types.hpp>

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

    static LuaValidator::LuaValidator GetLuaValidator(lua_State * lua)
    {
        return LuaValidator::LuaValidator(lua).FieldIs<const char*>("assetName");
    }

    static void CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        lua_getfield(lua, -1, "assetName");
        const char* assetName = lua_tostring(lua, lua_gettop(lua));

        for(const auto& [key, value] : loadedAssets)
        {
            if(key == assetName)
            {
                registry.emplace<Component::RComponent>(
                    entity,
                    Component::RComponent{
                        .assetName = key.c_str(),
                        .model = value,
                        .boundingBox = GetModelBoundingBox(value)});
                break;
            }
        }
        lua_pop(lua, 1);
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component)
    {
        lua_pushstring(lua, "assetName");
        lua_pushstring(lua, component.assetName);
        lua_settable(lua, -3);
    }

    static void View(Component::RComponent & component)
    {
        ImGui::Text("Asset: %s", component.assetName);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component)
    {
        if(ImGui::BeginCombo("Asset", component.assetName))
        {
            for(const auto& [key, value] : loadedAssets)
            {
                bool selected = key == component.assetName;
                if(ImGui::Selectable(key.c_str(), selected))
                {
                    component.assetName = key.c_str(); // Not very safe but fine
                    component.model = value;
                }
                if(selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
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