#include <entt/entt.hpp>

#include <component/health.hpp>
#include <component/projectile.hpp>
#include <component/render.hpp>
#include <component/transform.hpp>

namespace System
{
    // Move all projectiles and check collision between them and enemies, destroying the projectile
    // entity if it hits anything
    void UpdateProjectiles(entt::registry& registry)
    {
        for(auto [projectileEntity, projectileRender, projectileTransform, projectile] :
            registry.view<Component::Render, Component::Transform, Component::Projectile>().each())
        {
            bool destroy = false;

            auto projectileHitBox =
                BoundingBoxTransform(projectileRender.boundingBox, projectileTransform.position);

            // Entity = entity (potentially) affected by projectile
            for(auto [entity, entityRender, entityTransform, entityHealth] :
                registry.view<Component::Render, Component::Transform, Component::Health>().each())
            {
                auto entityHitBox =
                    BoundingBoxTransform(entityRender.boundingBox, entityTransform.position);

                if(CheckCollisionBoxes(projectileHitBox, entityHitBox))
                {
                    // Entity will be destroyed in a different system
                    entityHealth.currentHealth -= projectile.damage;
                    destroy = true;
                }
            }

            if(destroy)
                registry.destroy(projectileEntity);
        }
    }
}