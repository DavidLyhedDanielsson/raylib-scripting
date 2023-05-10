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
#include <entity/projectile.hpp>
#include <entity/render.hpp>
#include <entity/tile.hpp>
#include <entity/transform.hpp>
#include <entity/velocity.hpp>
#include <entt/entity/utility.hpp>
#include <external/imgui.hpp>
#include <external/imguizmo.hpp>
#include <external/raylib.hpp>
#include <limits>
#include <lua_impl/lua_register.hpp>
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

    const float k = 1.5;
    const float m = 2.0;
    const float t0 = 3;

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

bool buildNavigation = false;

namespace World
{
    void Register(lua_State* lua)
    {
        LuaRegister::GlobalRegister(
            lua,
            "BuildNavigation",
            +[]() {
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

                navigation = Navigation({min.x, min.z}, {max.x, max.z}, min.x, min.z, 0.5f);

                world.registry->view<Component::Walkable, Component::Render, Component::Transform>()
                    .each([&](entt::entity entity,
                              Component::Render render,
                              Component::Transform transform) {
                        // auto bounds = GetModelBoundingBox(render.model);
                        // Vector3 halfBounds =
                        //     Vector3Scale(Vector3Subtract(bounds.max, bounds.min), 0.5f);
                        Vector3 halfBounds = {1.0f, 0.0f, 1.0f};
                        Vector3 min = Vector3Subtract(transform.position, halfBounds);
                        Vector3 max = Vector3Add(transform.position, halfBounds);

                        if(world.registry->try_get<Component::EnemyGoal>(entity))
                        {
                            navigation.SetGoal({min.x, min.z}, {max.x, max.z});
                        }
                        else
                        {
                            navigation.SetWalkable({min.x, min.z}, {max.x, max.z});
                        }
                    });

                world.registry
                    ->view<Component::EnemyGoal, Component::Render, Component::Transform>()
                    .each([&](entt::entity entity,
                              Component::EnemyGoal _,
                              Component::Render render,
                              Component::Transform transform) {
                        Vector3 halfBounds = {1.0f, 0.0f, 1.0f};
                        // Vector3 halfBounds =
                        // Vector3Scale(Vector3Subtract(bounds.max, bounds.min), 0.5f);
                        Vector3 min = Vector3Subtract(transform.position, halfBounds);
                        Vector3 max = Vector3Add(transform.position, halfBounds);

                        navigation.SetGoal({min.x, min.z}, {max.x, max.z});
                    });

                navigation.Build();
            });

        lua_pushboolean(lua, drawNavigation);
        lua_setglobal(lua, "DrawNavigation");
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

        for(auto [entity, transform, moveTowards, velocity, acceleration] :
            world.registry
                ->view<
                    Component::Transform,
                    Component::MoveTowards,
                    Component::Velocity,
                    Component::Acceleration>()
                .each())
        {
            float speed = moveTowards.speed;
            if(Vector3Distance(moveTowards.target, transform.position) <= speed * time)
            {
                // Quick fix
                velocity.x = 0.0f;
                velocity.y = 0.0f;
                velocity.z = 0.0f;
                acceleration.acceleration = Vector3Zero();
                transform.position = moveTowards.target;
                continue;
            }

            // auto movementDirection =
            // Vector3Normalize(Vector3Subtract(moveTowards.target, transform.position));
            Vector2 force = navigation.GetForce({transform.position.x, transform.position.z});
            Vector3 movementDirection = {force.x, 0.0f, force.y};
            Vector3 goalVelocity = Vector3Scale(movementDirection, speed);

            const float ksi = 0.54f;
            Vector2 forces = Vector2Scale(
                Vector2Subtract({goalVelocity.x, goalVelocity.z}, {velocity.x, velocity.z}),
                1.0f / ksi);

            forces = Vector2Add(forces, Vector2Scale({randomNumber(mt), randomNumber(mt)}, 0.5f));

            for(auto [otherEntity, otherTransform, otherHealth] :
                world.registry->view<Component::Transform, Component::Health>().each())
            {
                if(entity == otherEntity)
                    continue;

                if(Vector3Distance(transform.position, otherTransform.position) > 7.0f)
                    continue;

                const float radius = 0.35f;

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
                    {velocity.x, velocity.z},
                    {otherVelocity.x, otherVelocity.z},
                    radius);

                if(coeff.has_value())
                {
                    Vector2 avoidForce = coeff.value();
                    forces.x += avoidForce.x;
                    forces.y += avoidForce.y;
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

        lua_getglobal(lua, "DrawNavigation");
        drawNavigation = lua_toboolean(lua, -1);
        if(drawNavigation)
            navigation.Draw();
    }

    void DrawImgui()
    {
        // Yay this is empty now!
    }
}