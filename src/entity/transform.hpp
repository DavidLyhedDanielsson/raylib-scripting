#pragma once

#include <external/raylib.hpp>

namespace Component
{
    struct Transform
    {
        Vector3 position;
        Quaternion rotation;
    };
}