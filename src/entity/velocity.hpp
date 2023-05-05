#pragma once

#include <external/raylib.hpp>

namespace Component
{
    struct Velocity
    {
        float x;
        float y;
        float z;

        Vector3 ToVector3()
        {
            return {x, y, z};
        }
    };
}