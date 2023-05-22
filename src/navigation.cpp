#include "navigation.hpp"
#include "raylib.h"
#include "raymath.h"

#include <limits>
#include <queue>

Navigation::Navigation() {}

Navigation::Navigation(Vector2 min, Vector2 max, float offsetX, float offsetY, float tileSize)
    : sizeX((max.x - min.x) / tileSize) // If size is 5, [0; 5] are all valid positions
    , sizeY((max.y - min.y) / tileSize)
    , offsetX(offsetX)
    , offsetY(offsetY)
    , tileSize(tileSize)
{
    assert(tileSize != 0.0f);

    tiles.resize(this->sizeY, std::vector<Tile>(this->sizeX, Tile{.type = Tile::NONE}));
}

void Navigation::Build()
{
    // Size of map is uint32_t, so use int64_t to avoid unsigned issues
    struct TileRef
    {
        int64_t x;
        int64_t y;
    };

    std::vector<std::vector<uint32_t>> goalDistance(
        sizeY,
        std::vector<uint32_t>(sizeX, std::numeric_limits<uint32_t>::max()));

    std::queue<TileRef> openList;

    ForEachTile([&](uint32_t x, uint32_t y, Tile tile) {
        if(tile.type == Tile::GOAL)
        {
            goalDistance[y][x] = 0;
            openList.push({.x = x, .y = y});
        }
    });

    while(!openList.empty())
    {
        auto current = openList.front();
        openList.pop();

        const auto currentDistance = goalDistance[current.y][current.x];

        const TileRef neighbours[]{
            {.x = current.x, .y = current.y - 1},
            {.x = current.x, .y = current.y + 1},
            {.x = current.x - 1, .y = current.y},
            {.x = current.x + 1, .y = current.y},
        };
        for(auto [x, y] : neighbours)
        {
            if(Reachable(x, y) && goalDistance[y][x] > currentDistance + 1)
            {
                goalDistance[y][x] = currentDistance + 1;
                openList.push({.x = x, .y = y});
            }
        }
    }

    std::vector<std::vector<Vector2>> vectorField(
        sizeY,
        std::vector<Vector2>(sizeX, {.x = 0.0f, .y = 0.0f}));

    for(uint32_t y = 0; y < sizeY; ++y)
    {
        for(uint32_t x = 0; x < sizeX; ++x)
        {
            if(tiles[y][x].type == Tile::NONE)
                continue;

            auto upDistance =
                Reachable(x, y - 1) ? goalDistance[y - 1][x] : std::numeric_limits<uint32_t>::max();
            auto downDistance =
                Reachable(x, y + 1) ? goalDistance[y + 1][x] : std::numeric_limits<uint32_t>::max();
            auto leftDistance =
                Reachable(x - 1, y) ? goalDistance[y][x - 1] : std::numeric_limits<uint32_t>::max();
            auto rightDistance =
                Reachable(x + 1, y) ? goalDistance[y][x + 1] : std::numeric_limits<uint32_t>::max();

            if(upDistance == std::numeric_limits<uint32_t>::max()
               && downDistance == std::numeric_limits<uint32_t>::max()
               && leftDistance == std::numeric_limits<uint32_t>::max()
               && rightDistance == std::numeric_limits<uint32_t>::max())
            {
                continue;
            }

            auto closestNeighbour =
                std::min(upDistance, std::min(downDistance, std::min(leftDistance, rightDistance)));

            Vector2 vectorDirection{};

            if(upDistance == closestNeighbour)
                vectorDirection = {.x = 0.0f, .y = -1.0f};
            else if(downDistance == closestNeighbour)
                vectorDirection = {.x = 0.0f, .y = 1.0f};
            else if(leftDistance == closestNeighbour)
                vectorDirection = {.x = -1.0f, .y = 0.0f};
            // This could be an else if, but there is a high chance it would
            // fall through somehow, even though it shouldn't be able to
            else
                vectorDirection = {.x = 1.0f, .y = 0.0f};

            vectorField[y][x] = vectorDirection;
        }
    }

    this->vectorField = vectorField;
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

bool Navigation::Reachable(int64_t x, int64_t y)
{
    if(x < 0 || x >= sizeX)
        return false;
    if(y < 0 || y >= sizeY)
        return false;

    return tiles[y][x].type != Tile::NONE;
}

bool Navigation::IsGoal(Vector3 pos)
{
    auto tilePos = GetTileSpace({.x = pos.x, .y = pos.z});
    if(!Reachable(tilePos.x, tilePos.y))
        return false;
    return tiles[tilePos.y][tilePos.x].type == Tile::GOAL;
}

void Navigation::Draw()
{
    ForEachTile([&](uint32_t x, uint32_t y, Tile) {
        if(vectorField.size() <= y)
            return;
        if(vectorField[y].size() <= x)
            return;

        auto direction = vectorField[y][x];
        Vector3 start = {
            .x = (float)x * tileSize + offsetX + tileSize * 0.5f,
            .y = 0.2,
            .z = (float)y * tileSize + offsetY + tileSize * 0.5f};
        Vector3 end = Vector3Add(
            start,
            Vector3Scale({.x = direction.x, .y = 0.0f, .z = direction.y}, tileSize * 0.75f));

        Color color = tiles[y][x].type == Tile::GOAL ? GREEN : RED;

        // DrawSphere(start, 0.1f, color);
        DrawCube(start, 0.1f, 0.1f, 0.1f, ColorAlpha(color, 0.2f));
        DrawLine3D(start, end, color);
    });
}