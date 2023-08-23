#pragma once

#include <entity_reflection/reflection_entity.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register_types.hpp>

#include <component/enemy_goal.hpp>
#define RComponent EnemyGoal
EntityReflectionStruct(RComponent)
{
    ; // This semicolon needs to be here or else clang-format breaks. The benefits of the macro
      // outweigh the weirdness
    static void Create(entt::registry & registry, entt::entity entity)
    {
        registry.emplace<Component::RComponent>(entity, Component::RComponent{.ids = {}});
    }

    static LuaValidator::LuaValidator GetLuaValidator(lua_State * lua)
    {
        return LuaValidator::LuaValidator(lua);
    }

    static void CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        std::vector<uint32_t> ids;

        lua_getfield(lua, -1, "ids");
        for(int i = 0; i < (int)lua_rawlen(lua, -1); ++i)
        {
            lua_rawgeti(lua, -1, i + 1);
            ids.push_back(lua_tointeger(lua, -1));
            lua_pop(lua, 1);
        }

        registry.emplace<Component::RComponent>(entity, Component::RComponent{.ids = ids});

        lua_pop(lua, 1);
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component)
    {
        lua_createtable(lua, component.ids.size(), 0);
        for(uint32_t i = 0; i < component.ids.size(); ++i)
        {
            lua_pushinteger(lua, component.ids[i]);
            lua_rawseti(lua, -2, i + 1);
        }
        lua_setfield(lua, -2, "ids");
    }

    static void View(Component::RComponent component)
    {
        ImGui::Text("Goal IDs");
        for(uint32_t i = 0; i < component.ids.size(); ++i)
            ImGui::Text("%u", component.ids[i]);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component)
    {
        ImGui::Text("Goal IDs");
        for(uint32_t i = 0; i < component.ids.size(); ++i)
        {
            ImGui::Text("%u", component.ids[i]);
            ImGui::SameLine();
            ImGui::PushID(i);
            if(ImGui::SmallButton("-"))
            {
                component.ids.erase(component.ids.begin() + i);
                --i;
            }
            ImGui::PopID();
        }

        static int val = 0;
        ImGui::InputInt("New ID", &val);
        ImGui::SameLine();
        bool canAdd =
            std::find(component.ids.begin(), component.ids.end(), val) == component.ids.end();

        if(!canAdd)
            ImGui::BeginDisabled();
        if(ImGui::Button("+"))
        {
            component.ids.push_back(val);
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
            Component::RComponent{.ids = component.ids});
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent