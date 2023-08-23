#include <entt/entt.hpp>

#include <component/health.hpp>

namespace System
{
    // Destroy any entities that have no health left
    void CheckHealth(entt::registry& registry)
    {
        for(auto [entity, health] : registry.view<Component::Health>().each())
        {
            if(health.currentHealth <= 0.0001f)
                registry.destroy(entity);
        }
    }
}
