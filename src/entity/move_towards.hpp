#pragma once

#include <external/raylib.hpp>

namespace Component
{
    struct MoveTowards
    {
        Vector3 target;
        float speed;
    };
}