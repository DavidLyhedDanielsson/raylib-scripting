#include "world.hpp"
#include "entity/enemy_goal.hpp"
#include "entity/enemy_spawn.hpp"
#include "entity/walkable.hpp"
#include "navigation.hpp"
#include "profiling.hpp"
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <array>
#include <assets.hpp>
#include <cmath>
#include <cstdint>
#include <entity/acceleration.hpp>
#include <entity/area_tracker.hpp>
#include <entity/behaviour.hpp>
#include <entity/camera.hpp>
#include <entity/health.hpp>
#include <entity/max_range.hpp>
#include <entity/move_towards.hpp>
#include <entity/obstacle.hpp>
#include <entity/projectile.hpp>
#include <entity/render.hpp>
#include <entity/tile.hpp>
#include <entity/transform.hpp>
#include <entity/velocity.hpp>
#include <entt/entity/utility.hpp>
#include <external/imgui.hpp>
#include <external/imguizmo.hpp>
#include <external/raylib.hpp>
#include <fstream>
#include <iostream>
#include <limits>
#include <lua_impl/lua_register.hpp>
#include <lua_impl/lua_register_types.hpp>
#include <optional>
#include <random>

struct WorldData
{
    entt::registry* registry;
    lua_State* lua;
    int behaviourTable;
} world;

float RoundToMultiple(float number, float multiple)
{
    return std::round(number / multiple) * multiple;
}

// Move these
bool drawNavigation = false;
Navigation navigation;

std::random_device rd;
std::mt19937 mt(rd());
// This range is awctually [-1, 1), but that's fine
std::uniform_real_distribution<float> randomNumber(-1.0, 1.0f);

// const float k = 1.5;
// const float m = 2.0;
// const float t0 = 3;
std::optional<float> TimeToCollisionSphere(
    const Vector2 position,
    const Vector2 otherPosition,
    const Vector2 velocity,
    const Vector2 otherVelocity,
    const float radius)
{
    float totalRadius = radius + radius;
    float radiusSquared = totalRadius * totalRadius;

    float distanceSquared = Vector2DistanceSqr(position, otherPosition);
    if(distanceSquared < radiusSquared)
        return 0.0f;

    Vector2 relPos = Vector2Subtract(otherPosition, position);
    Vector2 relVel = Vector2Subtract(velocity, otherVelocity);

    float a = Vector2DotProduct(relVel, relVel);
    float b = Vector2DotProduct(relPos, relVel);
    float c = Vector2DotProduct(relPos, relPos) - radiusSquared;

    float disc = b * b - a * c;
    if(disc < 0.0f || a <= 0.0f)
        return std::nullopt;

    float t = (b - std::sqrt(disc)) / a;

    if(t <= 0.0f)
        return std::nullopt;

    return t;
}

Vector2 linePointDistance(Vector2 p0, Vector2 p1, Vector2 point)
{
    float a = Vector2DotProduct(Vector2Subtract(point, p0), Vector2Subtract(p1, p0));
    if(a <= 0.0f)
        return p0;

    float b = Vector2DotProduct(Vector2Subtract(point, p1), Vector2Subtract(p0, p1));
    if(b <= 0.0f)
        return p1;

    return Vector2Add(p0, Vector2Scale(Vector2Subtract(p1, p0), a / (a + b)));
}

// https://ericleong.me/research/circle-line/
std::optional<Vector2> lineLineIntersection(
    Vector2 start0,
    Vector2 end0,
    Vector2 start1,
    Vector2 end1,
    bool& isOn0,
    bool& isOn1)
{
    float a1 = end0.y - start0.y;
    float b1 = start0.x - end0.x;
    float c1 = a1 * start0.x + b1 * start0.y;

    float a2 = end1.y - start1.y;
    float b2 = start1.x - end0.x;
    float c2 = a2 * start1.x + b2 * start1.y;

    float det = a1 * b2 - a2 * b1;
    if(std::abs(det) < 0.0000000000001f)
        return std::nullopt;

    float x = (b2 * c1 - b1 * c2) / det;
    float y = (a1 * c2 - a2 * c1) / det;

    float d0 = x - std::min(start0.x, end0.x);
    float d1 = std::max(start0.x, end0.x) - x;
    float d2 = y - std::min(start0.y, end0.y);
    float d3 = std::max(start0.y, end0.y) - y;

    const float epsilon = -7.0e-4f;
    if(d0 >= epsilon && d1 >= epsilon && d2 >= epsilon && d3 >= epsilon)
        isOn0 = true;

    float d4 = x - std::min(start1.x, end1.x);
    float d5 = std::max(start1.x, end1.x) - x;
    float d6 = y - std::min(start1.y, end1.y);
    float d7 = std::max(start1.y, end1.y) - y;

    if(d4 >= epsilon && d5 >= epsilon && d6 >= epsilon && d7 >= epsilon)
        isOn1 = true;

    return {{
        .x = x,
        .y = y,
    }};
}

Vector2 closestPointLine(Vector2 start, Vector2 end, Vector2 p, bool& isOnLine)
{
    float a1 = end.y - start.y;
    float b1 = start.x - end.x;
    float c1 = a1 * start.x + b1 * start.y;
    float c2 = -b1 * p.x + a1 * p.y;
    float det = a1 * a1 + b1 * b1;
    if(std::abs(det) > 0.0000000000001f)
    {
        Vector2 point = {
            .x = (a1 * c1 - b1 * c2) / det,
            .y = (a1 * c2 + b1 * c1) / det,
        };

        float dist = Vector2Distance(start, end) - Vector2Distance(start, point)
                     - Vector2Distance(point, end);

        isOnLine = std::abs(dist) < 0.00001f;

        return point;
    }
    else
    {
        isOnLine = true;
        return {.x = p.x, .y = p.y};
    }
}

Vector2 closestPointLine(Vector2 start, Vector2 end, Vector2 p)
{
    bool _ = false;
    return closestPointLine(start, end, p, _);
}

// TODO: Undo changes to this and just use Eric's formulas?
std::optional<float> TimeToCollisionCircleLine(
    const Vector2 circlePosition,
    const Vector2 velocity,
    const float radius,
    const Vector2 lineStart,
    const Vector2 lineEnd)
{
    Vector2 goalPosition = Vector2Add(circlePosition, velocity);

    auto GetLineTTC = [&](Vector2 a) {
        Vector2 ac = Vector2DirectionTo(a, circlePosition);
        Vector2 p1c = Vector2DirectionTo(lineEnd, circlePosition);
        Vector2 p = Vector2Subtract(
            a,
            Vector2Scale(
                Vector2Normalize(velocity),
                radius * (Vector2Dot(ac, ac) / Vector2Dot(p1c, p1c))));

        float speed = Vector2Length(velocity);
        return Vector2Distance(circlePosition, p) / speed;
    };

    auto GetEndpointTTC = [&](Vector2 endpoint) {
        Vector2 closestPoint = closestPointLine(circlePosition, goalPosition, endpoint);

        float distance = Vector2Distance(endpoint, closestPoint);
        float intersectionDepth = std::sqrt(radius * radius - distance * distance);

        float distanceToClosestPoint = Vector2Length(Vector2Subtract(closestPoint, circlePosition));

        Vector2 p = Vector2Add(
            circlePosition,
            Vector2Scale(
                Vector2DirectionTo(circlePosition, closestPoint),
                distanceToClosestPoint - intersectionDepth));

        float speed = Vector2Length(velocity);
        return Vector2Distance(circlePosition, p) / speed;
    };

    bool isOnVelocity = false;
    bool isOnObstacle = false;

    std::optional<Vector2> aOpt = lineLineIntersection(
        circlePosition,
        goalPosition,
        lineStart,
        lineEnd,
        isOnVelocity,
        isOnObstacle);

    if(!aOpt.has_value())
        return std::nullopt;

    Vector2 a = aOpt.value();
    if(aOpt.has_value() && isOnObstacle)
    {
        // Already colliding
        if(isOnVelocity)
            return 0.0f;
        else
        {
            bool isOnLine = false;
            Vector2 b = closestPointLine(lineStart, lineEnd, goalPosition, isOnLine);
            if(isOnLine && Vector2Dot(Vector2DirectionTo(circlePosition, b), velocity) > 0.0f)
            {
                // Colliding with the obstacle "line"
                float ttc = GetLineTTC(a);
                if(ttc > 0.0f)
                    return ttc;
            }
        }
    }

    if(Vector2Dot(Vector2DirectionTo(circlePosition, lineStart), velocity) > 0.0f)
    {
        Vector2 c = closestPointLine(circlePosition, goalPosition, lineStart);
        if(Vector2Distance(c, lineStart) <= radius)
        {
            // Colliding with obstacle end-point
            float ttc = GetEndpointTTC(lineStart);
            if(ttc > 0.0f)
                return ttc;
        }
    }

    if(Vector2Dot(Vector2DirectionTo(circlePosition, lineEnd), velocity) > 0.0f)
    {
        Vector2 d = closestPointLine(circlePosition, goalPosition, lineEnd);
        if(Vector2Distance(d, lineEnd) <= radius)
        {
            // Colliding with obstacle end-point
            float ttc = GetEndpointTTC(lineEnd);
            if(ttc > 0.0f)
                return ttc;
        }
    }

    return std::nullopt;
}

bool buildNavigation = false;

namespace LuaRegister
{
    template<>
    constexpr auto LuaGetFunc<Navigation::Tile> = [](lua_State* lua, int i) {
        assert(false && "Not yet");
    };

    template<>
    constexpr auto LuaSetFunc<Navigation::Tile> = [](lua_State* lua, Navigation::Tile tile) {
        lua_createtable(lua, 0, 0);
        lua_pushstring(lua, "type");
        lua_pushinteger(lua, (int)tile.type);
        lua_settable(lua, -3);
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

                navigation = Navigation(
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

                        if(world.registry->try_get<Component::EnemyGoal>(entity))
                        {
                            navigation.SetGoal(min, max);
                        }
                        else if(world.registry->try_get<Component::EnemySpawn>(entity))
                        {
                            navigation.SetSpawn(min, max);
                        }
                        else
                        {
                            navigation.SetWalkable(min, max);
                        }
                    });

                world.registry->view<Component::Obstacle, Component::Render, Component::Transform>()
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

                        navigation.SetObstacle(min, max);
                    });

                world.registry
                    ->view<Component::EnemyGoal, Component::Render, Component::Transform>()
                    .each([&](entt::entity entity,
                              Component::EnemyGoal _,
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

                        navigation.SetGoal(min, max);
                    });

                world.registry
                    ->view<Component::EnemySpawn, Component::Render, Component::Transform>()
                    .each([&](entt::entity entity,
                              Component::EnemySpawn _,
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

                        navigation.SetSpawn(min, max);
                    });

                lua_getglobal(lua, "Navigation");
                lua_pushstring(lua, "sizeX");
                lua_pushinteger(lua, navigation.sizeX);
                lua_settable(lua, -3);
                lua_pushstring(lua, "sizeY");
                lua_pushinteger(lua, navigation.sizeY);
                lua_settable(lua, -3);
                lua_pushstring(lua, "tileSize");
                lua_pushnumber(lua, navigation.tileSize);
                lua_settable(lua, -3);
                lua_pushstring(lua, "offsetX");
                lua_pushnumber(lua, navigation.offsetX);
                lua_settable(lua, -3);
                lua_pushstring(lua, "offsetY");
                lua_pushnumber(lua, navigation.offsetY);
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
                navigation.ForEachTile([&](uint32_t x, uint32_t y, Navigation::Tile tile) {
                    if(error)
                        return;

                    lua_pushvalue(lua, -1);

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
            +[](lua_State* lua, Vector2 position) { return navigation.GetTileSpace(position); });

        LuaRegister::PushRegister(
            lua,
            "ConvertToTileSpace",
            +[](lua_State* lua, Vector2 min, Vector2 max) {
                return navigation.ConvertToTileSpace(min, max);
            });

        LuaRegister::PushRegister(
            lua,
            "Reachable",
            +[](lua_State* lua, int x, int y) { return navigation.Reachable(x, y); });

        LuaRegister::PushRegister(
            lua,
            "Walkable",
            +[](lua_State* lua, int x, int y) { return navigation.Walkable(x, y); });

        LuaRegister::PushRegister(
            lua,
            "SetVectorField",
            +[](lua_State* lua, LuaRegister::Placeholder table) {
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

                navigation.vectorField = std::move(vectorField);
            });

        LuaRegister::PushRegister(
            lua,
            "SetWall",
            +[](lua_State* lua, int x, int y, int side) {
                if(x < 0)
                    return;
                if(y < 0)
                    return;

                navigation.SetWall(x, y, (Navigation::Tile::Side)side);
            });

        lua_pushstring(lua, "draw");
        lua_pushboolean(lua, drawNavigation);
        lua_settable(lua, -3);
        lua_pushstring(lua, "ksi");
        lua_pushnumber(lua, 10.0f);
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

        static std::vector<std::string> entityNames;
        entityNames.clear();

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

        Profiling::ProfileCall("MoveEntities", [&]() {
            for(auto [entity, transform, moveTowards, velocityComponent, acceleration] :
                world.registry
                    ->view<
                        Component::Transform,
                        Component::MoveTowards,
                        Component::Velocity,
                        Component::Acceleration>()
                    .each())
            {
                entityNames.push_back(std::to_string((int)entity));
                PROFILE_SCOPE(entityNames.back().c_str());

                const float radius = 0.30f;

                if(navigation.IsGoal(transform.position))
                {
                    if(auto health = world.registry->try_get<Component::Health>(entity); health)
                        health->currentHealth = 0.0f;
                    continue;
                }

                Vector2 force =
                    navigation.GetForce({.x = transform.position.x, .y = transform.position.z});

                Vector3 movementDirection = {force.x, 0.0f, force.y};

                float speed = moveTowards.speed;
                Vector3 goalVelocity = Vector3Scale(movementDirection, speed);

                const Vector2 velocity = {.x = velocityComponent.x, .y = velocityComponent.z};
                Vector2 forces =
                    Vector2Scale(Vector2Subtract({goalVelocity.x, goalVelocity.z}, velocity), ksi);

                forces =
                    Vector2Add(forces, Vector2Scale({randomNumber(mt), randomNumber(mt)}, 0.5f));

                Profiling::ProfileCall("EntityAvoidance", [&]() {
                    for(auto [otherEntity, otherTransform, otherHealth] :
                        world.registry->view<Component::Transform, Component::Health>().each())
                    {
                        if(entity == otherEntity)
                            continue;

                        float distance =
                            Vector3Distance(transform.position, otherTransform.position);

                        if(distance > 3.0f)
                            continue;

                        Vector2 otherVelocity = Vector2Zero();
                        if(Component::Velocity* oVel =
                               world.registry->try_get<Component::Velocity>(otherEntity);
                           oVel)
                        {
                            otherVelocity = {.x = oVel->ToVector3().x, .y = oVel->ToVector3().z};
                        };

                        Vector2 position = {transform.position.x, transform.position.z};
                        Vector2 otherPosition = {
                            otherTransform.position.x,
                            otherTransform.position.z};

                        std::optional<float> timeToCollision = TimeToCollisionSphere(
                            position,
                            otherPosition,
                            velocity,
                            otherVelocity,
                            radius);

                        if(!timeToCollision.has_value())
                            continue;

                        float t = timeToCollision.value();

                        if(t == 0.0f)
                        {
                            Vector2 avoidanceForce = Vector2DirectionTo(otherPosition, position);
                            float magnitude = Vector2Length(velocity);

                            if(magnitude > 10.0f)
                                magnitude = 10.0f;

                            forces.x += avoidanceForce.x * magnitude;
                            forces.y += avoidanceForce.y * magnitude;
                        }
                        else
                        {
                            Vector2 avoidanceForce = Vector2Normalize(Vector2Subtract(
                                Vector2Add(position, Vector2Scale(velocity, t)),
                                Vector2Add(otherPosition, Vector2Scale(otherVelocity, t))));

                            float magnitude = 0.0f;
                            if(t >= 0.0f && t < avoidanceT)
                                magnitude = (avoidanceT - t) / (t + 0.001f);

                            if(magnitude > 20.0f)
                                magnitude = 20.0f;

                            forces.x += avoidanceForce.x * magnitude;
                            forces.y += avoidanceForce.y * magnitude;
                        }
                    }
                });

                Profiling::ProfileCall("ObstacleAvoidance", [&]() {
                    Vector2 position = {.x = transform.position.x, .y = transform.position.z};
                    float currentSpeed = Vector2Length(velocity);
                    navigation.ForArea(
                        {
                            .x = transform.position.x - currentSpeed * obstacleT,
                            .y = transform.position.z - currentSpeed * obstacleT,
                        },
                        {
                            .x = transform.position.x + currentSpeed * obstacleT,
                            .y = transform.position.z + currentSpeed * obstacleT,
                        },
                        [&](Navigation::Tile tile, uint32_t x, uint32_t y) {
                            tile.forEachWall([&](Navigation::Tile::Side side) {
                                Navigation::Wall wall = navigation.GetWall(x, y, side);

                                std::optional<float> timeToCollisionOpt = TimeToCollisionCircleLine(
                                    position,
                                    velocity,
                                    radius,
                                    wall.start,
                                    wall.end);
                                if(!timeToCollisionOpt || *timeToCollisionOpt > obstacleT)
                                    return;

                                float timeToCollision = timeToCollisionOpt.value();

                                Vector2 avoidanceForce = Vector2Scale(
                                    wall.normal,
                                    Vector2Dot(forces, wall.normal)
                                        / Vector2Dot(wall.normal, wall.normal));

                                if(Vector2Dot(avoidanceForce, wall.normal) < 0.0f)
                                    avoidanceForce = Vector2Negate(avoidanceForce);

                                float magnitude = 0.0f;
                                if(timeToCollision >= 0.0f && timeToCollision < obstacleT)
                                    magnitude =
                                        (obstacleT - timeToCollision) / (timeToCollision + 0.001f);

                                if(magnitude > 40.0f)
                                    magnitude = 40.0f;

                                forces.x += avoidanceForce.x * magnitude;
                                forces.y += avoidanceForce.y * magnitude;
                            });
                        });
                });

                acceleration.acceleration.x += forces.x * time;
                acceleration.acceleration.z += forces.y * time;
            }
        });

        Profiling::ProfileCall("CalculateVelocity", [&]() {
            for(auto [entity, velocity, acceleration] :
                world.registry->view<Component::Velocity, Component::Acceleration>().each())
            {
                float length = Vector3Length(acceleration.acceleration);
                if(length > 20.0f * time)
                {
                    acceleration.acceleration =
                        Vector3Scale(Vector3Normalize(acceleration.acceleration), 20.0f * time);
                }

                assert(!std::isnan(acceleration.acceleration.x));
                assert(!std::isnan(acceleration.acceleration.y));
                assert(!std::isnan(acceleration.acceleration.z));

                velocity.x += acceleration.acceleration.x;
                velocity.y += acceleration.acceleration.y;
                velocity.z += acceleration.acceleration.z;

                acceleration.acceleration = {0.0f, 0.0f, 0.0f};
            }
        });

        Profiling::ProfileCall("MoveEntities", [&]() {
            for(auto [entity, transform, velocity] :
                world.registry->view<Component::Transform, Component::Velocity>().each())
            {
                transform.position.x += velocity.x * time;
                transform.position.y += velocity.y * time;
                transform.position.z += velocity.z * time;
            }
        });

        Profiling::ProfileCall("Tile", [&]() {
            for(auto [entity, transform] :
                world.registry->view<Component::Transform, Component::Tile>().each())
            {
                transform.position.x = std::roundf(transform.position.x);
                transform.position.y = std::roundf(transform.position.y);
                transform.position.z = std::roundf(transform.position.z);

                transform.rotation.x =
                    RoundToMultiple(transform.rotation.x * RAD2DEG, 90.0f) * DEG2RAD;
                transform.rotation.y =
                    RoundToMultiple(transform.rotation.y * RAD2DEG, 90.0f) * DEG2RAD;
                transform.rotation.z =
                    RoundToMultiple(transform.rotation.z * RAD2DEG, 90.0f) * DEG2RAD;
            }
        });

        Profiling::ProfileCall("Projectiles", [&]() {
            for(auto [projectileEntity, projectileRender, projectileTransform, projectile] :
                world.registry
                    ->view<Component::Render, Component::Transform, Component::Projectile>()
                    .each())
            {
                bool destroy = false;

                auto projectileHitBox = BoundingBoxTransform(
                    projectileRender.boundingBox,
                    projectileTransform.position);

                // Entity = entity (potentially) affected by projectile
                for(auto [entity, entityRender, entityTransform, entityHealth] :
                    world.registry
                        ->view<Component::Render, Component::Transform, Component::Health>()
                        .each())
                {
                    auto entityHitBox =
                        BoundingBoxTransform(entityRender.boundingBox, entityTransform.position);

                    if(CheckCollisionBoxes(projectileHitBox, entityHitBox))
                    {
                        // Entity will be destroyed in a different system
                        entityHealth.currentHealth -= projectile.damage;
                        destroy = true;
                    }
                }

                if(destroy)
                    world.registry->destroy(projectileEntity);
            }
        });

        // From the docs:
        // "Deleting the current entity or removing its components is allowed during iterations"
        for(auto [entity, transform, maxRange] :
            world.registry->view<Component::Transform, Component::MaxRange>().each())
        {
            if(Vector3Distance(transform.position, maxRange.distanceFrom) >= maxRange.maxDistance)
                world.registry->destroy(entity);
        }

        for(auto [entity, health] : world.registry->view<Component::Health>().each())
        {
            if(health.currentHealth <= 0.0001f)
                world.registry->destroy(entity);
        }

        Profiling::ProfileCall("AreaTracker", [&]() {
            for(auto [trackerEntity, trackerTransform, tracker] :
                world.registry->view<Component::Transform, Component::AreaTracker>().each())
            {
                tracker.entitiesInside.clear();

                auto trackerHitBox = tracker.GetBoundingBox(trackerTransform);

                for(auto [entity, entityRender, entityTransform, entityHealth] :
                    world.registry
                        ->view<Component::Render, Component::Transform, Component::Health>()
                        .each())
                {
                    auto entityHitBox =
                        BoundingBoxTransform(entityRender.boundingBox, entityTransform.position);

                    if(CheckCollisionBoxes(trackerHitBox, entityHitBox))
                        tracker.entitiesInside.push_back(entity);
                }
            }
        });
    }

    void Draw()
    {
        auto lua = world.lua;

        auto group = world.registry->group<Component::Render, Component::Transform>();
        for(auto entity : group)
        {
            auto [render, transform] = group.get<Component::Render, Component::Transform>(entity);

            render.model.transform = MatrixMultiply(
                MatrixRotateZYX(transform.rotation),
                MatrixTranslate(transform.position.x, transform.position.y, transform.position.z));
            DrawModel(render.model, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE);

            render.model.transform = MatrixIdentity();
        }

        world.registry->view<Component::Transform, Component::AreaTracker>().each(
            [](entt::entity entity,
               Component::Transform transform,
               Component::AreaTracker tracker) {
                auto boundingBox = tracker.GetBoundingBox(transform);
                Vector3 center = Vector3Scale(Vector3Add(boundingBox.min, boundingBox.max), 0.5f);
                DrawCubeV(
                    center,
                    Vector3Subtract(boundingBox.max, boundingBox.min),
                    {255, 0, 0, 80});
            });

        lua_getglobal(lua, "Navigation");
        lua_getfield(lua, -1, "draw");
        drawNavigation = lua_toboolean(lua, -1);
        lua_pop(lua, 2);
        if(drawNavigation)
            navigation.Draw();
    }

    void DrawImgui()
    {
        // Yay this is empty now!
    }
}