#pragma once

#include "reflection_entity.hpp"
#include <entity/tile.hpp>
#include <external/raylib.hpp>
#include <lua/lua_register_types.hpp>

static const char tileReflection[] = "tile";
struct TileReflection: public ReflectionComponent<TileReflection, Component::Tile, tileReflection>
{
    static void Create(entt::registry& registry, entt::entity entity)
    {
        registry.emplace<Component::Tile>(entity);
    }

    static void CreateFromLua(
        lua_State* lua,
        entt::registry& registry,
        entt::entity entity,
        int stackIndex)
    {
        registry.emplace<Component::Tile>(entity);
    }

    static void View() {}

    static void Modify(entt::registry& registry, entt::entity entity, bool allowDeletion)
    {
        if(allowDeletion)
            AddRemoveButton("REMOVE TILE COMPONENT", registry, entity);
    }

    static void Duplicate(entt::registry& registry, entt::entity target)
    {
        registry.emplace<Component::Tile>(target);
    }
};
// Quick hack to call constructor and register self
static TileReflection ImguiTileInstance{};