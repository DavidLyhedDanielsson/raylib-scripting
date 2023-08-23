#pragma once

#include <external/raylib.hpp>

namespace Component
{
    struct Render
    {
        const char* assetName;
        Model model;
        // Doesn't strictly have (probably shouldn't) to be part of this component, but it works
        // fine
        BoundingBox boundingBox;
    };
}