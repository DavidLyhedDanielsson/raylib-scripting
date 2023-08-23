#include "world.hpp"
// #include "raylib.h"
// #include "raymath.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <entt/entity/utility.hpp>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <random>

#include <assets.hpp>
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
#include <external/imgui.hpp>
#include <external/imguizmo.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register.hpp>
#include <lua_impl/lua_register_types.hpp>
#include <navigation.hpp>
#include <profiling.hpp>
#include <system/align_tiles.hpp>
#include <system/area_tracker.hpp>
#include <system/calculate_velocity.hpp>
#include <system/check_health.hpp>
#include <system/draw_renderables.hpp>
#include <system/max_range.hpp>
#include <system/move_entities.hpp>
#include <system/navigate.hpp>
#include <system/update_projectiles.hpp>

struct WorldData
{
    entt::registry* registry;
    lua_State* lua;
    int behaviourTable;

    bool drawNavigationTiles = false;
    std::optional<int32_t> drawNavigationField;
    Navigation navigation;
} world;

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

namespace World
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
                world.registry->view<Component::Walkable, Component::Transform>().each(
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

                world.navigation = Navigation(
                    {min.x, min.z},
                    {max.x, max.z},
                    min.x,
                    min.z,
                    tileSize == 0.0f ? 1.0f : tileSize);

                world.registry->view<Component::Walkable, Component::Render, Component::Transform>()
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

                        if(auto goal = world.registry->try_get<Component::EnemyGoal>(entity); goal)
                        {
                            for(auto id : goal->ids)
                                world.navigation.SetGoal(id, min, max);
                        }
                        else if(auto spawn = world.registry->try_get<Component::EnemySpawn>(entity);
                                spawn)
                        {
                            world.navigation.SetSpawn(spawn->id, spawn->goalId, min, max);
                        }
                        else
                        {
                            world.navigation.SetWalkable(min, max);
                        }
                    });

                world.registry->view<Component::NavGate, Component::Render, Component::Transform>()
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
                            world.navigation.SetNavGate(navGate.allowedGoalIds[i], min, max);
                    });

                world.registry
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
                            world.navigation.SetGoal(id, min, max);
                    });

                world.registry
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

                        world.navigation.SetSpawn(spawn.id, spawn.goalId, min, max);
                    });

                lua_getglobal(lua, "Navigation");
                lua_pushstring(lua, "sizeX");
                lua_pushinteger(lua, world.navigation.GetSizeX());
                lua_settable(lua, -3);
                lua_pushstring(lua, "sizeY");
                lua_pushinteger(lua, world.navigation.GetSizeY());
                lua_settable(lua, -3);
                lua_pushstring(lua, "tileSize");
                lua_pushnumber(lua, world.navigation.tileSize);
                lua_settable(lua, -3);
                lua_pushstring(lua, "offsetX");
                lua_pushnumber(lua, world.navigation.offsetX);
                lua_settable(lua, -3);
                lua_pushstring(lua, "offsetY");
                lua_pushnumber(lua, world.navigation.offsetY);
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
                world.navigation.ForEachTile([&](uint32_t x, uint32_t y, Navigation::Tile tile) {
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
                return world.navigation.GetTileSpace(position);
            });

        LuaRegister::PushRegister(
            lua,
            "ConvertToTileSpace",
            +[](lua_State* lua, Vector2 min, Vector2 max) {
                return world.navigation.ConvertToTileSpace(min, max);
            });

        // TODO: These below here can be quick registered?
        LuaRegister::PushRegister(
            lua,
            "IsReachable",
            +[](lua_State* lua, int x, int y) { return world.navigation.IsReachable(x, y); });

        LuaRegister::PushRegister(
            lua,
            "IsWalkable",
            +[](lua_State* lua, int x, int y) { return world.navigation.IsWalkable(x, y); });

        LuaRegister::PushRegister(
            lua,
            "IsNavGate",
            +[](lua_State* lua, int x, int y) { return world.navigation.IsNavGate(x, y); });

        LuaRegister::PushRegister(
            lua,
            "IsPassable",
            +[](lua_State* lua, int spawnId, int goalId, int x, int y) {
                return world.navigation.IsPassable(spawnId, goalId, x, y);
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

                world.navigation.SetVectorField(fieldId, std::move(vectorField));
            });

        LuaRegister::PushRegister(
            lua,
            "SetWall",
            +[](lua_State* lua, int x, int y, int side) {
                if(x < 0)
                    return;
                if(y < 0)
                    return;

                world.navigation.SetWall(x, y, (Navigation::Tile::Side)side);
            });

        lua_pushstring(lua, "drawTiles");
        lua_pushboolean(lua, world.drawNavigationTiles);
        lua_settable(lua, -3);
        lua_pushstring(lua, "drawField");
        if(world.drawNavigationField)
            lua_pushinteger(lua, world.drawNavigationField.value());
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

    void Init(entt::registry* registry, lua_State* lua)
    {
        registry->on_construct<Component::Behaviour>()
            .connect<[](entt::registry& registry, entt::entity entity) {
                Component::Behaviour behaviour = registry.get<Component::Behaviour>(entity);

                if(behaviour.script == "")
                    return;

                auto filePath = BehaviourFilePath(behaviour.script.c_str());
                {
                    std::ifstream in(filePath.data());
                    if(!in.is_open())
                    {
                        std::cerr << "Behaviour file " << behaviour.script << " doesn't exist"
                                  << std::endl;
                        return;
                    }
                }

                Profiling::ProfileCall("Load behaviour", [&]() {
                    auto res = luaL_dofile(world.lua, filePath.data());
                    if(res != LUA_OK)
                    {
                        std::cerr << "Couldn't load " << filePath.data() << " or error occurred"
                                  << std::endl;
                        std::cerr << lua_tostring(world.lua, -1) << std::endl;
                    }
                    else
                    {
                        if(!lua_isfunction(world.lua, -1))
                        {
                            std::cerr << "Behaviour file does not return a function";
                            return;
                        }

                        lua_rawgeti(world.lua, LUA_REGISTRYINDEX, world.behaviourTable);
                        lua_rotate(world.lua, -2, 1);
                        lua_setfield(world.lua, -2, filePath.data());
                        lua_pop(world.lua, 1);
                    }
                });
            }>();
        registry->on_destroy<Component::Behaviour>()
            .connect<[](entt::registry& registry, entt::entity entity) {
                Component::Behaviour behaviour = registry.get<Component::Behaviour>(entity);

                if(behaviour.script == "")
                    return;

                auto filePath = BehaviourFilePath(behaviour.script.c_str());
                lua_rawgeti(world.lua, LUA_REGISTRYINDEX, world.behaviourTable);
                lua_pushnil(world.lua);
                lua_setfield(world.lua, -2, filePath.data());
                lua_pop(world.lua, 1);
            }>();
        world.registry = registry;
        world.lua = lua;

        lua_createtable(lua, 0, 0);
        world.behaviourTable = luaL_ref(lua, LUA_REGISTRYINDEX);
    }

    void Update()
    {
        float time = 1.0f / 60.0f;
        auto lua = world.lua;

        // Run any global scripts, aka behaviour scripts
        lua_rawgeti(lua, LUA_REGISTRYINDEX, world.behaviourTable);
        auto table = lua_gettop(lua);

        lua_pushnil(lua);
        while(lua_next(lua, table) != 0)
        {
            if(lua_pcall(lua, 0, 0, 0) != LUA_OK)
            {
                std::cerr << "Error executing behaviour script: " << lua_tostring(lua, -1)
                          << std::endl;
            }
        }

        // These variables are tweakable from imgui for easy experimentation with navigation
        lua_getglobal(lua, "Navigation");
        lua_getfield(lua, -1, "ksi");
        const float ksi = (float)lua_tonumber(lua, -1);
        lua_pop(lua, 1);
        lua_getfield(lua, -1, "avoidanceLookAhead");
        const float avoidanceT = (float)lua_tonumber(lua, -1);
        lua_pop(lua, 1);
        lua_getfield(lua, -1, "obstacleLookAhead");
        const float obstacleT = (float)lua_tonumber(lua, -1);
        lua_pop(lua, 2);

        PROFILE_CALL(
            System::Navigate,
            *world.registry,
            world.navigation,
            ksi,
            avoidanceT,
            obstacleT,
            time);
        PROFILE_CALL(System::CalculateVelocity, *world.registry, time);
        PROFILE_CALL(System::MoveEntities, *world.registry, time);
        PROFILE_CALL(System::AlignTiles, *world.registry);
        PROFILE_CALL(System::UpdateProjectiles, *world.registry);
        PROFILE_CALL(System::MaxRange, *world.registry);
        PROFILE_CALL(System::CheckHealth, *world.registry);
        PROFILE_CALL(System::UpdateAreaTrackers, *world.registry);
    }

    void Draw()
    {
        PROFILE_CALL(System::DrawRenderable, *world.registry);

        // Debugging visualisation
        // world.registry->view<Component::Transform, Component::AreaTracker>().each(
        //     [](entt::entity entity,
        //        Component::Transform transform,
        //        Component::AreaTracker tracker) {
        //         auto boundingBox = tracker.GetBoundingBox(transform);
        //         Vector3 center = Vector3Scale(Vector3Add(boundingBox.min, boundingBox.max),
        //         0.5f); DrawCubeV(
        //             center,
        //             Vector3Subtract(boundingBox.max, boundingBox.min),
        //             {255, 0, 0, 80});
        //     });

        // Some debugging visualisations that are controllable from imgui
        auto lua = world.lua;
        lua_getglobal(lua, "Navigation");
        lua_getfield(lua, -1, "drawTiles");
        world.drawNavigationTiles = lua_toboolean(lua, -1);
        lua_pop(lua, 2);
        if(world.drawNavigationTiles)
            world.navigation.DrawTiles();

        lua_getglobal(lua, "Navigation");
        if(lua_getfield(lua, -1, "drawField") == LUA_TNUMBER)
            world.drawNavigationField = (int)lua_tointeger(lua, -1);
        else
            world.drawNavigationField = std::nullopt;
        lua_pop(lua, 2);
        if(world.drawNavigationField)
            world.navigation.DrawField(world.drawNavigationField.value());
    }
}