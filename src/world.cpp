#include "world.hpp"
#include "assets.hpp"
#include "entity/move_towards.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <entity/camera.hpp>
#include <entity/render.hpp>
#include <entity/tile.hpp>
#include <entity/transform.hpp>
#include <entity/velocity.hpp>
#include <entity_reflection/entity_reflection.hpp>
#include <external/raylib.hpp>
#include <imgui.h>

#include <ImGuizmo.h>
#include <raymath.h>

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

    void Update()
    {
        for(auto [entity, transform, velocity, moveTowards] :
            world.registry
                ->view<Component::Transform, Component::Velocity, Component::MoveTowards>()
                .each())
        {
            auto movementDirection =
                Vector3Normalize(Vector3Subtract(moveTowards.target, transform.position));

            float speed = moveTowards.speed;
            if(Vector3Distance(moveTowards.target, transform.position) <= speed)
                speed = Vector3Distance(moveTowards.target, transform.position);

            Vector3 finalVelocity = Vector3Scale(movementDirection, speed);
            velocity.x = finalVelocity.x;
            velocity.y = finalVelocity.y;
            velocity.z = finalVelocity.z;
        }

        for(auto [entity, transform, velocity] :
            world.registry->view<Component::Transform, Component::Velocity>().each())
        {
            transform.position.x += velocity.x;
            transform.position.y += velocity.y;
            transform.position.z += velocity.z;
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
    }

    void Draw()
    {
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
    }

    void DrawImgui()
    {
        // Yay this is empty now!
    }
}