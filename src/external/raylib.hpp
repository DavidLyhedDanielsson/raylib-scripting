#pragma once

// Clang also defines __GNUC__
#if defined(__GNUC__)
#elif defined(_MSC_VER)
    #pragma warning(push, 0)
#endif

#include "../../libs/raylib/src/config.h"
#include "../../libs/raylib/src/raylib.h"
#include "../../libs/raylib/src/raymath.h"

#if defined(__GNUC__)
#elif defined(_MSC_VER)
    #pragma warning(pop)
#endif

// Taken from Raylib's source and modified
inline BoundingBox GetModelBoundingBox(Model model, Vector3 translation)
{
    auto fmin = std::numeric_limits<float>::lowest();
    auto fmax = std::numeric_limits<float>::max();
    Vector3 minVertex{fmax, fmax, fmax};
    Vector3 maxVertex{fmin, fmin, fmin};

    for(int i = 0; i < model.meshCount; ++i)
    {
        Mesh mesh = model.meshes[i];
        if(mesh.vertices != NULL)
        {
            for(int i = 0; i < mesh.vertexCount; i++)
            {
                minVertex = Vector3Min(
                    minVertex,
                    Vector3Add(
                        {mesh.vertices[i * 3], mesh.vertices[i * 3 + 1], mesh.vertices[i * 3 + 2]},
                        translation));
                maxVertex = Vector3Max(
                    maxVertex,
                    Vector3Add(
                        {mesh.vertices[i * 3], mesh.vertices[i * 3 + 1], mesh.vertices[i * 3 + 2]},
                        translation));
            }
        }
    }

    // Create the bounding box
    BoundingBox box = {};
    box.min = minVertex;
    box.max = maxVertex;

    return box;
}

// Taken from Raylib's source and modified
inline BoundingBox GetModelBoundingBox(Model model, Matrix transform)
{
    auto fmin = std::numeric_limits<float>::lowest();
    auto fmax = std::numeric_limits<float>::max();
    Vector3 minVertex{fmax, fmax, fmax};
    Vector3 maxVertex{fmin, fmin, fmin};

    for(int i = 0; i < model.meshCount; ++i)
    {
        Mesh mesh = model.meshes[i];
        if(mesh.vertices != NULL)
        {
            for(int i = 0; i < mesh.vertexCount; i++)
            {
                minVertex = Vector3Min(
                    minVertex,
                    Vector3Transform(
                        {mesh.vertices[i * 3], mesh.vertices[i * 3 + 1], mesh.vertices[i * 3 + 2]},
                        transform));
                maxVertex = Vector3Max(
                    maxVertex,
                    Vector3Transform(
                        {mesh.vertices[i * 3], mesh.vertices[i * 3 + 1], mesh.vertices[i * 3 + 2]},
                        transform));
            }
        }
    }

    // Create the bounding box
    BoundingBox box = {};
    box.min = minVertex;
    box.max = maxVertex;

    return box;
}

inline bool CheckCollisionPointBox(Vector3 point, BoundingBox box)
{
    return point.x >= box.min.x && point.y >= box.min.y && point.z >= box.min.z
           && point.x <= box.max.x && point.y <= box.max.y && point.z <= box.max.z;
}
