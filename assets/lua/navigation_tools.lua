---@class Neighbour
---@field x integer
---@field y integer
---@field direction Direction
---@field opposite string

---@class OpenListTile
---@field x integer
---@field y integer
---@field wallId integer
---@field id integer

---@class Tile
---@field distance integer shortest distance from spawn
---@field distanceToUnpassable integer distance from this tile to the closest wall
---@field spawn boolean whether or not this tile is an enemy spawn tile
---@field locked boolean if a walker crosses a spawn tile, it and all subsequent tiles will be locked

---@class Walker
---@field distance integer distance walked
---@field parentDirection Direction direction to the previous tile of this walker
---@field id integer this walker's id
---@field wallId integer the wall that this walker is following

if navigationState == nil then
    navigationState = {
        smoothField = true,
        tileSize = 0.5,
    }
end

---For the given tile, find the tile-distance to the nearest unreachable tile
---@param spawnId integer
---@param goalId integer
---@param x integer
---@param y integer
---@return integer
local function DistanceToUnpassable(spawnId, goalId, x, y)
    if not Navigation.IsReachable(x - 1, y - 1) then
        return 0
    end

    -- The radius of 100 is excessive, but a result is expected to be found a lot sooner
    for radius = 1, 100 do
        -- Expand search radius every iteration. Slow but is fast enough so far
        for ry = -radius, radius do
            for rx = -radius, radius do
                local x = x + rx
                local y = y + ry

                if not Navigation.IsPassable(spawnId, goalId, x - 1, y - 1) then
                    return radius
                end
            end
        end
    end

    return -1
end

---Given two adjacent tiles, check if they share a wall in any of the tiles around them
---@param spawnId integer
---@param goalId integer
---@param x integer x position of first tile
---@param y integer y position of first tile
---@param ox integer x position of second tile
---@param oy integer y position of second tile
---@param searchRadius integer
---@return boolean
local function HasSharedWall(spawnId, goalId, x, y, ox, oy, searchRadius)
    -- Build a grid that is centered on the two tiles (X and Y in the example
    -- below), step around the tiles and add `1` if there is a wall in that
    -- position. After stepping around both points, check if there is a `2` in
    -- this grid. If so, they share a wall.
    -- Example:
    -- ┌─┬─┬─┬─┐
    -- │1│ │ │ │
    -- ├─┼─┼─┼─┤
    -- │1│X│ │ │
    -- ├─┼─┼─┼─┤
    -- │1│2│Y│ │
    -- ├─┼─┼─┼─┤
    -- │ │1│1│1│
    -- └─┴─┴─┴─┘

    local map = {}
    for y = 0, searchRadius * 2 + 1 + 1 do
        map[y] = {}
        for x = 0, searchRadius * 2 + 1 + 1 do
            map[y][x] = 0
        end
    end

    -- Place X
    local offsetX = x > ox and 1 or 0
    local offsetY = y > oy and 1 or 0

    for yy = -searchRadius, searchRadius do
        for xx = -searchRadius, searchRadius do
            if not Navigation.IsPassable(spawnId, goalId, x + xx - 1, y + yy - 1) then
                -- Map from [-sR; sR] to [0; 2*sR]
                local val = map[yy + searchRadius + offsetY][xx + searchRadius + offsetX]
                map[yy + searchRadius + offsetY][xx + searchRadius + offsetX] = val + 1
            end
        end
    end

    -- Place Y
    local offsetX = ox > x and 1 or 0
    local offsetY = oy > y and 1 or 0

    for yy = -searchRadius, searchRadius do
        for xx = -searchRadius, searchRadius do
            if not Navigation.IsPassable(spawnId, goalId, ox + xx - 1, oy + yy - 1) then
                local val = map[yy + searchRadius + offsetY][xx + searchRadius + offsetX]

                if val == 1 then
                    return true
                end
            end
        end
    end

    return false
end

-- Resuse TileSide so we can cast between them
---@enum Direction
local Direction <const> = {
    NONE = Navigation.TileSide.NONE,
    UP = Navigation.TileSide.TOP,
    DOWN = Navigation.TileSide.BOTTOM,
    LEFT = Navigation.TileSide.LEFT,
    RIGHT = Navigation.TileSide.RIGHT,
}

---For the given tile, get neighbours
---@param x integer
---@param y integer
---@return Neighbour[]
local function GetNeighbours(x, y)
    return {
        { x = x,     y = y - 1, direction = Direction.UP,    opposite = Direction.DOWN },
        { x = x,     y = y + 1, direction = Direction.DOWN,  opposite = Direction.UP },
        { x = x - 1, y = y,     direction = Direction.LEFT,  opposite = Direction.RIGHT },
        { x = x + 1, y = y,     direction = Direction.RIGHT, opposite = Direction.LEFT },
    }
end

---Runs the given callback for each neighbour
---@param x integer
---@param y integer
---@param func fun(n: Neighbour)
local function ForEachNeighbour(x, y, func)
    for _, n in ipairs(GetNeighbours(x, y)) do
        func(n)
    end
end

---@param x integer
---@param y integer
---@param func fun(n: Neighbour)
local function ForEachUnwalkableNeighbour(x, y, func)
    for _, n in ipairs(GetNeighbours(x, y)) do
        if not Navigation.IsWalkable(n.x - 1, n.y - 1) then
            func(n)
        end
    end
end

---Runs the given callback for each passable neighbour. A passable tile is
--either walkable or a navgate with a matching goal id.
---@param spawnId integer
---@param goalId integer
---@param x integer
---@param y integer
---@param func fun(n: Neighbour)
local function ForEachPassableNeighbour(spawnId, goalId, x, y, func)
    for _, n in ipairs(GetNeighbours(x, y)) do
        if Navigation.IsPassable(spawnId, goalId, n.x - 1, n.y - 1) then
            func(n)
        end
    end
end

---Runs the given callback for each unpassable neighbour. A passable tile is
--either walkable or a navgate with a matching goal id.
---@param spawnId integer
---@param goalId integer
---@param x integer
---@param y integer
---@param func fun(n: Neighbour)
local function ForEachUnpassableNeighbour(spawnId, goalId, x, y, func)
    for _, n in ipairs(GetNeighbours(x, y)) do
        if not Navigation.IsPassable(spawnId, goalId, n.x - 1, n.y - 1) then
            func(n)
        end
    end
end

---Runs the given function for _all adjacent_ neighbours that are walkable
---@param spawnId integer
---@param goalId integer
---@param x integer
---@param y integer
---@param func fun(n: {x: integer, y: integer})
local function ForEachAdjacentPassable(spawnId, goalId, x, y, func)
    local neighbours = {
        { x = x - 1, y = y - 1, },
        { x = x - 1, y = y, },
        { x = x - 1, y = y + 1, },

        { x = x,     y = y - 1, },
        { x = x,     y = y + 1, },

        { x = x + 1, y = y - 1, },
        { x = x + 1, y = y, },
        { x = x + 1, y = y + 1, },
    }
    for _, n in ipairs(neighbours) do
        if Navigation.IsPassable(spawnId, goalId, n.x - 1, n.y - 1) then
            func(n)
        end
    end
end

---Given a tile, checks whether or not it is adjacent to the walker with the given wall id
---@param spawnId integer
---@param goalId integer
---@param x integer
---@param y integer
---@param walkerWallId integer
---@return boolean
function IsAdjacentToWalker(spawnId, goalId, x, y, walkerWallId)
    for ry = -1, 1, 1 do
        for rx = -1, 1, 1 do
            local x = x + rx
            local y = y + ry

            if Navigation.IsPassable(spawnId, goalId, x - 1, y - 1) then
                if walkerMap[y][x].id == walkerWallId then
                    return true
                end
            elseif walkerWallId == 0 then -- wallId 0 is reserved
                return true
            end
        end
    end

    return false
end

-- 0 is reserved for unreachable tiles!
local idCounter = 1
local function GetId()
    idCounter = idCounter + 1
    return idCounter - 1
end

---Given a walker, sets the given position as this walker's next step
---@param walker Walker
---@param x integer
---@param y integer
---@param parentDirection Direction
local function SetNextWalkerStep(walker, x, y, parentDirection)
    local temp = walkerMap[y][x]
    temp.id = walker.id
    temp.distance = walker.distance + 1
    temp.wallId = walker.wallId
    temp.parentDirection = parentDirection
end

---Pops and returns the next tile from the given list, updating the walker map if required
---@param openList OpenListTile[]
---@return Tile|{ x: integer, y: integer }
---@return Walker
local function GetNextTile(openList)
    ---@type OpenListTile
    local current = table.remove(openList, 1)
    local cWalker = walkerMap[current.y][current.x]

    if current.id ~= nil then
        cWalker.id = current.id
    end

    if current.wallId ~= nil then
        cWalker.wallId = current.wallId
    end

    local tile = tileMap[current.y][current.x]

    ---@type Tile | {x: integer, y: integer}
    local cTile = {
        x = current.x,
        y = current.y,
        distanceToUnpassable = tile.distanceToUnpassable,
        distance = tile.distance,
        spawn = tile.spawn,
        locked = tile.locked,
    }
    return cTile, cWalker
end

---Steps the given position in the given direction
---@param x integer
---@param y integer
---@param direction Direction
---@return integer x new x position
---@return integer y new y position
local function StepInDirection(x, y, direction)
    if direction == Direction.UP then
        y = y + 1
    elseif direction == Direction.DOWN then
        y = y - 1
    elseif direction == Direction.LEFT then
        x = x + 1
    elseif direction == Direction.RIGHT then
        x = x - 1
    end

    return x, y
end

---Converts a direction to a vector
---@param direction Direction
---@return {x: integer, y: integer}
local function DirectionToVector(direction)
    if direction == Direction.UP then
        return { x = 0, y = -1 }
    elseif direction == Direction.DOWN then
        return { x = 0, y = 1 }
    elseif direction == Direction.LEFT then
        return { x = -1, y = 0 }
    elseif direction == Direction.RIGHT then
        return { x = 1, y = 0 }
    else
        return { x = 0, y = 0 }
    end
end

local function Build()
    -- This isn't structly related to navigation and probably shouldn't be here,
    -- but it makes the calling code a lot simpler
    ---@type { spawnId: integer, goalId: integer }
    local configs = {}

    Entity.ForEachWithComponent("EnemySpawn", function(entity)
        local esComponent = Entity.Get(entity).EnemySpawn
        ---@type integer
        local spawnId = esComponent.id
        ---@type integer
        local goalId = esComponent.goalId

        if configs[spawnId] ~= nil and configs[spawnId] ~= goalId then
            print("Spawn id ", spawnId, " targets both ", goalId, " and ", configs[spawnId])
        else
            configs[spawnId] = goalId
        end
    end)

    --StopAllThreads()
    Navigation.Build(navigationState.tileSize)
    Navigation.ForEachTile(function(x, y, _)
        -- TODO: Add this back?
        --if not Navigation.IsWalkable(x, y) and not Navigation.IsNavGate(x, y) then
        --return
        --end

        ForEachNeighbour(x, y, function(n)
            if not Navigation.IsReachable(n.x, n.y) then
                Navigation.SetWall(x, y, n.direction)
            end
        end)
    end)

    --RegisterThread(function()
    local MAX_DISTANCE <const> = 999999
    local UNSET <const> = -1

    for spawnId, goalId in pairs(configs) do
        print("Building navigation for spawn ", spawnId, " and goal ", goalId)

        ---@type Tile[][]
        tileMap = {} -- Map for simple shorest distance from goal, in tiles

        ---@type Walker[][]
        walkerMap = {} -- Map for walking along walls

        -- Initialize
        for y = 1, Navigation.sizeY do
            walkerMap[y] = {}
            tileMap[y] = {}
            for x = 1, Navigation.sizeX do
                local distanceToUnpassable = DistanceToUnpassable(spawnId, goalId, x, y)
                -- All unreachable tiles are given the special id 0
                local id = distanceToUnpassable > 0 and UNSET or 0

                tileMap[y][x] = {
                    distance = MAX_DISTANCE,
                    distanceToUnpassable = distanceToUnpassable,
                    spawn = false,
                    locked = false,
                }
                walkerMap[y][x] = {
                    distance = MAX_DISTANCE,
                    parentDirection = Direction.NONE,
                    id = id,
                    wallId = UNSET,
                }
            end
        end

        ---@type OpenListTile[]
        local openList = {}

        -- Starting condition
        Navigation.ForEachTile(function(x, y, tile)
            x = x + 1
            y = y + 1

            if tile.type == Navigation.TileType.GOAL and tile.id == goalId and tileMap[y][x].distanceToUnpassable == 1 then
                walkerMap[y][x].wallId = 0
                walkerMap[y][x].distance = 0
                walkerMap[y][x].id = GetId()
                table.insert(openList, { x = x, y = y })
            elseif tile.type == Navigation.TileType.SPAWN and tile.id == spawnId then
                tileMap[y][x].spawn = true
            end
        end)

        if #openList == 0 then
            print("ERROR: No goal that is touching a wall found, navigation will not be built")
            return
        end

        ---@type {x: integer, y: integer, parentDirection: Direction}
        local gateList = {}

        local processedDistance = 1
        local anyAddedLastIter = false
        -- 9999 set to avoid an infinite loop, but this should be exited when no
        -- more tiles are available to explore
        while processedDistance < 9999 do
            -- Dijkstras
            while #openList > 0 do
                local cTile, cWalker = GetNextTile(openList)

                if not cTile.spawn and not cTile.locked then
                    ForEachNeighbour(cTile.x, cTile.y, function(n)
                        if Navigation.IsPassable(spawnId, goalId, n.x - 1, n.y - 1) then
                            if walkerMap[n.y][n.x].id == UNSET then
                                if IsAdjacentToWalker(spawnId, goalId, n.x, n.y, cWalker.wallId) then
                                    local valid = true
                                    if cWalker.wallId == 0 then
                                        if not HasSharedWall(spawnId, goalId, cTile.x, cTile.y, n.x, n.y, cTile.distanceToUnpassable) then
                                            valid = false
                                        end
                                    else
                                        ForEachPassableNeighbour(spawnId, goalId, n.x, n.y, function(nn)
                                            if walkerMap[nn.y][nn.x].id ~= cWalker.wallId and tileMap[nn.y][nn.x].distanceToUnpassable == cTile.distanceToUnpassable - 1 then
                                                valid = false
                                                return
                                            end
                                        end)
                                    end

                                    if valid then
                                        table.insert(openList, { x = n.x, y = n.y })
                                        SetNextWalkerStep(cWalker, n.x, n.y, n.opposite)
                                    end
                                end
                            end
                        else
                            if Navigation.IsNavGate(n.x - 1, n.y - 1) then
                                table.insert(gateList, { x = n.x, y = n.y })
                                SetNextWalkerStep(cWalker, n.x, n.y, n.opposite)
                            end
                        end
                    end)
                else
                    tileMap[cTile.y][cTile.x].locked = true

                    local parentDirection = cWalker.parentDirection
                    SetNextWalkerStep(cWalker, cTile.x, cTile.y, parentDirection)

                    local nextX, nextY = StepInDirection(cTile.x, cTile.y, parentDirection)
                    if Navigation.IsPassable(spawnId, goalId, nextX - 1, nextY - 1) and walkerMap[nextY][nextX].id == UNSET then
                        tileMap[nextY][nextX].locked = true

                        table.insert(openList, { x = nextX, y = nextY })
                        SetNextWalkerStep(cWalker, nextX, nextY, parentDirection)
                    end
                end
            end

            while #gateList > 0 do
                local cTile, cWalker = GetNextTile(gateList)

                ForEachNeighbour(cTile.x, cTile.y, function(n)
                    if walkerMap[n.y][n.x].id == UNSET then
                        if IsAdjacentToWalker(spawnId, goalId, n.x, n.y, cWalker.wallId) then
                            local valid = true
                            if cWalker.wallId == 0 then
                                if not HasSharedWall(spawnId, goalId, cTile.x, cTile.y, n.x, n.y, cTile.distanceToUnpassable) then
                                    valid = false
                                end
                            else
                                ForEachNeighbour(n.x, n.y, function(nn)
                                    if walkerMap[nn.y][nn.x].id ~= cWalker.wallId and tileMap[nn.y][nn.x].distanceToUnpassable == cTile.distanceToUnpassable - 1 then
                                        valid = false
                                        return
                                    end
                                end)
                            end

                            if valid then
                                table.insert(gateList, { x = n.x, y = n.y })
                                SetNextWalkerStep(cWalker, n.x, n.y, n.opposite)
                            end
                        end
                    end
                end)
            end

            -- For each walker, find the closest place where a new walker can be
            -- spawned during the next loop iteration
            ---@type {[integer]: {x: integer, y: integer, distance: integer}}
            local spawnMap = {}
            Navigation.ForEachTile(function(x, y, _) -- Is this faster than a simple nested for loop? Who knows
                x = x + 1
                y = y + 1

                if tileMap[y][x].distanceToUnpassable == processedDistance and not tileMap[y][x].spawn and walkerMap[y][x].id ~= UNSET then
                    local cWalker = walkerMap[y][x]

                    ForEachPassableNeighbour(spawnId, goalId, x, y, function(n)
                        if tileMap[n.y][n.x].distanceToUnpassable == processedDistance + 1 and walkerMap[n.y][n.x].id == UNSET then
                            if not spawnMap[cWalker.id] or spawnMap[cWalker.id].distance > cWalker.distance then
                                spawnMap[cWalker.id] = { distance = cWalker.distance, x = x, y = y }
                            end
                            return
                        end
                    end)
                end
            end)

            local anyAdded = false
            for wallId, spawnData in pairs(spawnMap) do
                anyAdded = true
                ForEachPassableNeighbour(spawnId, goalId, spawnData.x, spawnData.y, function(n)
                    -- Check id here as well just in case it is modified in a
                    -- previous iteration. It probably wouldn't matter if it was,
                    -- though...
                    if walkerMap[n.y][n.x].id == UNSET then
                        table.insert(openList, { x = n.x, y = n.y, wallId = wallId, id = GetId() })
                        walkerMap[n.y][n.x].wallId = wallId
                        walkerMap[n.y][n.x].distance = 0
                        walkerMap[n.y][n.x].parentDirection = n.opposite
                    end
                end)
            end

            -- It is possible that it takes multiple runs of the loop to step on all
            -- tiles with a distanceToUnpassable of processedDistance + 1. In that case
            -- the same distance will be processed again
            if not anyAdded then
                processedDistance = processedDistance + 1

                if not anyAddedLastIter then
                    break
                end
            end

            anyAddedLastIter = anyAdded
        end

        -- "raw" vector field without any smoothing
        local vectorField = {}
        for y = 1, Navigation.sizeY do
            vectorField[y] = {}
            for x = 1, Navigation.sizeX do
                if tileMap[y][x].distanceToUnpassable == 0 then
                    -- Make all unwalkable tiles that are adjacent to walkable tiles
                    -- push entities towards the walkable tile. This is in case
                    -- someone feels like wandering off the map
                    local toMap = { x = 0, y = 0 }
                    ForEachAdjacentPassable(spawnId, goalId, x, y, function(n)
                        toMap.x = toMap.x + n.x - x
                        toMap.y = toMap.y + n.y - y
                    end)

                    local len = math.sqrt(toMap.x * toMap.x + toMap.y * toMap.y)
                    -- Let's not track down nan issues again
                    if len ~= 0.0 then
                        toMap.x = toMap.x / len
                        toMap.y = toMap.y / len
                    end

                    vectorField[y][x] = toMap
                else
                    vectorField[y][x] = { x = 0, y = 0 }
                end
            end
        end

        -- Build vector field
        Navigation.ForEachTile(function(x, y, _)
            if not Navigation.IsReachable(x, y) then
                return
            end

            x = x + 1
            y = y + 1

            vectorField[y][x] = DirectionToVector(walkerMap[y][x].parentDirection)
        end)

        local vectorFieldId <const> = (spawnId << 16) | goalId

        if navigationState.smoothField then
            -- The only reason to not smooth the field is for troubleshooting
            local finalVectorField = {}
            for y = 1, Navigation.sizeY do
                finalVectorField[y] = {}
                for x = 1, Navigation.sizeX do
                    finalVectorField[y][x] = vectorField[y][x]
                end
            end

            -- Average over a 5x5 area
            -- TODO: Base area on tileSize instead of hard-coding it
            for y = 1, Navigation.sizeY do
                for x = 1, Navigation.sizeX do
                    if Navigation.IsPassable(spawnId, goalId, x - 1, y - 1) then
                        local average = { x = 0, y = 0 }
                        for yy = -2, 2 do
                            for xx = -2, 2 do
                                if Navigation.IsPassable(spawnId, goalId, x + xx - 1, y + yy - 1) then
                                    local tileDir = vectorField[y + yy][x + xx]
                                    average.x = average.x + tileDir.x
                                    average.y = average.y + tileDir.y
                                end
                            end
                        end

                        local len = math.sqrt(average.x * average.x + average.y * average.y)
                        if len ~= 0.0 then
                            average.x = average.x / len
                            average.y = average.y / len
                        end

                        finalVectorField[y][x] = average
                    end
                end
            end

            Navigation.SetVectorField(vectorFieldId, finalVectorField)
        else
            Navigation.SetVectorField(vectorFieldId, vectorField)
        end

        print("Navigation built for goal " ..
            goalId .. " and spawn " .. spawnId .. " with vectorFieldId " .. vectorFieldId)
    end
end

return {
    Build = Build,
    DistanceToPassable = DistanceToUnpassable
}
