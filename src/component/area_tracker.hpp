#pragma once

#include <component/transform.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/fwd.hpp>
#include <external/raylib.hpp>
#include <vector>

namespace Component
{
    struct AreaTracker
    {
        Vector3 offset;
        Vector3 size;
        std::vector<entt::entity> entitiesInside;

        inline BoundingBox GetBoundingBox(Component::Transform entityTransform) const
        {
            // This could all be one matrix calculation, but that would require
            // 3 matrix multiplications and probably be slower
            const Vector3 halfSize = Vector3Scale(size, 0.5f);
            const Vector3 localMin = {-halfSize.x, -halfSize.y, 0.0f};
            const Vector3 localMax = {halfSize.x, halfSize.y, size.z};

            const Vector3 offsetMin = Vector3Add(localMin, offset);
            const Vector3 offsetMax = Vector3Add(localMax, offset);

            const Matrix worldMatrix = MatrixRotateZYX(entityTransform.rotation);
            // Note: not actually component-wise min and max after transformation!
            const Vector3 worldMin = Vector3Transform(offsetMin, worldMatrix);
            const Vector3 worldMax = Vector3Transform(offsetMax, worldMatrix);

            const Vector3 finalNear = Vector3Add(worldMin, entityTransform.position);
            const Vector3 finalFar = Vector3Add(worldMax, entityTransform.position);

            return BoundingBox{
                .min = Vector3Min(finalNear, finalFar),
                .max = Vector3Max(finalNear, finalFar),
            };
        }
    };
}