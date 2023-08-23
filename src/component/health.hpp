#pragma once

#include <external/raylib.hpp>

namespace Component
{
    struct Health
    {
        // Using the same name for a component and its properties can cause an
        // ID collision in imgui if, for instance, a tree node is called
        // "Health" and it contains a widget with the text "Health"
        float currentHealth;
    };
}
