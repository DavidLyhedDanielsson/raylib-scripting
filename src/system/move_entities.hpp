#include <entt/entt.hpp>

#include <component/transform.hpp>
#include <component/velocity.hpp>

namespace System
{
    // For all entities with Transform + Velocity components, update their position
    void MoveEntities(entt::registry& registry, float time)
    {
        for(auto [entity, transform, velocity] :
            registry.view<Component::Transform, Component::Velocity>().each())
        {
            transform.position.x += velocity.x * time;
            transform.position.y += velocity.y * time;
            transform.position.z += velocity.z * time;
        }
    }
}