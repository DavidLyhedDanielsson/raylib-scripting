#pragma once

#include "reflection_entity.hpp"
#include <external/raylib.hpp>
#include <lua/lua_register_types.hpp>

#include <entity/tile.hpp>
#define RComponent Tile
EntityReflectionStruct(RComponent)
{
    ; // This semicolon needs to be here or else clang-format breaks. The benefits of the macro
      // outweigh the weirdness
    static void Create(entt::registry & registry, entt::entity entity)
    {
        registry.emplace<Component::RComponent>(entity);
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
        registry.emplace<Component::RComponent>(entity);
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component) {}

    static void View() {}

    static void Modify(entt::registry & registry, entt::entity entity, bool allowDeletion)
    {
        if(allowDeletion)
            AddRemoveButton("REMOVE TILE COMPONENT", registry, entity);
    }

    static void Duplicate(entt::registry & registry, entt::entity target)
    {
        registry.emplace<Component::RComponent>(target);
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent