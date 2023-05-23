#include "navigation.hpp"
#include "raylib.h"
#include "raymath.h"

#include <cassert>
#include <limits>
#include <queue>

Navigation::Navigation() {}

Navigation::Navigation(Vector2 min, Vector2 max, float offsetX, float offsetY, float tileSize)
    : sizeX((uint32_t)((max.x - min.x) / tileSize)) // If size is 5, [0; 5] are all valid positions
    , sizeY((uint32_t)((max.y - min.y) / tileSize))
    , offsetX(offsetX)
    , offsetY(offsetY)
    , tileSize(tileSize)
{
    assert(tileSize != 0.0f);
    assert(max.x >= min.x);
    assert(max.y >= min.y);

    tiles.resize(
        this->sizeY,
        std::vector<Tile>(
            this->sizeX,
            Tile{.type = Tile::NONE, .wallSides = (int32_t)Tile::Side::NONE}));
}

void Navigation::Build()
{
    // This is implemented in lua now!
    assert(false);
}

Vector2 Navigation::GetForce(Vector2 position)
{
    Vector2 offset = {.x = -this->offsetX, .y = -this->offsetY};
    position = Vector2Add(position, offset);
    position = Vector2Scale(position, 1.0f / this->tileSize);

    if(Valid((int64_t)position.x, (int64_t)position.y))
        return vectorField[(int)position.y][(int)position.x];
    else
        return Vector2Zero();
}

Vector2 Vector2Round(Vector2 v)
{
    Vector2 result{.x = std::round(v.x), .y = std::round(v.y)};
    return result;
}

Vector2 Navigation::GetTileSpace(Vector2 position) const
{
    Vector2 offset = {.x = -this->offsetX, .y = -this->offsetY};
    return Vector2Round(Vector2Scale(Vector2Add(position, offset), 1.0f / tileSize));
}

void Navigation::ConvertToTileSpace(Vector2& min, Vector2& max) const
{
    Vector2 offset = {.x = -this->offsetX, .y = -this->offsetY};
    min = Vector2Round(Vector2Scale(Vector2Add(min, offset), 1.0f / tileSize));
    max = Vector2Round(Vector2Scale(Vector2Add(max, offset), 1.0f / tileSize));
}

Vector2 Navigation::ConvertToWorldSpace(uint32_t tileX, uint32_t tileY) const
{
    return {
        .x = (float)tileX * tileSize + offsetX,
        .y = (float)tileY * tileSize + offsetY,
    };
}

Navigation::Wall Navigation::GetWall(uint32_t tileX, uint32_t tileY, Tile::Side wallSide) const
{
    const Vector2 topLeft = {
        .x = (float)tileX * tileSize + offsetX,
        .y = (float)tileY * tileSize + offsetY,
    };
    const Vector2 topRight = {
        .x = (float)tileX * tileSize + offsetX + tileSize,
        .y = (float)tileY * tileSize + offsetY,
    };
    const Vector2 bottomLeft = {
        .x = (float)tileX * tileSize + offsetX,
        .y = (float)tileY * tileSize + offsetY + tileSize,
    };
    const Vector2 bottomRight = {
        .x = (float)tileX * tileSize + offsetX + tileSize,
        .y = (float)tileY * tileSize + offsetY + tileSize,
    };

    switch(wallSide)
    {
        case Tile::Side::TOP:
            return {
                .start = topLeft,
                .end = topRight,
                .normal = {.x = 0.0f, .y = 1.0f},
            };
        case Tile::Side::BOTTOM:
            return {
                .start = bottomLeft,
                .end = bottomRight,
                .normal = {.x = 0.0f, .y = -1.0f},
            };
        case Tile::Side::LEFT:
            return {
                .start = topLeft,
                .end = bottomLeft,
                .normal = {.x = 1.0f, .y = 0.0f},
            };
        case Tile::Side::RIGHT:
            return {
                .start = topRight,
                .end = bottomRight,
                .normal = {.x = -1.0f, .y = 0.0f},
            };
    }
    assert(false && "No you");
}

void Navigation::SetWalkable(Vector2 min, Vector2 max)
{
    ForArea(min, max, [](Tile& tile) { tile.type = Tile::WALKABLE; });
}

void Navigation::SetGoal(Vector2 min, Vector2 max)
{
    ForArea(min, max, [](Tile& tile) { tile.type = Tile::GOAL; });
}

void Navigation::SetSpawn(Vector2 min, Vector2 max)
{
    ForArea(min, max, [](Tile& tile) { tile.type = Tile::SPAWN; });
}

void Navigation::SetObstacle(Vector2 min, Vector2 max)
{
    ForArea(min, max, [](Tile& tile) { tile.type = Tile::OBSTACLE; });
}

void Navigation::SetWall(uint64_t x, uint64_t y, Tile::Side side)
{
    if(Reachable(x, y))
        tiles[y][x].wallSides |= (int32_t)side;
}

bool Navigation::Valid(int64_t x, int64_t y)
{
    if(x < 0 || x >= sizeX)
        return false;
    if(y < 0 || y >= sizeY)
        return false;

    return true;
}

bool Navigation::Reachable(int64_t x, int64_t y)
{
    return Valid(x, y) && tiles[y][x].type != Tile::NONE;
}

bool Navigation::Walkable(int64_t x, int64_t y)
{
    return Valid(x, y) && tiles[y][x].type != Tile::NONE && tiles[y][x].type != Tile::OBSTACLE;
}

bool Navigation::IsGoal(Vector3 pos)
{
    auto tilePos = GetTileSpace({.x = pos.x, .y = pos.z});
    if(!Reachable((int64_t)tilePos.x, (int64_t)tilePos.y))
        return false;
    return tiles[(int64_t)tilePos.y][(int64_t)tilePos.x].type == Tile::GOAL;
}

void Navigation::Draw()
{
    for(uint32_t y = 0; y < vectorField.size(); ++y)
    {
        for(uint32_t x = 0; x < vectorField[y].size(); ++x)
        {
            auto direction = vectorField[y][x];
            Vector3 start = {
                .x = (float)x * tileSize + offsetX + tileSize * 0.5f,
                .y = 0.2f,
                .z = (float)y * tileSize + offsetY + tileSize * 0.5f};
            Vector3 end = Vector3Add(
                start,
                Vector3Scale({.x = direction.x, .y = 0.0f, .z = direction.y}, tileSize * 0.75f));

            Color color = [&]() {
                switch(tiles[y][x].type)
                {
                    case(Tile::GOAL): return GREEN;
                    case(Tile::SPAWN): return BLUE;
                    default: return RED;
                }
            }();

            // DrawSphere(start, 0.1f, color);
            DrawCube(start, 0.1f, 0.1f, 0.1f, ColorAlpha(color, 0.2f));
            DrawLine3D(start, end, color);
        }
    }

    for(uint32_t y = 0; y < tiles.size(); ++y)
    {
        for(uint32_t x = 0; x < tiles[y].size(); ++x)
        {
            tiles[y][x].forEachWall([&](Tile::Side side) {
                Wall wall = GetWall(x, y, side);
                DrawLine3D(
                    Vector3{wall.start.x, 0.5f, wall.start.y},
                    {wall.end.x, 0.5f, wall.end.y},
                    ORANGE);

                Vector2 center = Vector2Add(
                    wall.start,
                    Vector2Scale(Vector2Subtract(wall.end, wall.start), 0.5f));
                DrawLine3D(
                    {center.x, 0.5f, center.y},
                    Vector3Add(
                        {center.x, 0.5f, center.y},
                        {.x = wall.normal.x, .y = 0.0f, .z = wall.normal.y}),
                    ORANGE);
            });
        }
    }
}