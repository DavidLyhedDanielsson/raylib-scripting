#pragma once

#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <cstdint>
#include <external/raylib.hpp>
#include <unordered_map>
#include <vector>

class Navigation
{
  public:
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
                uint32_t id;
                uint32_t goalId;
            } spawn;
            struct
            {
                uint32_t id;
            } goal;
            struct
            {
            } obstacle;
        };
        int wallSides;

        template<typename Func>
        void ForEachWall(Func func) const
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

    class VectorField
    {
      public:
        std::vector<std::vector<Vector2>> vectors;

        Vector2 GetForce(uint32_t x, uint32_t y) const;

        template<typename Func>
        void ForEach(const Func& func) const
        {
            for(uint32_t y = 0; y < vectors.size(); ++y)
            {
                for(uint32_t x = 0; x < vectors[y].size(); ++x)
                    func(x, y, vectors[y][x]);
            }
        }
    };

    // If tiles change, so does the vector field, so keep it in the same struct
    struct TileData
    {
        std::vector<std::vector<Tile>> tiles;
        std::unordered_map<int32_t, VectorField> vectorFields;
    } tileData;

    Navigation();
    Navigation(Vector2 min, Vector2 max, float offsetX, float offsetY, float tileSize);

    template<typename Func>
    void ForEachTile(const Func& func)
    {
        // Possible use for const_cast?
        for(uint32_t y = 0; y < tileData.tiles.size(); ++y)
        {
            auto& row = tileData.tiles[y];
            for(uint32_t x = 0; x < row.size(); ++x)
            {
                if(row[x].type != Tile::NONE)
                    func(x, y, row[x]);
            }
        }
    }

    template<typename Func>
    void ForEachTile(const Func& func) const
    {
        for(uint32_t y = 0; y < tileData.tiles.size(); ++y)
        {
            const auto& row = tileData.tiles[y];
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

        const auto sizeY = tileData.tiles.size();
        for(auto y = (uint32_t)min.y; y < (uint32_t)max.y; ++y)
        {
            if(y < 0 || y >= sizeY)
                continue;

            auto& row = tileData.tiles[y];
            const auto sizeX = row.size();
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

    Vector2 GetForce(int32_t fieldId, Vector2 position) const;
    Wall GetWall(uint32_t tileX, uint32_t tileY, Tile::Side wallSide) const;
    uint32_t GetSizeX() const;
    uint32_t GetSizeY() const;

    void SetWalkable(Vector2 min, Vector2 max);
    void SetGoal(uint32_t id, Vector2 min, Vector2 max);
    void SetSpawn(uint32_t id, uint32_t goalId, Vector2 min, Vector2 max);
    void SetObstacle(Vector2 min, Vector2 max);
    void SetWall(uint64_t x, uint64_t y, Tile::Side side);
    void SetVectorField(uint32_t fieldId, const std::vector<std::vector<Vector2>>& field);
    void SetVectorField(uint32_t fieldId, std::vector<std::vector<Vector2>>&& field);

    bool IsValid(int64_t x, int64_t y) const;
    bool IsReachable(int64_t x, int64_t y) const;
    bool IsWalkable(int64_t x, int64_t y) const;
    bool IsGoal(Vector3 position) const;

    Vector2 GetTileSpace(Vector2 position) const;
    Vector2 ConvertToWorldSpace(uint32_t tileX, uint32_t tileY) const;
    void ConvertToTileSpace(Vector2& min, Vector2& max) const;

    void DrawTiles() const;
    void DrawField(int32_t fieldId) const;
};