#pragma once

#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <cstdint>
#include <external/raylib.hpp>
#include <vector>

class Navigation
{
    uint32_t sizeX;
    uint32_t sizeY;
    float offsetX;
    float offsetY;
    float tileSize;

    struct Tile
    {
        enum Type
        {
            NONE,
            WALKABLE,
            SPAWN,
            GOAL,
        };
        Type type;
        // This is where I miss rust enums :(
        // And, you know, always :'(
        union
        {
            struct
            {
            } none;
            struct
            {
            } walkable;
            struct
            {
                int32_t goalId;
            } spawn;
            struct
            {
                int32_t id;
            } goal;
        };
    };

    std::vector<std::vector<Tile>> tiles;
    std::vector<std::vector<Vector2>> vectorField;

    void ConvertToTileSpace(Vector2& min, Vector2& max) const;

  public:
    Navigation();
    Navigation(Vector2 min, Vector2 max, float offsetX, float offsetY, float tileSize);

    void Build();

    template<typename Func>
    void ForEachTile(const Func& func)
    {
        for(uint32_t y = 0; y < tiles.size(); ++y)
        {
            const auto& row = tiles[y];
            for(uint32_t x = 0; x < row.size(); ++x)
            {
                if(row[x].type != Tile::NONE)
                {
                    if(!func(x, y))
                        return;
                }
            }
        }
    }

    template<typename Func>
    void ForArea(Vector2 min, Vector2 max, const Func& func)
    {
        ConvertToTileSpace(min, max);

        Vector2 startTile = {std::round(min.x / tileSize), std::round(min.y / tileSize)};
        Vector2 endTile = {std::round(max.x / tileSize), std::round(max.y / tileSize)};

        for(uint32_t y = startTile.y; y < endTile.y; ++y)
        {
            auto& row = tiles[y];
            for(uint32_t x = startTile.x; x < endTile.x; ++x)
            {
                func(row[x]);
            }
        }
    }

    Vector2 GetForce(Vector2 position)
    {
        Vector2 offset = {.x = -this->offsetX, .y = -this->offsetY};
        position = Vector2Add(position, offset);
        position = Vector2Scale(position, 1.0f / this->tileSize);

        return vectorField[(int)position.y][(int)position.x];
    }

    void SetWalkable(Vector2 min, Vector2 max);
    void SetGoal(Vector2 min, Vector2 max);

    bool Reachable(int64_t x, int64_t y);

    void Draw();
};