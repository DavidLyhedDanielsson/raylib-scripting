#include "navigation.hpp"

#include <cassert>
#include <external/raylib.hpp>
#include <limits>

Navigation::Navigation() {}

Navigation::Navigation(Vector2 min, Vector2 max, float offsetX, float offsetY, float tileSize)
    : offsetX(offsetX)
    , offsetY(offsetY)
    , tileSize(tileSize)
{
    assert(tileSize != 0.0f);
    assert(max.x >= min.x);
    assert(max.y >= min.y);

    tileData = {
        .tiles = {},
        .vectorFields = {},
    };

    // If size is 5, [0; 5] are all valid positions
    tileData.tiles.resize(
        (max.y - min.y) / tileSize,
        std::vector<Tile>(
            (max.x - min.x) / tileSize,
            Tile{.type = Tile::NONE, .wallSides = (int32_t)Tile::Side::NONE}));
}

Vector2 Navigation::GetForce(int32_t fieldId, Vector2 position) const
{
    auto [x, y] = GetTileSpace(position);

    if(IsValid((int64_t)x, (int64_t)y))
    {
        auto iter = tileData.vectorFields.find(fieldId);
        if(iter != tileData.vectorFields.end())
            return iter->second.GetForce((int32_t)x, (int32_t)y);
    }

    return Vector2Zero();
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

uint32_t Navigation::GetSizeX() const
{
    if(tileData.tiles.empty())
        return 0;

    return tileData.tiles[0].size();
}

uint32_t Navigation::GetSizeY() const
{
    return tileData.tiles.size();
}

void Navigation::SetWalkable(Vector2 min, Vector2 max)
{
    ForArea(min, max, [](Tile& tile) { tile.type = Tile::WALKABLE; });
}

void Navigation::SetGoal(uint32_t id, Vector2 min, Vector2 max)
{
    ForArea(min, max, [&](Tile& tile) {
        tile.type = Tile::GOAL;
        tile.goal.id = id;
    });
}

void Navigation::SetSpawn(uint32_t id, uint32_t goalId, Vector2 min, Vector2 max)
{
    ForArea(min, max, [&](Tile& tile) {
        tile.type = Tile::SPAWN;
        tile.spawn.id = id;
        tile.spawn.goalId = goalId;
    });
}

void Navigation::SetObstacle(Vector2 min, Vector2 max)
{
    ForArea(min, max, [](Tile& tile) { tile.type = Tile::OBSTACLE; });
}

void Navigation::SetWall(uint64_t x, uint64_t y, Tile::Side side)
{
    if(IsReachable(x, y))
        tileData.tiles[y][x].wallSides |= (int32_t)side;
}

void Navigation::SetVectorField(uint32_t fieldId, const std::vector<std::vector<Vector2>>& field)
{
    tileData.vectorFields[fieldId] = VectorField{.vectors = field};
}

void Navigation::SetVectorField(uint32_t fieldId, std::vector<std::vector<Vector2>>&& field)
{
    tileData.vectorFields[fieldId] = VectorField{.vectors = std::move(field)};
}

bool Navigation::IsValid(int64_t x, int64_t y) const
{
    if(y < 0 || y >= (int64_t)tileData.tiles.size())
        return false;
    if(x < 0 || x >= (int64_t)tileData.tiles[0].size())
        return false;

    return true;
}

bool Navigation::IsReachable(int64_t x, int64_t y) const
{
    return IsValid(x, y) && tileData.tiles[y][x].type != Tile::NONE;
}

bool Navigation::IsWalkable(int64_t x, int64_t y) const
{
    return IsValid(x, y) && tileData.tiles[y][x].type != Tile::NONE
           && tileData.tiles[y][x].type != Tile::OBSTACLE;
}

bool Navigation::IsGoal(Vector3 pos) const
{
    auto tilePos = GetTileSpace({.x = pos.x, .y = pos.z});
    if(!IsReachable((int64_t)tilePos.x, (int64_t)tilePos.y))
        return false;
    return tileData.tiles[(int64_t)tilePos.y][(int64_t)tilePos.x].type == Tile::GOAL;
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

void Navigation::DrawTiles() const
{
    ForEachTile([&](uint32_t x, uint32_t y, const Tile& tile) {
        tile.ForEachWall([&](Tile::Side side) {
            Wall wall = GetWall(x, y, side);
            DrawLine3D(
                Vector3{wall.start.x, 0.5f, wall.start.y},
                {wall.end.x, 0.5f, wall.end.y},
                ORANGE);

            Vector2 center =
                Vector2Add(wall.start, Vector2Scale(Vector2Subtract(wall.end, wall.start), 0.5f));
            DrawLine3D(
                {center.x, 0.5f, center.y},
                Vector3Add(
                    {center.x, 0.5f, center.y},
                    {.x = wall.normal.x, .y = 0.0f, .z = wall.normal.y}),
                ORANGE);
        });
    });
}

void Navigation::DrawField(int32_t fieldId) const
{
    auto iter = tileData.vectorFields.find(fieldId);
    if(iter == tileData.vectorFields.end())
        return;

    const auto& vectorField = iter->second;
    vectorField.ForEach([&](uint32_t x, uint32_t y, Vector2 direction) {
        Vector3 start = {
            .x = (float)x * tileSize + offsetX + tileSize * 0.5f,
            .y = 0.2f,
            .z = (float)y * tileSize + offsetY + tileSize * 0.5f};
        Vector3 end = Vector3Add(
            start,
            Vector3Scale({.x = direction.x, .y = 0.0f, .z = direction.y}, tileSize * 0.75f));

        Color color = [&]() {
            switch(tileData.tiles[y][x].type)
            {
                case(Tile::GOAL): return GREEN;
                case(Tile::SPAWN): return BLUE;
                default: return RED;
            }
        }();

        DrawCube(start, 0.1f, 0.1f, 0.1f, ColorAlpha(color, 0.2f));
        DrawLine3D(start, end, color);
    });
}

Vector2 Navigation::VectorField::GetForce(uint32_t x, uint32_t y) const
{
    return this->vectors[y][x];
}