---@class Neighbour
---@field x integer
---@field y integer
---@field direction string
---@field opposite string

---@class OpenListTile
---@field x integer
---@field y integer
---@field wallId integer
---@field id integer

---@class Tile
---@field distance integer shortest distance from spawn
---@field distanceToWall integer distance from this tile to the closest wall
---@field spawn boolean whether or not this tile is an enemy spawn tile
---@field locked boolean if a walker crosses a spawn tile, it and all subsequent tiles will be locked

---@class Walker
---@field distance integer distance walked
---@field parentDirection string direction to the previous tile of this walker
---@field id integer this walker's id
---@field wallId integer the wall that this walker is following

if navigationState == nil then
    navigationState = {
        smoothField = true,
        tileSize = 0.5,
        fixDisconnected = false,
    }
end

---For the given tile, find the tile-distance to the nearest unreachable tile
---@param x integer
---@param y integer
---@return integer
local function DistanceToWall(x, y)
    if not Navigation.Reachable(x - 1, y - 1) then
        return 0
    end

    -- The radius of 100 is excessive, but a result is expected to be found a lot sooner
    for radius = 1, 100 do
        -- Expand search radius every iteration. Slow but is fast enough so far
        for ry = -radius, radius do
            for rx = -radius, radius do
                local x = x + rx
                local y = y + ry

                if not Navigation.Reachable(x - 1, y - 1) then
                    return radius
                end
            end
        end
    end

    return -1
end

---Given two adjacent tiles, check if they share a wall in any of the tiles around them
---@param x integer x position of first tile
---@param y integer y position of first tile
---@param ox integer x position of second tile
---@param oy integer y position of second tile
---@param searchRadius integer
---@return boolean
local function HasSharedWall(x, y, ox, oy, searchRadius)
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
            if not Navigation.Walkable(x + xx - 1, y + yy - 1) then
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
            if not Navigation.Walkable(ox + xx - 1, oy + yy - 1) then
                local val = map[yy + searchRadius + offsetY][xx + searchRadius + offsetX]

                if val == 1 then
                    return true
                end
            end
        end
    end

    return false
end

---For the given tile, get neighbours
---@param x integer
---@param y integer
---@return Neighbour[]
local function GetNeighbours(x, y)
    return {
        { x = x,     y = y - 1, direction = "up",    opposite = "down" },
        { x = x,     y = y + 1, direction = "down",  opposite = "up" },
        { x = x - 1, y = y,     direction = "left",  opposite = "right" },
        { x = x + 1, y = y,     direction = "right", opposite = "left" },
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

---Runs the given function for each cardinally adjacent neighbour that is walkable
---@param x integer
---@param y integer
---@param func fun(n: Neighbour)
local function ForEachWalkableNeighbour(x, y, func)
    for _, n in ipairs(GetNeighbours(x, y)) do
        if Navigation.Walkable(n.x - 1, n.y - 1) then
            func(n)
        end
    end
end

---Runs the given function for _all adjacent_ neighbours that are walkable
---@param x integer
---@param y integer
---@param func fun(n: {x: integer, y: integer})
local function ForEachAdjacentWalkable(x, y, func)
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
        if Navigation.Walkable(n.x - 1, n.y - 1) then
            func(n)
        end
    end
end

---Given a tile, checks whether or not it is adjacent to the walker with the given wall id
---@param x integer
---@param y integer
---@param walkerWallId integer
---@return boolean
function IsAdjacentToWalker(x, y, walkerWallId)
    for ry = -1, 1, 1 do
        for rx = -1, 1, 1 do
            local x = x + rx
            local y = y + ry

            if Navigation.Walkable(x - 1, y - 1) then
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
---@param parentDirection string
local function SetNextWalkerStep(walker, x, y, parentDirection)
    local temp = walkerMap[y][x]
    temp.id = walker.id
    temp.distance = walker.distance + 1
    temp.wallId = walker.wallId
    temp.parentDirection = parentDirection
end

---Pops and returns the next tile from the given list, updating the walker map if required
---@param OpenListTile[] openList
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
        distanceToWall = tile.distanceToWall,
        distance = tile.distance,
        spawn = tile.spawn,
        locked = tile.locked,
    }
    return cTile, cWalker
end

local function Build()
    StopAllThreads()
    Navigation.Build(navigationState.tileSize)

    --RegisterThread(function()
    local MAX_DISTANCE <const> = 999999
    local UNSET <const> = -1

    ---@type Tile[][]
    tileMap = {} -- Map for simple shorest distance from goal, in tiles

    ---@type Walker[][]
    walkerMap = {} -- Map for walking along walls

    -- Initialize
    for y = 1, Navigation.sizeY do
        walkerMap[y] = {}
        tileMap[y] = {}
        for x = 1, Navigation.sizeX do
            local distanceToWall = DistanceToWall(x, y)
            -- All unreachable tiles are given the special id 0
            local id = distanceToWall > 0 and UNSET or 0

            tileMap[y][x] = {
                distance = MAX_DISTANCE,
                distanceToWall = distanceToWall,
                spawn = false,
                locked = false,
            }
            walkerMap[y][x] = {
                distance = MAX_DISTANCE,
                parentDirection = "none",
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

        if tile.type == Navigation.TileType.GOAL and tileMap[y][x].distanceToWall == 1 then
            walkerMap[y][x].wallId = 0
            walkerMap[y][x].distance = 0
            walkerMap[y][x].id = GetId()
            table.insert(openList, { x = x, y = y })
        elseif tile.type == Navigation.TileType.SPAWN then
            tileMap[y][x].spawn = true
        end
    end)

    if #openList == 0 then
        print("ERROR: No goal that is touching a wall found, navigation will not be built")
        return
    end

    local walkedDistance = 1
    local anyAddedLastIter = false
    -- 9999 set to avoid an infinite loop, but this should be exited when no
    -- more tiles are available to exporse
    while walkedDistance < 9999 do
        -- Dijkstras
        while #openList > 0 do
            local cTile, cWalker = GetNextTile(openList)

            if not cTile.spawn and not cTile.locked then
                ForEachWalkableNeighbour(cTile.x, cTile.y, function(n)
                    if walkerMap[n.y][n.x].id == UNSET then
                        if IsAdjacentToWalker(n.x, n.y, cWalker.wallId) then
                            local valid = true
                            if cWalker.wallId == 0 then
                                if not HasSharedWall(cTile.x, cTile.y, n.x, n.y, cTile.distanceToWall) then
                                    valid = false
                                end
                            else
                                ForEachWalkableNeighbour(n.x, n.y, function(nn)
                                    if walkerMap[nn.y][nn.x].id ~= cWalker.wallId and tileMap[nn.y][nn.x].distanceToWall == cTile.distanceToWall - 1 then
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
                end)

                --coroutine.yield(10)
            else
                tileMap[cTile.y][cTile.x].locked = true

                local parentDirection = cWalker.parentDirection
                SetNextWalkerStep(cWalker, cTile.x, cTile.y, cWalker.parentDirection)

                local next = { x = cTile.x, y = cTile.y }
                if parentDirection == "up" then
                    next.y = next.y + 1
                elseif parentDirection == "down" then
                    next.y = next.y - 1
                elseif parentDirection == "left" then
                    next.x = next.x + 1
                elseif parentDirection == "right" then
                    next.x = next.x - 1
                end

                if Navigation.Walkable(next.x - 1, next.y - 1) and walkerMap[next.y][next.x].id == UNSET then
                    tileMap[next.y][next.x].locked = true

                    table.insert(openList, { x = next.x, y = next.y })
                    SetNextWalkerStep(cWalker, next.x, next.y, parentDirection)
                end
            end
        end

        local spawnMap = {}
        for y = 1, Navigation.sizeY do
            for x = 1, Navigation.sizeX do
                if tileMap[y][x].distanceToWall == walkedDistance and not tileMap[y][x].spawn then
                    local cId = walkerMap[y][x].id

                    if cId ~= UNSET then
                        local valid = false
                        ForEachWalkableNeighbour(x, y, function(n)
                            if tileMap[n.y][n.x].distanceToWall == walkedDistance + 1 and walkerMap[n.y][n.x].id == UNSET then
                                valid = true
                                return
                            end
                        end)

                        if valid then
                            if not spawnMap[cId] then
                                spawnMap[cId] = { distance = walkerMap[y][x].distance, x = x, y = y }
                            elseif spawnMap[cId].distance > walkerMap[y][x].distance then
                                spawnMap[cId] = { distance = walkerMap[y][x].distance, x = x, y = y }
                            end
                        end
                    end
                end
            end
        end

        local anyAdded = false
        for wallId, spawnData in pairs(spawnMap) do
            anyAdded = true
            ForEachWalkableNeighbour(spawnData.x, spawnData.y, function(n)
                if tileMap[n.y][n.x].distanceToWall == walkedDistance + 1 and walkerMap[n.y][n.x].id == UNSET then
                    table.insert(openList, { x = n.x, y = n.y, wallId = wallId, id = GetId() })
                    walkerMap[n.y][n.x].wallId = wallId
                    walkerMap[n.y][n.x].distance = 0
                    walkerMap[n.y][n.x].parentDirection = n.opposite
                end
            end)
        end

        if not anyAdded then
            walkedDistance = walkedDistance + 1

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
            if tileMap[y][x].distanceToWall == 0 then
                local toMap = { x = 0, y = 0 }
                ForEachAdjacentWalkable(x, y, function(n)
                    toMap.x = toMap.x + n.x - x
                    toMap.y = toMap.y + n.y - y
                end)

                local len = math.sqrt(toMap.x * toMap.x + toMap.y * toMap.y)
                if len == 0.0 then
                    len = 1
                end
                toMap.x = toMap.x / len
                toMap.y = toMap.y / len

                vectorField[y][x] = toMap
            else
                vectorField[y][x] = { x = 0, y = 0 }
            end
        end
    end

    -- Build vector field
    Navigation.ForEachTile(function(x, y, _)
        if not Navigation.Walkable(x, y) then
            return
        end

        local current = { x = x + 1, y = y + 1 }

        local parentDirection = walkerMap[current.y][current.x].parentDirection
        if parentDirection == "up" then
            vectorField[current.y][current.x] = { x = 0, y = -1 }
        elseif parentDirection == "down" then
            vectorField[current.y][current.x] = { x = 0, y = 1 }
        elseif parentDirection == "left" then
            vectorField[current.y][current.x] = { x = -1, y = 0 }
        elseif parentDirection == "right" then
            vectorField[current.y][current.x] = { x = 1, y = 0 }
        end

        ForEachNeighbour(current.x, current.y, function(n)
            if not Navigation.Walkable(n.x - 1, n.y - 1) then
                if n.direction == "up" then
                    Navigation.SetWall(current.x - 1, current.y - 1, Navigation.TileSide.TOP)
                elseif n.direction == "down" then
                    Navigation.SetWall(current.x - 1, current.y - 1, Navigation.TileSide.BOTTOM)
                elseif n.direction == "left" then
                    Navigation.SetWall(current.x - 1, current.y - 1, Navigation.TileSide.LEFT)
                elseif n.direction == "right" then
                    Navigation.SetWall(current.x - 1, current.y - 1, Navigation.TileSide.RIGHT)
                end
            end
        end)
    end)

    -- Vector field with some smoothing
    local finalVectorField = {}
    if navigationState.smoothField then
        for y = 1, Navigation.sizeY do
            finalVectorField[y] = {}
            for x = 1, Navigation.sizeX do
                finalVectorField[y][x] = vectorField[y][x]
            end
        end

        -- Average over a 3x3 area
        for y = 1, Navigation.sizeY do
            for x = 1, Navigation.sizeX do
                if Navigation.Walkable(x - 1, y - 1) then
                    local average = { x = 0, y = 0 }
                    for yy = -2, 2 do
                        for xx = -2, 2 do
                            if Navigation.Walkable(x + xx - 1, y + yy - 1) then
                                local tileDir = vectorField[y + yy][x + xx]
                                average.x = average.x + tileDir.x
                                average.y = average.y + tileDir.y
                            end
                        end
                    end

                    local len = math.sqrt(average.x * average.x + average.y * average.y)
                    if len == 0.0 then
                        len = 1
                    end
                    average.x = average.x / len
                    average.y = average.y / len

                    finalVectorField[y][x] = average
                end
            end
        end

        Navigation.SetVectorField(finalVectorField)
    else
        Navigation.SetVectorField(vectorField)
    end

    print("Navigation built")
    --end)
end

return {
    Build = Build,
    DistanceToWall = DistanceToWall
}
