#pragma once

#include <entity_reflection/reflection_entity.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register_types.hpp>

#include <entity/nav_gate.hpp>
#define RComponent NavGate
EntityReflectionStruct(RComponent)
{
    ; // This semicolon needs to be here or else clang-format breaks. The benefits of the macro
      // outweigh the weirdness
    static void Create(entt::registry & registry, entt::entity entity)
    {
        registry.emplace<Component::RComponent>(
            entity,
            Component::RComponent{.allowedGoalIds = {}});
    }

    static LuaValidator::LuaValidator GetLuaValidator(lua_State * lua)
    {
        return LuaValidator::LuaValidator(
            lua); // I don't feel like implementing this for std::vector
    }

    static void CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        std::vector<uint32_t> allowedGoalIds;

        lua_getfield(lua, -1, "allowedGoalIds");
        for(uint32_t i = 0; i < lua_rawlen(lua, -1); ++i)
        {
            lua_rawgeti(lua, -1, i + 1);
            allowedGoalIds.push_back(lua_tointeger(lua, -1));
            lua_pop(lua, 1);
        }

        registry.emplace<Component::RComponent>(
            entity,
            Component::RComponent{.allowedGoalIds = std::move(allowedGoalIds)});

        lua_pop(lua, 1);
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component)
    {
        lua_createtable(lua, 0, 0);
        for(uint32_t i = 0; i < component.allowedGoalIds.size(); ++i)
        {
            lua_pushinteger(lua, component.allowedGoalIds[i]);
            lua_rawseti(lua, -2, i + 1);
        }
        lua_setfield(lua, -2, "allowedGoalIds");
    }

    static void View(Component::RComponent component)
    {
        ImGui::Text("Allowed goal IDs");
        for(uint32_t i = 0; i < component.allowedGoalIds.size(); ++i)
            ImGui::Text("%u", component.allowedGoalIds[i]);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component)
    {
        ImGui::Text("Allowed goal IDs");
        for(uint32_t i = 0; i < component.allowedGoalIds.size(); ++i)
        {
            ImGui::Text("%u", component.allowedGoalIds[i]);
            ImGui::SameLine();
            ImGui::PushID(i);
            if(ImGui::SmallButton("-"))
            {
                component.allowedGoalIds.erase(component.allowedGoalIds.begin() + i);
                --i;
            }
            ImGui::PopID();
        }

        static int val = 0;
        ImGui::InputInt("New ID", &val);
        ImGui::SameLine();
        bool canAdd =
            std::find(component.allowedGoalIds.begin(), component.allowedGoalIds.end(), val)
            == component.allowedGoalIds.end();

        if(!canAdd)
            ImGui::BeginDisabled();
        if(ImGui::Button("+"))
        {
            component.allowedGoalIds.push_back(val);
            val = 0;
        }
        if(!canAdd)
            ImGui::EndDisabled();
    }

    static void Duplicate(
        entt::registry & registry,
        const Component::RComponent& component,
        entt::entity target)
    {
        registry.emplace<Component::RComponent>(
            target,
            Component::RComponent{.allowedGoalIds = component.allowedGoalIds});
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent