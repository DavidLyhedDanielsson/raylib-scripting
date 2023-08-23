#pragma once

#include <cstdint>
#include <vector>

namespace Component
{
    struct NavGate
    {
        std::vector<uint32_t> allowedGoalIds;
    };
}