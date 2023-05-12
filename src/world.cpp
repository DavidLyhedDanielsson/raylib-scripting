#include "world.hpp"
#include "entity/enemy_goal.hpp"
#include "entity/walkable.hpp"
#include "navigation.hpp"
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <array>
#include <assets.hpp>
#include <cmath>
#include <entity/acceleration.hpp>
#include <entity/area_tracker.hpp>
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
#include <iostream>
#include <limits>
#include <lua_impl/lua_register.hpp>
#include <lua_impl/lua_register_types.hpp>
#include <optional>
#include <random>

struct WorldData
{
    entt::registry* registry;
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

const float k = 1.5;
const float m = 2.0;
const float t0 = 3;
std::optional<Vector2> CollisionCoefficient(
    const Vector2 position,
    const Vector2 otherPosition,
    const Vector2 velocity,
    const Vector2 otherVelocity,
    const float radius,
    float* outTime = nullptr)
{
    float totalRadius = radius + radius;
    float radiusSquared = totalRadius * totalRadius;

    float distanceSquared = Vector2DistanceSqr(position, otherPosition);

    if(outTime && distanceSquared < radiusSquared)
    {
        *outTime = 0.0f;
        radiusSquared = std::pow(totalRadius - sqrtf(distanceSquared), 2.0f);
    }

    Vector2 relPos = Vector2Subtract(otherPosition, position);
    Vector2 relVel = Vector2Subtract(velocity, otherVelocity);

    float a = Vector2DotProduct(relVel, relVel);
    float b = Vector2DotProduct(relPos, relVel);
    float c = Vector2DotProduct(relPos, relPos) - radiusSquared;

    float disc = b * b - a * c;
    if(disc < 0.0f || std::abs(disc) < 0.00001f)
        return std::nullopt;

    disc = std::sqrt(disc);
    float t = (b - disc) / a;

    if(t < 0.0f)
        return std::nullopt;

    if(outTime)
    {
        *outTime = t;
        return std::nullopt;
    }

    const float c0 = -k * std::exp(-t / t0);
    const Vector2 c1 = Vector2Subtract(
        relVel,
        Vector2Scale(
            Vector2Subtract(Vector2Scale(relVel, b), Vector2Scale(relPos, a)),
            1.0f / disc));
    const float c2 = a * std::pow(t, m) * (m / t + 1.0f / t0);

    const Vector2 d = Vector2Scale(Vector2Scale(c1, c0), 1.0f / c2);

    return d;
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

                world.registry->view<Component::Walkable, Component::Transform>().each(
                    [&](Component::Transform transform) {
                        min = Vector3Min(
                            min,
                            {.x = transform.position.x, .y = 0.0f, .z = transform.position.z});
                        max = Vector3Max(
                            max,
                            {.x = transform.position.x, .y = 0.0f, .z = transform.position.z});
                    });

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
                        auto bounds =
                            GetModelBoundingBox(render.model, MatrixRotateZYX(transform.rotation));
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
                        else
                        {
                            navigation.SetWalkable(min, max);
                        }
                    });

                world.registry
                    ->view<Component::EnemyGoal, Component::Render, Component::Transform>()
                    .each([&](entt::entity entity,
                              Component::EnemyGoal _,
                              Component::Render render,
                              Component::Transform transform) {
                        auto bounds =
                            GetModelBoundingBox(render.model, MatrixRotateZYX(transform.rotation));
                        Vector2 minBounds = {.x = bounds.min.x, .y = bounds.min.z};
                        Vector2 maxBounds = {.x = bounds.max.x, .y = bounds.max.z};

                        Vector2 min =
                            Vector2Add({transform.position.x, transform.position.z}, minBounds);
                        Vector2 max =
                            Vector2Add({transform.position.x, transform.position.z}, maxBounds);

                        navigation.SetGoal(min, max);
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

        lua_pushstring(lua, "draw");
        lua_pushboolean(lua, drawNavigation);
        lua_settable(lua, -3);

        lua_setglobal(lua, "Navigation");
    }

    void Init(entt::registry* registry)
    {
        world.registry = registry;
    }

    // Don't look at this
    lua_State* lua;

    // TODO: `lua` should probably be in WorldData
    void Update(lua_State* luaS)
    {
        // Don't look at this
        lua = luaS;

        float time = 1.0f / 60.0f;

        for(auto [entity, transform, moveTowards, velocityComponent, acceleration] :
            world.registry
                ->view<
                    Component::Transform,
                    Component::MoveTowards,
                    Component::Velocity,
                    Component::Acceleration>()
                .each())
        {
            const float radius = 0.35f;

            float speed = moveTowards.speed;

            if(navigation.IsGoal(transform.position))
            {
                if(auto health = world.registry->try_get<Component::Health>(entity); health)
                    health->currentHealth = 0.0f;
                continue;
            }

            const Vector2 velocity = {.x = velocityComponent.x, .y = velocityComponent.z};

            // auto movementDirection =
            //     Vector3Normalize(Vector3Subtract(moveTowards.target, transform.position));

            Vector2 force = navigation.GetForce({transform.position.x, transform.position.z});
            Vector3 movementDirection = {force.x, 0.0f, force.y};

            Vector3 goalVelocity = Vector3Scale(movementDirection, speed);

            const float ksi = 0.54f;
            Vector2 forces = Vector2Scale(
                Vector2Subtract({goalVelocity.x, goalVelocity.z}, velocity),
                1.0f / ksi);

            forces = Vector2Add(forces, Vector2Scale({randomNumber(mt), randomNumber(mt)}, 0.5f));

            for(auto [otherEntity, otherTransform, otherHealth] :
                world.registry->view<Component::Transform, Component::Health>().each())
            {
                if(entity == otherEntity)
                    continue;

                if(Vector3Distance(transform.position, otherTransform.position) > 3.0f)
                    continue;

                Vector3 otherVelocity = Vector3Zero();
                if(Component::Velocity* oVel =
                       world.registry->try_get<Component::Velocity>(otherEntity);
                   oVel)
                {
                    otherVelocity = oVel->ToVector3();
                };

                std::optional<Vector2> coeff = CollisionCoefficient(
                    {transform.position.x, transform.position.z},
                    {otherTransform.position.x, otherTransform.position.z},
                    velocity,
                    {otherVelocity.x, otherVelocity.z},
                    radius);

                if(coeff.has_value())
                {
                    Vector2 avoidForce = coeff.value();
                    forces.x += avoidForce.x;
                    forces.y += avoidForce.y;
                }
            }

            for(auto [obstacle, oTransform, render] :
                world.registry->view<Component::Transform, Component::Render, Component::Obstacle>()
                    .each())
            {
                auto bbox = GetModelBoundingBox(
                    render.model,
                    MatrixMultiply(
                        MatrixRotateZYX(oTransform.rotation),
                        MatrixTranslate(oTransform.position)));

                Vector2 bboxMin = {.x = bbox.min.x, .y = bbox.min.z};
                Vector2 bboxMax = {.x = bbox.max.x, .y = bbox.max.z};
                Vector2 bboxSize = Vector2Subtract(bboxMax, bboxMin);
                Vector2 bboxCenter = Vector2Add(bboxMin, Vector2Scale(bboxSize, 0.5f));

                float bboxWidth = bboxMax.x - bboxMin.x;
                float bboxHeight = bboxMax.y - bboxMin.y;

                Vector2 p0;
                Vector2 p1;
                Vector2 normal; // It doesn't actually matter which direction this points
                if(bboxWidth > bboxHeight)
                {
                    p0 = {.x = bboxCenter.x - bboxWidth * 0.5f, .y = bboxCenter.y};
                    p1 = {.x = bboxCenter.x + bboxWidth * 0.5f, .y = bboxCenter.y};

                    float x = p1.x - p0.x;
                    float y = p1.y - p0.y;
                    normal = Vector2Normalize({.x = -y, .y = x});
                }
                else
                {
                    p0 = {.x = bboxCenter.x, .y = bboxCenter.y - bboxHeight * 0.5f};
                    p1 = {.x = bboxCenter.x, .y = bboxCenter.y + bboxHeight * 0.5f};

                    float x = p1.x - p0.x;
                    float y = p1.y - p0.y;
                    normal = Vector2Normalize({.x = -y, .y = x});
                }

                Vector2 position = {.x = transform.position.x, .y = transform.position.z};

                Vector2 nw = Vector2Subtract(linePointDistance(p0, p1, position), position);
                float dw = Vector2DotProduct(nw, nw);

                if(Vector2Dot(velocity, nw) < 0 || std::abs(dw - radius * radius) < 0.00001
                   || dw > 3.0f * 3.0f)
                {
                    continue;
                }

                enum CollisionType
                {
                    NONE,
                    DISC,
                    SEGMENT
                };

                struct
                {
                    float tmin = std::numeric_limits<float>::max();
                    CollisionType collisionType = NONE;
                    union
                    {
                        struct
                        {
                            float b;
                            float c;
                            float disc;
                            Vector2 w;
                        } discData;
                        struct
                        {
                            Vector2 o;
                            Vector2 wo;
                        } segmentData;
                    };
                } hitInfo;

                const float correctRadius = dw < radius * radius ? std::sqrt(dw) : radius;
                const float a = Vector2DotProduct(velocity, velocity);

                auto DiscCollision = [&](Vector2 point) {
                    Vector2 w = Vector2Subtract(point, position);
                    float b = Vector2DotProduct(w, velocity);
                    float c = Vector2DotProduct(w, w) - correctRadius * correctRadius;
                    float disc = b * b - a * c;
                    if(disc > 0.0f && std::abs(a) > 0.0001f)
                    {
                        disc = std::sqrt(disc);
                        float t = (b - disc) / a;
                        if(t > 0 && t < hitInfo.tmin)
                        {
                            hitInfo = {
                                .tmin = t,
                                .collisionType = CollisionType::DISC,
                                .discData = {
                                    .b = b,
                                    .c = c,
                                    .disc = disc,
                                    .w = w,
                                }};
                        }
                    }
                };

                DiscCollision(p0);
                DiscCollision(p1);

                auto SegmentCollision = [&](Vector2 p0, Vector2 p1) {
                    Vector2 o = Vector2Subtract(p1, p0);
                    float d = Vector2Det(velocity, o);
                    if(std::abs(d) > 0.0001f)
                    {
                        float invD = 1.0f / d;
                        float t = Vector2Det(o, Vector2Subtract(position, p0)) * invD;
                        float s = Vector2Det(velocity, Vector2Subtract(position, p0)) * invD;
                        if(t > 0.0f && s >= 0.0f && s <= 1.0f && t < hitInfo.tmin)
                        {
                            hitInfo = {
                                .tmin = t,
                                .collisionType = CollisionType::SEGMENT,
                                .segmentData = {
                                    .o = o,
                                    .wo = Vector2Subtract(position, p0),
                                }};
                        }
                    }
                };

                SegmentCollision(
                    Vector2Add(p0, Vector2Scale(normal, correctRadius)),
                    Vector2Add(p1, Vector2Scale(normal, correctRadius)));
                SegmentCollision(
                    Vector2Subtract(p0, Vector2Scale(normal, correctRadius)),
                    Vector2Subtract(p1, Vector2Scale(normal, correctRadius)));

                switch(hitInfo.collisionType)
                {
                    case CollisionType::DISC: {
                        const float c0 = -k * std::exp(-hitInfo.tmin / t0);
                        const Vector2 c1 = Vector2Subtract(
                            velocity,
                            Vector2Scale(
                                Vector2Subtract(
                                    Vector2Scale(velocity, hitInfo.discData.b),
                                    Vector2Scale(hitInfo.discData.w, a)),
                                1.0f / hitInfo.discData.disc));
                        const float c2 = a * std::pow(hitInfo.tmin, m);
                        const float c3 = (m / hitInfo.tmin + 1.0f / t0);
                        const Vector2 d =
                            Vector2Scale(Vector2Scale(Vector2Scale(c1, c0), 1.0f / c2), c3);

                        forces = Vector2Add(forces, d);
                        break;
                    }
                    case CollisionType::SEGMENT: {
                        const float c0 = k * std::exp(-hitInfo.tmin / t0);
                        const float c1 =
                            std::pow(hitInfo.tmin, m) * Vector2Det(velocity, hitInfo.segmentData.o);
                        const float c2 = (m / hitInfo.tmin + 1.0f / t0);

                        const Vector2 d = Vector2Scale(
                            {.x = -hitInfo.segmentData.o.y, .y = hitInfo.segmentData.o.x},
                            c0 / c1 * c2);
                        forces = Vector2Add(forces, d);
                        break;
                    }
                    case CollisionType::NONE: // Do nothing
                        break;
                }
            }

            acceleration.acceleration.x += forces.x * time;
            acceleration.acceleration.z += forces.y * time;
        }

        for(auto [entity, velocity, acceleration] :
            world.registry->view<Component::Velocity, Component::Acceleration>().each())
        {
            float length = Vector3Length(acceleration.acceleration);
            if(length > 20.0f * time)
            {
                acceleration.acceleration =
                    Vector3Scale(Vector3Normalize(acceleration.acceleration), 20.0f * time);
            }

            velocity.x += acceleration.acceleration.x;
            velocity.y += acceleration.acceleration.y;
            velocity.z += acceleration.acceleration.z;

            acceleration.acceleration = {0.0f, 0.0f, 0.0f};
        }

        for(auto [entity, transform, velocity] :
            world.registry->view<Component::Transform, Component::Velocity>().each())
        {
            transform.position.x += velocity.x * time;
            transform.position.y += velocity.y * time;
            transform.position.z += velocity.z * time;
        }

        for(auto [entity, transform] :
            world.registry->view<Component::Transform, Component::Tile>().each())
        {
            transform.position.x = std::roundf(transform.position.x);
            transform.position.y = std::roundf(transform.position.y);
            transform.position.z = std::roundf(transform.position.z);

            transform.rotation.x = RoundToMultiple(transform.rotation.x * RAD2DEG, 90.0f) * DEG2RAD;
            transform.rotation.y = RoundToMultiple(transform.rotation.y * RAD2DEG, 90.0f) * DEG2RAD;
            transform.rotation.z = RoundToMultiple(transform.rotation.z * RAD2DEG, 90.0f) * DEG2RAD;
        }

        for(auto [projectileEntity, projectileRender, projectileTransform, projectile] :
            world.registry->view<Component::Render, Component::Transform, Component::Projectile>()
                .each())
        {
            bool destroy = false;

            auto projectileHitBox =
                GetModelBoundingBox(projectileRender.model, projectileTransform.position);

            // Entity = entity (potentially) affected by projectile
            for(auto [entity, entityRender, entityTransform, entityHealth] :
                world.registry->view<Component::Render, Component::Transform, Component::Health>()
                    .each())
            {
                auto entityHitBox =
                    GetModelBoundingBox(entityRender.model, entityTransform.position);

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

        for(auto [trackerEntity, trackerTransform, tracker] :
            world.registry->view<Component::Transform, Component::AreaTracker>().each())
        {
            tracker.entitiesInside.clear();

            auto trackerHitBox = tracker.GetBoundingBox(trackerTransform);

            // TODO: Add BoundingBox component
            for(auto [entity, entityRender, entityTransform, entityHealth] :
                world.registry->view<Component::Render, Component::Transform, Component::Health>()
                    .each())
            {
                auto entityHitBox =
                    GetModelBoundingBox(entityRender.model, entityTransform.position);

                if(CheckCollisionBoxes(trackerHitBox, entityHitBox))
                    tracker.entitiesInside.push_back(entity);
            }
        }
    }

    void Draw()
    {
        DrawGrid(10, 1.0f);

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