#include <entt/entt.hpp>

#include <component/area_tracker.hpp>
#include <component/health.hpp>
#include <component/render.hpp>
#include <component/transform.hpp>

namespace System
{
    // For all entities with AreaTracker components, find all other entities that are within its
    // area and track them
    void UpdateAreaTrackers(entt::registry& registry)
    {
        for(auto [trackerEntity, trackerTransform, tracker] :
            registry.view<Component::Transform, Component::AreaTracker>().each())
        {
            tracker.entitiesInside.clear();

            auto trackerHitBox = tracker.GetBoundingBox(trackerTransform);

            for(auto [entity, entityRender, entityTransform, entityHealth] :
                registry.view<Component::Render, Component::Transform, Component::Health>().each())
            {
                auto entityHitBox =
                    BoundingBoxTransform(entityRender.boundingBox, entityTransform.position);

                if(CheckCollisionBoxes(trackerHitBox, entityHitBox))
                    tracker.entitiesInside.push_back(entity);
            }
        }
    }
}
