#include <entt/entt.hpp>

#include <component/area_tracker.hpp>
#include <component/health.hpp>
#include <component/render.hpp>
#include <component/transform.hpp>

namespace System
{
    // What it says on the can
    void DrawRenderable(entt::registry& registry)
    {
        auto group = registry.group<Component::Render, Component::Transform>();
        for(auto entity : group)
        {
            auto [render, transform] = group.get<Component::Render, Component::Transform>(entity);

            // Raylib wants the model transform to be set per model.
            // I want 1 shared model instance for all entities, which is why this is set here and
            // then reset after DrawMode.
            render.model.transform = MatrixMultiply(
                MatrixRotateZYX(transform.rotation),
                MatrixTranslate(transform.position.x, transform.position.y, transform.position.z));
            DrawModel(render.model, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
            render.model.transform = MatrixIdentity();
        }
    }
}