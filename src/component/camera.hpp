#pragma once

#include <external/raylib.hpp>

namespace Component
{
    struct Camera
    {
        Vector3 target;
        Vector3 up;
        float fovy;
        int projection;

        inline Camera3D CreateRaylibCamera(Vector3 position) const
        {
            return Camera3D{
                .position = position,
                .target = target,
                .up = up,
                .fovy = fovy,
                .projection = projection,
            };
        }
    };
}
