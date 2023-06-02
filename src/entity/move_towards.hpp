#pragma once

#include <external/raylib.hpp>

namespace Component
{
    struct MoveTowards
    {
        uint32_t vectorFieldId;
        float speed;
    };
}