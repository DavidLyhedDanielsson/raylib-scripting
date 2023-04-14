#pragma once

// Clang also defines __GNUC__
#if defined(__GNUC__)
#elif defined(_MSC_VER)
    #pragma warning(push, 0)
#endif

#include <raylib.h>
#include <raymath.h>

#if defined(__GNUC__)
#elif defined(_MSC_VER)
    #pragma warning(pop)
#endif