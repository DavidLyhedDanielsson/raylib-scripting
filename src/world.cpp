#include "world.hpp"
#include <raylib.h>
#include <entt/entt.hpp>
#include "entity/position.hpp"
#include "entity/render.hpp"
#include "entity/velocity.hpp"
#include "assets.hpp"

struct WorldData
{
    entt::registry registry;
} world;

namespace World
{
    void Init()
    {
        // Insurgent comes from https://quaternius.com/. Thanks Quaternius!
        const auto insurgent = LoadModel(AssetPath("Insurgent/glTF/Insurgent.gltf").data());

        const auto entity = world.registry.create();
        world.registry.emplace<Position>(entity, 0.0f, 0.0f, 0.0f);
        world.registry.emplace<Velocity>(entity, 0.0f, 0.0f, 0.0f);
        world.registry.emplace<Render>(entity, insurgent);
    }

    void Update()
    {
        for (auto [entity, position, velocity] : world.registry.view<Position, Velocity>().each())
        {
            position.x += velocity.x;
            position.y += velocity.y;
            position.z += velocity.z;
        }
    }

    void Draw()
    {
        for (auto [entity, position, render] : world.registry.view<Position, Render>().each())
        {
            DrawModel(render.model, {position.x, position.y, position.z}, 1.0f, WHITE);
        }
    }
}