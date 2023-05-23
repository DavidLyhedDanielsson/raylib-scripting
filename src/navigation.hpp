#pragma once

#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <cstdint>
#include <external/raylib.hpp>
#include <vector>

class Navigation
{
  public:
    uint32_t sizeX;
    uint32_t sizeY;
    float offsetX;
    float offsetY;
    float tileSize;

    struct Tile
    {
        enum Type
        {
            NONE = 0,
            WALKABLE,
            SPAWN,
            GOAL,
            OBSTACLE,
        };
        enum class Side
        {
            NONE = 0,
            TOP = 1,
            BOTTOM = 1 << 1,
            LEFT = 1 << 2,
            RIGHT = 1 << 3,
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
            struct
            {
            } obstacle;
        };
        int wallSides;

        template<typename Func>
        void forEachWall(Func func)
        {
            if((wallSides & (int)Side::TOP) != 0)
                func(Side::TOP);
            if((wallSides & (int)Side::BOTTOM) != 0)
                func(Side::BOTTOM);
            if((wallSides & (int)Side::LEFT) != 0)
                func(Side::LEFT);
            if((wallSides & (int)Side::RIGHT) != 0)
                func(Side::RIGHT);
        }
    };

    struct Wall
    {
        Vector2 start;
        Vector2 end;
        Vector2 normal;
    };

    std::vector<std::vector<Tile>> tiles;
    std::vector<std::vector<Vector2>> vectorField;

    Vector2 GetTileSpace(Vector2 position) const;
    void ConvertToTileSpace(Vector2& min, Vector2& max) const;
    Vector2 ConvertToWorldSpace(uint32_t tileX, uint32_t tileY) const;
    Wall GetWall(uint32_t tileX, uint32_t tileY, Tile::Side wallSide) const;

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
                    func(x, y, row[x]);
            }
        }
    }

    template<typename Func>
    void ForArea(Vector2 min, Vector2 max, const Func& func)
    {
        ConvertToTileSpace(min, max);

        if(min.x < 0.0f)
            min.x = 0.0f;
        if(min.y < 0.0f)
            min.y = 0.0f;
        if(max.x < 0.0f)
            max.x = 0.0f;
        if(max.y < 0.0f)
            max.y = 0.0f;

        if(min.x == max.x)
            max.x += 1.0f;
        if(min.y == max.y)
            max.y += 1.0f;

        for(auto y = (uint32_t)min.y; y < (uint32_t)max.y; ++y)
        {
            if(y < 0 || y >= sizeY)
                continue;
            auto& row = tiles[y];
            for(auto x = (uint32_t)min.x; x < (uint32_t)max.x; ++x)
            {
                if(x < 0 || x >= sizeX)
                    continue;

                if constexpr(requires(Tile tile) { func(tile); })
                {
                    func(row[x]);
                }
                if constexpr(requires(Tile tile) { func(tile, 0, 0); })
                {
                    func(row[x], x, y);
                }
            }
        }
    }

    Vector2 GetForce(Vector2 position);

    void SetWalkable(Vector2 min, Vector2 max);
    void SetGoal(Vector2 min, Vector2 max);
    void SetSpawn(Vector2 min, Vector2 max);
    void SetObstacle(Vector2 min, Vector2 max);

    void SetWall(uint64_t x, uint64_t y, Tile::Side side);

    bool Valid(int64_t x, int64_t y);
    bool Reachable(int64_t x, int64_t y);
    bool Walkable(int64_t x, int64_t y);

    bool IsGoal(Vector3 position);

    void Draw();
};