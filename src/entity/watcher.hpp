#pragma once

#include <external/raylib.hpp>
#include <raymath.h>

namespace Component
{
    struct Watcher
    {
        Vector3 offset;
        Vector3 size;

        inline BoundingBox GetBoundingBox(Vector3 position) const
        {
            Vector3 halfSize = Vector3Scale(size, 0.5f);
            Vector3 min = Vector3Subtract(Vector3Add(position, offset), halfSize);
            return BoundingBox{
                .min = min,
                .max = Vector3Add(min, size),
            };
        }
    };
}