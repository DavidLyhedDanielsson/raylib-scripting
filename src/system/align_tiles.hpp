#include <entt/entt.hpp>

#include <component/tile.hpp>
#include <component/transform.hpp>

static float RoundToMultiple(float number, float multiple)
{
    return std::round(number / multiple) * multiple;
}

namespace System
{
    // Aligns entities with the world grid (makes them "tile-based")
    void AlignTiles(entt::registry& registry)
    {
        for(auto [entity, transform] :
            registry.view<Component::Transform, Component::Tile>().each())
        {
            transform.position.x = std::roundf(transform.position.x);
            transform.position.y = std::roundf(transform.position.y);
            transform.position.z = std::roundf(transform.position.z);

            transform.rotation.x = RoundToMultiple(transform.rotation.x * RAD2DEG, 90.0f) * DEG2RAD;
            transform.rotation.y = RoundToMultiple(transform.rotation.y * RAD2DEG, 90.0f) * DEG2RAD;
            transform.rotation.z = RoundToMultiple(transform.rotation.z * RAD2DEG, 90.0f) * DEG2RAD;
        }
    }
}