#include <entt/entt.hpp>

#include <component/max_range.hpp>
#include <component/transform.hpp>


namespace System
{
    // Destroy any entities that have reached their max range
    void MaxRange(entt::registry& registry)
    {
        // From the docs:
        // "Deleting the current entity or removing its components is allowed during iterations"
        for(auto [entity, transform, maxRange] :
            registry.view<Component::Transform, Component::MaxRange>().each())
        {
            if(Vector3Distance(transform.position, maxRange.distanceFrom) >= maxRange.maxDistance)
                registry.destroy(entity);
        }
    }
}