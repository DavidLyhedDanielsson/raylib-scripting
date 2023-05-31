#pragma once

#include <entity_reflection/reflection_entity.hpp>
#include <entt/entt.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register.hpp>
#include <lua_impl/lua_register_types.hpp>

#include <entity/behaviour.hpp>
#define RComponent Behaviour
EntityReflectionStruct(RComponent)
{
    ; // This semicolon needs to be here or else clang-format breaks. The benefits of the macro
      // outweigh the weirdness

    static void Create(entt::registry & registry, entt::entity entity) {}

    static LuaValidator::LuaValidator GetLuaValidator(lua_State * lua)
    {
        return LuaValidator::LuaValidator(lua).FieldIs<const char*>("script");
    }

    static bool CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        Component::RComponent component{};

        // TODO: Just allow negative indices in LuaGetFunc to skip this gettop + 1 nonsense
        auto table = lua_gettop(lua);

        lua_getfield(lua, table, "script");
        component.script = lua_tostring(lua, -1);
        lua_pop(lua, 1);

        registry.emplace<Component::RComponent>(entity, component);
        return true;
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component)
    {
        lua_pushstring(lua, "script");
        lua_pushstring(lua, component.script.c_str());
        lua_settable(lua, -3);
    }

    static void View(Component::RComponent & component)
    {
        ImGui::Text("Script: %s", component.script.c_str());
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component)
    {
        constexpr int BUFFER_SIZE = 256;
        char buffer[BUFFER_SIZE];
        snprintf(buffer, BUFFER_SIZE, "%s", component.script.c_str());

        ImGui::InputText("Script", buffer, BUFFER_SIZE);

        component.script = buffer;
    }

    static void Duplicate(
        entt::registry & registry,
        const Component::RComponent& component,
        entt::entity target)
    {
        // Not yet
        // registry.emplace<Component::Camera>(target, component.x, component.y, component.z);
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent