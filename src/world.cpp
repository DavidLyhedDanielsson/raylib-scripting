#include "world.hpp"
#include <algorithm>
#include <array>
#include <assets.hpp>
#include <cmath>
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
#include <entity_reflection/entity_reflection.hpp>
#include <external/imgui.hpp>
#include <external/imguizmo.hpp>
#include <external/raylib.hpp>

struct WorldData
{
    entt::registry* registry;
} world;

float RoundToMultiple(float number, float multiple)
{
    return std::round(number / multiple) * multiple;
}

namespace World
{
    void Init(entt::registry* registry)
    {
        world.registry = registry;
    }

    // TODO: `lua` should probably be in WorldData
    void Update(lua_State* lua)
    {
        float time = 1.0f / 60.0f;

        for(auto [entity, transform, velocity, moveTowards] :
            world.registry
                ->view<Component::Transform, Component::Velocity, Component::MoveTowards>()
                .each())
        {
            auto movementDirection =
                Vector3Normalize(Vector3Subtract(moveTowards.target, transform.position));

            float speed = moveTowards.speed;
            if(Vector3Distance(moveTowards.target, transform.position) <= speed * time)
                speed = 0.0f;

            Vector3 finalVelocity = Vector3Scale(movementDirection, speed);
            velocity.x = finalVelocity.x;
            velocity.y = finalVelocity.y;
            velocity.z = finalVelocity.z;
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
        auto group = world.registry->group<Component::Render, Component::Transform>(
            entt::exclude<Component::Velocity>);
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

        for(auto [entity, render, transform, moveTowards, velocity] :
            world.registry
                ->view<
                    Component::Render,
                    Component::Transform,
                    Component::MoveTowards,
                    Component::Velocity>()
                .each())
        {
            DrawLine3D(transform.position, moveTowards.target, GREEN);

            Vector3 mid = transform.position;
            mid.y += 1.0f;
            DrawLine3D(
                mid,
                Vector3Add(
                    mid,
                    Vector3Scale(
                        Vector3Normalize({velocity.x, velocity.y, velocity.z}),
                        moveTowards.speed)),
                BLUE);

            Color meshColor = WHITE;

            for(auto [otherEntity, otherTransform, otherMoveTowards, otherVelocity] :
                world.registry
                    ->view<Component::Transform, Component::MoveTowards, Component::Velocity>()
                    .each())
            {
                if(entity == otherEntity)
                    continue;

                const float radius = 0.45f;

                if(Vector3Distance(transform.position, otherTransform.position) <= 2.0f * radius)
                {
                    meshColor = GREEN;
                    continue;
                }

                Vector3 relPos = Vector3Subtract(otherTransform.position, transform.position);
                Vector3 relVel = Vector3Subtract(
                    // These are already normalized
                    Vector3Normalize({velocity.x, velocity.y, velocity.z}),
                    Vector3Normalize({otherVelocity.x, otherVelocity.y, otherVelocity.z}));

                float a = Vector3DotProduct(relVel, relVel);
                float b = Vector3DotProduct(relPos, relVel);
                float c = Vector3DotProduct(relPos, relPos) - std::pow((2.0f * radius), 2.0f);

                float disc = b * b - a * c;
                if(disc < 0.0f || (disc < 0.00001f && disc > -0.00001))
                    continue;

                disc = std::sqrt(disc);
                float t = (b - disc) / a;

                if(t < 0.0f)
                    continue;
                if(t > 999.0f)
                    continue;

                DrawSphere(
                    Vector3Add(
                        transform.position,
                        Vector3Scale(Vector3Normalize({velocity.x, velocity.y, velocity.z}), t)),
                    radius,
                    ColorAlpha(RED, 0.2f));
            }

            render.model.transform = MatrixMultiply(
                MatrixRotateZYX(transform.rotation),
                MatrixTranslate(transform.position.x, transform.position.y, transform.position.z));
            DrawModel(render.model, {0.0f, 0.0f, 0.0f}, 1.0f, meshColor);

            render.model.transform = MatrixIdentity();
        }
    }

    void DrawImgui()
    {
        // Yay this is empty now!
    }
}