#include "world.hpp"
#include "assets.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <entity/camera.hpp>
#include <entity/render.hpp>
#include <entity/tile.hpp>
#include <entity/transform.hpp>
#include <entity/velocity.hpp>
#include <entity_reflection/entity_reflection.hpp>
#include <entity_reflection/reflection_camera.hpp>
#include <entity_reflection/reflection_render.hpp>
#include <entity_reflection/reflection_tile.hpp>
#include <entity_reflection/reflection_transform.hpp>
#include <entity_reflection/reflection_velocity.hpp>
#include <external/raylib.hpp>
#include <imgui.h>

#include <ImGuizmo.h>

struct WorldData
{
    entt::registry* registry;
} world;

namespace World
{
    void Init(entt::registry* registry)
    {
        world.registry = registry;
    }

    void Update()
    {
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
        }
    }

    void Draw()
    {
        auto group = world.registry->group<Component::Render, Component::Transform>();

        for(auto entity : group)
        {
            auto [render, transform] = group.get<Component::Render, Component::Transform>(entity);
            render.model.transform = MatrixMultiply(
                MatrixRotateXYZ(transform.rotation),
                MatrixTranslate(transform.position.x, transform.position.y, transform.position.z));

            // DrawModel uses model.transform as well as the parameters here
            DrawModel(render.model, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE);

            // The transform matrix is used here
            render.model.transform = MatrixIdentity();
        }
    }

    void DrawImgui()
    {
        // Yay this is empty now!
    }
}