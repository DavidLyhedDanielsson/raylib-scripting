#pragma once

// Clang also defines __GNUC__
#if defined(__GNUC__)
#elif defined(_MSC_VER)
    #pragma warning(push, 0)
#endif

#include "../../libs/raylib/src/config.h"
#include "../../libs/raylib/src/raylib.h"
#include "../../libs/raylib/src/raymath.h"

// Also raygui
#include "../../libs/raygui-custom/raygui.h"

#include <cmath>

inline BoundingBox BoundingBoxTransform(BoundingBox box, Vector3 translation)
{
    Vector3 min = Vector3Add(box.min, translation);
    Vector3 max = Vector3Add(box.max, translation);

    return BoundingBox{.min = min, .max = max};
}

inline BoundingBox BoundingBoxTransform(BoundingBox box, Matrix rotation)
{
    // Thanks Real-Time Collision Detection
    Vector3 min = {};
    Vector3 max = {};

    float* floats = &rotation.m0;

    for(int i = 0; i < 3; ++i)
    {
        for(int j = 0; j < 3; ++j)
        {
            const int index = i * 4 + j;

            float e = floats[index] * *(&box.min.x + j);
            float f = floats[index] * *(&box.max.x + j);

            if(e < f)
            {
                *(&min.x + i) += e;
                *(&max.x + i) += f;
            }
            else
            {
                *(&min.x + i) += f;
                *(&max.x + i) += e;
            }
        }
    }

    return BoundingBox{.min = min, .max = max};
}

inline BoundingBox BoundingBoxTransform(BoundingBox box, Vector3 translation, Matrix rotation)
{
    // Thanks Real-Time Collision Detection
    Vector3 min = translation;
    Vector3 max = translation;

    float* floats = &rotation.m0;

    for(int i = 0; i < 3; ++i)
    {
        for(int j = 0; j < 3; ++j)
        {
            const int index = i * 4 + j;

            float e = floats[index] * *(&box.min.x + j);
            float f = floats[index] * *(&box.max.x + j);

            if(e < f)
            {
                *(&min.x + i) += e;
                *(&max.x + i) += f;
            }
            else
            {
                *(&min.x + i) += f;
                *(&max.x + i) += e;
            }
        }
    }

    return BoundingBox{.min = min, .max = max};
}

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
            for(int j = 0; j < mesh.vertexCount; j++)
            {
                minVertex = Vector3Min(
                    minVertex,
                    Vector3Add(
                        {mesh.vertices[j * 3], mesh.vertices[j * 3 + 1], mesh.vertices[j * 3 + 2]},
                        translation));
                maxVertex = Vector3Max(
                    maxVertex,
                    Vector3Add(
                        {mesh.vertices[j * 3], mesh.vertices[j * 3 + 1], mesh.vertices[j * 3 + 2]},
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
            for(int j = 0; j < mesh.vertexCount; j++)
            {
                minVertex = Vector3Min(
                    minVertex,
                    Vector3Transform(
                        {mesh.vertices[j * 3], mesh.vertices[j * 3 + 1], mesh.vertices[j * 3 + 2]},
                        transform));
                maxVertex = Vector3Max(
                    maxVertex,
                    Vector3Transform(
                        {mesh.vertices[j * 3], mesh.vertices[j * 3 + 1], mesh.vertices[j * 3 + 2]},
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

inline Matrix MatrixTranslate(Vector3 v)
{
    return MatrixTranslate(v.x, v.y, v.z);
}

inline float Vector2Dot(Vector2 v1, Vector2 v2)
{
    return v1.x * v2.x + v1.y * v2.y;
}

inline float Vector2Det(Vector2 v1, Vector2 v2)
{
    return v1.x * v2.y - v1.y * v2.x;
}

inline float Vector2Perpendicular(Vector2 v1, Vector2 v2)
{
    return v1.x * v2.y - v1.y * v2.x;
}

inline Vector2 Vector2Floor(Vector2 v1)
{
    return {.x = std::floor(v1.x), .y = std::floor(v1.y)};
}

inline Vector2 Vector2Ceil(Vector2 v1)
{
    return {.x = std::ceil(v1.x), .y = std::ceil(v1.y)};
}

inline Vector3 Vector3DirectionTo(Vector3 v1, Vector3 v2)
{
    return Vector3Normalize(Vector3Subtract(v2, v1));
}

inline Vector2 Vector2DirectionTo(Vector2 v1, Vector2 v2)
{
    return Vector2Normalize(Vector2Subtract(v2, v1));
}

inline Vector2 Vector3Flatten(Vector3 v)
{
    return {.x = v.x, .y = v.z};
}

inline Vector3 Vector2Inflate(Vector2 v, float y = 0.0f)
{
    return {.x = v.x, .y = y, .z = v.y};
}