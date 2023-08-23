#include "lua_world_impl.hpp"
#include <cassert>
#include <iostream>
#include <lua_impl/lua_register.hpp>
#include <lua_impl/lua_register_types.hpp>

#include <component/acceleration.hpp>
#include <component/area_tracker.hpp>
#include <component/behaviour.hpp>
#include <component/camera.hpp>
#include <component/enemy_goal.hpp>
#include <component/enemy_spawn.hpp>
#include <component/health.hpp>
#include <component/max_range.hpp>
#include <component/move_towards.hpp>
#include <component/nav_gate.hpp>
#include <component/projectile.hpp>
#include <component/render.hpp>
#include <component/tile.hpp>
#include <component/transform.hpp>
#include <component/velocity.hpp>
#include <component/walkable.hpp>
#include <navigation.hpp>
#include <world.hpp>

namespace LuaRegister
{
    template<>
    constexpr auto LuaGetFunc<Navigation::Tile> = [](lua_State* lua, int i) {
        assert(false && "Not yet");
    };

    template<>
    constexpr auto LuaSetFunc<Navigation::Tile> = [](lua_State* lua, Navigation::Tile tile) {
        lua_createtable(lua, 0, 0);
        lua_pushinteger(lua, (int)tile.type);
        lua_setfield(lua, -2, "type");
        switch(tile.type)
        {
            case Navigation::Tile::NONE: break;
            case Navigation::Tile::WALKABLE: break;
            case Navigation::Tile::SPAWN:
                lua_pushinteger(lua, tile.spawn.id);
                lua_setfield(lua, -2, "id");
                lua_pushinteger(lua, tile.spawn.goalId);
                lua_setfield(lua, -2, "goalId");
                break;
            case Navigation::Tile::GOAL:
                lua_createtable(lua, 0, 0);
                for(int i = 0; i < tile.goal.numberOfIds; ++i)
                {
                    lua_pushinteger(lua, tile.goal.ids[i]);
                    lua_rawseti(lua, -2, i + 1);
                }
                lua_setfield(lua, -2, "ids");
                break;
            case Navigation::Tile::NAV_GATE:
                lua_createtable(lua, 0, 0);
                for(int i = 0; i < tile.navGate.numberOfGoalIds; ++i)
                {
                    lua_pushinteger(lua, tile.navGate.allowedGoalIds[i]);
                    lua_rawseti(lua, -2, i + 1);
                }
                lua_setfield(lua, -2, "allowedGoalIds");
                break;
        }
    };

    template<>
    constexpr auto GetDefault<Navigation::Tile> = Navigation::Tile{.type = Navigation::Tile::NONE};
}

namespace LuaWorld
{
    void Register(lua_State* lua)
    {
        lua_createtable(lua, 0, 0);

        LuaRegister::PushRegister(
            lua,
            "Build",
            +[](lua_State* lua, float tileSize) {
                auto minVal = std::numeric_limits<float>::lowest();
                auto maxVal = std::numeric_limits<float>::max();

                Vector3 min{maxVal, 0.0f, maxVal};
                Vector3 max{minVal, 0.0f, minVal};

                bool anythingAdded = false;
                World::state.registry->view<Component::Walkable, Component::Transform>().each(
                    [&](Component::Transform transform) {
                        anythingAdded = true;
                        min = Vector3Min(
                            min,
                            {.x = transform.position.x - tileSize,
                             .y = 0.0f,
                             .z = transform.position.z - tileSize});
                        max = Vector3Max(
                            max,
                            {.x = transform.position.x + tileSize,
                             .y = 0.0f,
                             .z = transform.position.z + tileSize});
                    });

                if(!anythingAdded)
                    return;

                min = Vector3Subtract(min, {1.0f, 0.0f, 1.0f});
                max = Vector3Add(max, {1.0f, 0.0f, 1.0f});

                World::state.navigation = Navigation(
                    {min.x, min.z},
                    {max.x, max.z},
                    min.x,
                    min.z,
                    tileSize == 0.0f ? 1.0f : tileSize);

                World::state.registry
                    ->view<Component::Walkable, Component::Render, Component::Transform>()
                    .each([&](entt::entity entity,
                              Component::Render render,
                              Component::Transform transform) {
                        auto bounds = BoundingBoxTransform(
                            render.boundingBox,
                            MatrixRotateZYX(transform.rotation));
                        Vector2 minBounds = {.x = bounds.min.x, .y = bounds.min.z};
                        Vector2 maxBounds = {.x = bounds.max.x, .y = bounds.max.z};

                        Vector2 min =
                            Vector2Add({transform.position.x, transform.position.z}, minBounds);
                        Vector2 max =
                            Vector2Add({transform.position.x, transform.position.z}, maxBounds);

                        if(auto goal = World::state.registry->try_get<Component::EnemyGoal>(entity);
                           goal)
                        {
                            for(auto id : goal->ids)
                                World::state.navigation.SetGoal(id, min, max);
                        }
                        else if(auto spawn =
                                    World::state.registry->try_get<Component::EnemySpawn>(entity);
                                spawn)
                        {
                            World::state.navigation.SetSpawn(spawn->id, spawn->goalId, min, max);
                        }
                        else
                        {
                            World::state.navigation.SetWalkable(min, max);
                        }
                    });

                World::state.registry
                    ->view<Component::NavGate, Component::Render, Component::Transform>()
                    .each([&](entt::entity entity,
                              Component::NavGate navGate,
                              Component::Render render,
                              Component::Transform transform) {
                        auto bounds = BoundingBoxTransform(
                            render.boundingBox,
                            MatrixRotateZYX(transform.rotation));
                        Vector2 minBounds = {.x = bounds.min.x, .y = bounds.min.z};
                        Vector2 maxBounds = {.x = bounds.max.x, .y = bounds.max.z};

                        Vector2 min =
                            Vector2Add({transform.position.x, transform.position.z}, minBounds);
                        Vector2 max =
                            Vector2Add({transform.position.x, transform.position.z}, maxBounds);

                        for(uint32_t i = 0; i < navGate.allowedGoalIds.size(); ++i)
                            World::state.navigation.SetNavGate(navGate.allowedGoalIds[i], min, max);
                    });

                World::state.registry
                    ->view<Component::EnemyGoal, Component::Render, Component::Transform>()
                    .each([&](entt::entity entity,
                              Component::EnemyGoal goal,
                              Component::Render render,
                              Component::Transform transform) {
                        auto bounds = BoundingBoxTransform(
                            render.boundingBox,
                            MatrixRotateZYX(transform.rotation));
                        Vector2 minBounds = {.x = bounds.min.x, .y = bounds.min.z};
                        Vector2 maxBounds = {.x = bounds.max.x, .y = bounds.max.z};

                        Vector2 min =
                            Vector2Add({transform.position.x, transform.position.z}, minBounds);
                        Vector2 max =
                            Vector2Add({transform.position.x, transform.position.z}, maxBounds);

                        for(auto id : goal.ids)
                            World::state.navigation.SetGoal(id, min, max);
                    });

                World::state.registry
                    ->view<Component::EnemySpawn, Component::Render, Component::Transform>()
                    .each([&](entt::entity entity,
                              Component::EnemySpawn spawn,
                              Component::Render render,
                              Component::Transform transform) {
                        auto bounds = BoundingBoxTransform(
                            render.boundingBox,
                            MatrixRotateZYX(transform.rotation));
                        Vector2 minBounds = {.x = bounds.min.x, .y = bounds.min.z};
                        Vector2 maxBounds = {.x = bounds.max.x, .y = bounds.max.z};

                        Vector2 min =
                            Vector2Add({transform.position.x, transform.position.z}, minBounds);
                        Vector2 max =
                            Vector2Add({transform.position.x, transform.position.z}, maxBounds);

                        World::state.navigation.SetSpawn(spawn.id, spawn.goalId, min, max);
                    });

                lua_getglobal(lua, "Navigation");
                lua_pushstring(lua, "sizeX");
                lua_pushinteger(lua, World::state.navigation.GetSizeX());
                lua_settable(lua, -3);
                lua_pushstring(lua, "sizeY");
                lua_pushinteger(lua, World::state.navigation.GetSizeY());
                lua_settable(lua, -3);
                lua_pushstring(lua, "tileSize");
                lua_pushnumber(lua, World::state.navigation.tileSize);
                lua_settable(lua, -3);
                lua_pushstring(lua, "offsetX");
                lua_pushnumber(lua, World::state.navigation.offsetX);
                lua_settable(lua, -3);
                lua_pushstring(lua, "offsetY");
                lua_pushnumber(lua, World::state.navigation.offsetY);
                lua_settable(lua, -3);
                lua_pop(lua, 1);

                // lua_pcall(lua, 0, 0, 0);

                // navigation.Build();
            });

        lua_pushstring(lua, "TileType");
        lua_createtable(lua, 0, 0);

        lua_pushstring(lua, "NONE");
        lua_pushnumber(lua, (int)Navigation::Tile::Type::NONE);
        lua_settable(lua, -3);
        lua_pushstring(lua, "WALKABLE");
        lua_pushnumber(lua, (int)Navigation::Tile::Type::WALKABLE);
        lua_settable(lua, -3);
        lua_pushstring(lua, "SPAWN");
        lua_pushnumber(lua, (int)Navigation::Tile::Type::SPAWN);
        lua_settable(lua, -3);
        lua_pushstring(lua, "GOAL");
        lua_pushnumber(lua, (int)Navigation::Tile::Type::GOAL);
        lua_settable(lua, -3);

        lua_settable(lua, -3);

        lua_pushstring(lua, "TileSide");
        lua_createtable(lua, 0, 0);

        lua_pushstring(lua, "NONE");
        lua_pushnumber(lua, (int)Navigation::Tile::Side::NONE);
        lua_settable(lua, -3);
        lua_pushstring(lua, "TOP");
        lua_pushnumber(lua, (int)Navigation::Tile::Side::TOP);
        lua_settable(lua, -3);
        lua_pushstring(lua, "BOTTOM");
        lua_pushnumber(lua, (int)Navigation::Tile::Side::BOTTOM);
        lua_settable(lua, -3);
        lua_pushstring(lua, "LEFT");
        lua_pushnumber(lua, (int)Navigation::Tile::Side::LEFT);
        lua_settable(lua, -3);
        lua_pushstring(lua, "RIGHT");
        lua_pushnumber(lua, (int)Navigation::Tile::Side::RIGHT);
        lua_settable(lua, -3);

        lua_settable(lua, -3);

        LuaRegister::PushRegister(
            lua,
            "ForEachTile",
            +[](lua_State* lua, LuaRegister::Placeholder callback) {
                static bool error = false;
                error = false;
                World::state.navigation.ForEachTile(
                    [&](uint32_t x, uint32_t y, Navigation::Tile tile) {
                        if(error)
                            return;

                        lua_pushvalue(lua, callback.stackIndex);

                        lua_pushinteger(lua, x);
                        lua_pushinteger(lua, y);
                        LuaRegister::LuaSetFunc<Navigation::Tile>(lua, tile);
                        if(lua_pcall(lua, 3, 0, 0) != LUA_OK)
                        {
                            std::cerr << lua_tostring(lua, -1) << std::endl;
                            error = true;
                        }
                    });
            });

        LuaRegister::PushRegister(
            lua,
            "GetTileSpace",
            +[](lua_State* lua, Vector2 position) {
                return World::state.navigation.GetTileSpace(position);
            });

        LuaRegister::PushRegister(
            lua,
            "ConvertToTileSpace",
            +[](lua_State* lua, Vector2 min, Vector2 max) {
                return World::state.navigation.ConvertToTileSpace(min, max);
            });

        // TODO: These below here can be quick registered?
        LuaRegister::PushRegister(
            lua,
            "IsReachable",
            +[](lua_State* lua, int x, int y) {
                return World::state.navigation.IsReachable(x, y);
            });

        LuaRegister::PushRegister(
            lua,
            "IsWalkable",
            +[](lua_State* lua, int x, int y) { return World::state.navigation.IsWalkable(x, y); });

        LuaRegister::PushRegister(
            lua,
            "IsNavGate",
            +[](lua_State* lua, int x, int y) { return World::state.navigation.IsNavGate(x, y); });

        LuaRegister::PushRegister(
            lua,
            "IsPassable",
            +[](lua_State* lua, int spawnId, int goalId, int x, int y) {
                return World::state.navigation.IsPassable(spawnId, goalId, x, y);
            });

        LuaRegister::PushRegister(
            lua,
            "SetVectorField",
            +[](lua_State* lua, int32_t fieldId, LuaRegister::Placeholder table) {
                auto sizeY = luaL_len(lua, table.stackIndex);
                lua_geti(lua, table.stackIndex, 1);
                auto sizeX = luaL_len(lua, -1);
                lua_pop(lua, 1);

                std::vector<std::vector<Vector2>> vectorField(
                    sizeY,
                    std::vector<Vector2>(sizeX, {.x = 0.0f, .y = 0.0f}));
                for(uint32_t y = 0; y < sizeY; ++y)
                {
                    lua_geti(lua, table.stackIndex, y + 1);
                    for(uint32_t x = 0; x < sizeX; ++x)
                    {
                        lua_geti(lua, -1, x + 1);
                        lua_getfield(lua, -1, "x");
                        lua_getfield(lua, -2, "y");
                        vectorField[y][x] = {
                            .x = (float)lua_tonumber(lua, -2),
                            .y = (float)lua_tonumber(lua, -1),
                        };

                        lua_pop(lua, 3);
                    }
                    lua_pop(lua, 1);
                }

                World::state.navigation.SetVectorField(fieldId, std::move(vectorField));
            });

        LuaRegister::PushRegister(
            lua,
            "SetWall",
            +[](lua_State* lua, int x, int y, int side) {
                if(x < 0)
                    return;
                if(y < 0)
                    return;

                World::state.navigation.SetWall(x, y, (Navigation::Tile::Side)side);
            });

        lua_pushstring(lua, "drawTiles");
        lua_pushboolean(lua, World::state.drawNavigationTiles);
        lua_settable(lua, -3);
        lua_pushstring(lua, "drawField");
        if(World::state.drawNavigationField)
            lua_pushinteger(lua, World::state.drawNavigationField.value());
        else
            lua_pushnil(lua);
        lua_settable(lua, -3);
        lua_pushstring(lua, "ksi");
        lua_pushnumber(lua, 6.0f);
        lua_settable(lua, -3);
        lua_pushstring(lua, "avoidanceLookAhead");
        lua_pushnumber(lua, 3.0f);
        lua_settable(lua, -3);
        lua_pushstring(lua, "obstacleLookAhead");
        lua_pushnumber(lua, 2.0f);
        lua_settable(lua, -3);

        lua_setglobal(lua, "Navigation");
    }
}