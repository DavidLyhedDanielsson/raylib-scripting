local function DistanceToWall(tileX, tileY)
    for radius = 1, 100 do
        for ry = -radius, radius do
            for rx = -radius, radius do
                local x = tileX + rx
                local y = tileY + ry

                if not Navigation.Reachable(x - 1, y - 1) then
                    -- Manhattan value
                    -- if math.abs(rx) + math.abs(ry) <= radius then
                    --     return radius
                    -- end

                    return radius
                end
            end
        end
    end

    return -1
end

function DirectionToWall(x, y, distanceToWall)
    local dirToCurrentWall = { x = 0, y = 0 }
    local sum = 0
    if not Navigation.Reachable(x - 1, y - 1 - distanceToWall) then
        dirToCurrentWall.y = dirToCurrentWall.y - 1
        sum = sum + 1
    end
    if not Navigation.Reachable(x - 1, y - 1 + distanceToWall) then
        dirToCurrentWall.y = dirToCurrentWall.y + 1
        sum = sum + 1
    end
    if not Navigation.Reachable(x - 1 - distanceToWall, y - 1) then
        dirToCurrentWall.x = dirToCurrentWall.x - 1
        sum = sum + 1
    end
    if not Navigation.Reachable(x - 1 + distanceToWall, y - 1) then
        dirToCurrentWall.x = dirToCurrentWall.x + 1
        sum = sum + 1
    end

    if not Navigation.Reachable(x - 1 - distanceToWall, y - 1 - distanceToWall) then
        dirToCurrentWall.x = dirToCurrentWall.x - 1
        dirToCurrentWall.y = dirToCurrentWall.y - 1
        sum = sum + 1
    end
    if not Navigation.Reachable(x - 1 + distanceToWall, y - 1 - distanceToWall) then
        dirToCurrentWall.x = dirToCurrentWall.x + 1
        dirToCurrentWall.y = dirToCurrentWall.y - 1
        sum = sum + 1
    end
    if not Navigation.Reachable(x - 1 - distanceToWall, y - 1 + distanceToWall) then
        dirToCurrentWall.x = dirToCurrentWall.x - 1
        dirToCurrentWall.y = dirToCurrentWall.y + 1
        sum = sum + 1
    end
    if not Navigation.Reachable(x - 1 + distanceToWall, y - 1 + distanceToWall) then
        dirToCurrentWall.x = dirToCurrentWall.x + 1
        dirToCurrentWall.y = dirToCurrentWall.y + 1
        sum = sum + 1
    end

    dirToCurrentWall.x = dirToCurrentWall.x / sum
    dirToCurrentWall.y = dirToCurrentWall.y / sum
    return dirToCurrentWall
end

local function Build()
    Navigation.Build(1.0)

    local idCounter = 1
    local maxDistanceToWall = -1

    wallMap = {} -- Map for walking along walls
    tileMap = {} -- Map for simple shorest distance from goal, in tiles

    -- Initialize
    for y = 1, Navigation.sizeY do
        wallMap[y] = {}
        tileMap[y] = {}
        for x = 1, Navigation.sizeX do
            local distanceToWall = DistanceToWall(x, y)
            maxDistanceToWall = math.max(distanceToWall, maxDistanceToWall)

            tileMap[y][x] = { distance = 9999, distanceToWall = distanceToWall }

            wallMap[y][x] = {
                distance = 9999,
                parentDirection = "none",
                id = -1,
            }
        end
    end

    -- Add all goals to openList to start calculating distance using Dijkstras
    -- with constant cost
    local openList = {}
    Navigation.ForEachTile(function(x, y, tile)
        x = x + 1
        y = y + 1

        if tile.type == Navigation.TileType.GOAL then
            tileMap[y][x].distance = 0
            table.insert(openList, { x = x, y = y })
        end
    end)

    -- Max distance from goal, used later to walk along walls
    local maxDistance = 0

    while #openList > 0 do
        local current = table.remove(openList, 1)
        local currentDistance = tileMap[current.y][current.x].distance

        local neighbours = {
            { x = current.x,     y = current.y - 1, direction = "up",    opposite = "down" },
            { x = current.x,     y = current.y + 1, direction = "down",  opposite = "up" },
            { x = current.x - 1, y = current.y,     direction = "left",  opposite = "right" },
            { x = current.x + 1, y = current.y,     direction = "right", opposite = "left" },
        }
        for _, n in ipairs(neighbours) do
            if Navigation.Reachable(n.x - 1, n.y - 1) and tileMap[n.y][n.x].distance > currentDistance + 1 then
                table.insert(openList, { x = n.x, y = n.y })
                tileMap[n.y][n.x].distance = currentDistance + 1
                maxDistance = math.max(maxDistance, currentDistance + 1)
            end
        end
    end

    -- Walk along walls. Spawn walkers at a distance of `i` from the goal, walk
    -- all of these along whichever wall they happen to be close to. At some
    -- point all tiles have been visited and all walls have been walked along
    openList = {}
    for i = 0, maxDistance do
        local anyLeft = false
        Navigation.ForEachTile(function(x, y, _)
            x = x + 1
            y = y + 1

            if wallMap[y][x].id == -1 then
                -- There is at least one tile that hasn't been walked yet,
                -- otherwise make a quick exit.
                anyLeft = true

                if tileMap[y][x].distance == i then
                    wallMap[y][x].distance = 0
                    wallMap[y][x].id = idCounter
                    idCounter = idCounter + 1
                    table.insert(openList, { x = x, y = y })
                end
            end
        end)

        -- This usually saves about 90% of the iterations of this loop. Nice!
        if not anyLeft then
            break
        end

        -- Dijkstras again, but walk along walls
        while #openList > 0 do
            local current = table.remove(openList, 1)

            local cDistanceToWall = tileMap[current.y][current.x].distanceToWall
            local currentDistance = wallMap[current.y][current.x].distance
            local currentId = wallMap[current.y][current.x].id
            local currentWallDir = DirectionToWall(current.x, current.y, cDistanceToWall)

            local neighbours = {
                { x = current.x,     y = current.y - 1, direction = "up",    opposite = "down" },
                { x = current.x,     y = current.y + 1, direction = "down",  opposite = "up" },
                { x = current.x - 1, y = current.y,     direction = "left",  opposite = "right" },
                { x = current.x + 1, y = current.y,     direction = "right", opposite = "left" },
            }
            for _, n in ipairs(neighbours) do
                if Navigation.Reachable(n.x - 1, n.y - 1) then
                    local nDistanceToWall = tileMap[n.y][n.x].distanceToWall
                    local nWallDir = DirectionToWall(n.x, n.y, nDistanceToWall)
                    local wallDot = currentWallDir.x * nWallDir.x + currentWallDir.y * nWallDir.y

                    -- Only consider nodes that have the same distance from a
                    -- wall as the current node. Also, only consider nodes where
                    -- the wall is on the same side as the current node
                    if nDistanceToWall == cDistanceToWall and wallDot >= 0 then
                        if wallMap[n.y][n.x].distance > currentDistance + 1 then
                            table.insert(openList, { x = n.x, y = n.y })
                            wallMap[n.y][n.x].id = currentId
                            wallMap[n.y][n.x].distance = currentDistance + 1
                            wallMap[n.y][n.x].parentDirection = n.opposite
                        end
                    end
                end
            end
        end
    end

    -- When a walker is spawned at a random position, it won't have a
    -- parentDirection, so set it
    Navigation.ForEachTile(function(x, y, _)
        local current = { x = x + 1, y = y + 1 }

        if wallMap[current.y][current.x].parentDirection == "none" then
            local lowestId = wallMap[current.y][current.x].id
            local bestDirection = nil

            local neighbours = {
                { x = current.x,     y = current.y - 1, direction = "up",    opposite = "down" },
                { x = current.x,     y = current.y + 1, direction = "down",  opposite = "up" },
                { x = current.x - 1, y = current.y,     direction = "left",  opposite = "right" },
                { x = current.x + 1, y = current.y,     direction = "right", opposite = "left" },
            }
            for _, n in ipairs(neighbours) do
                if Navigation.Reachable(n.x - 1, n.y - 1) then
                    if wallMap[n.y][n.x].id < lowestId then
                        lowestId = wallMap[n.y][n.x].id
                        bestDirection = n.direction
                    end
                end
            end

            if lowestId == wallMap[current.y][current.x].lowestId then
                print(":(")
            else
                wallMap[current.y][current.x].parentDirection = bestDirection
            end
        end
    end)


    -- "raw" vector field without any smoothing
    local vectorField = {}
    for y = 1, Navigation.sizeY do
        vectorField[y] = {}
        for x = 1, Navigation.sizeX do
            vectorField[y][x] = { x = 0, y = 0 }
        end
    end

    -- Build vector field
    Navigation.ForEachTile(function(x, y, _)
        local current = { x = x + 1, y = y + 1 }

        local parentDirection = wallMap[current.y][current.x].parentDirection
        if parentDirection == "up" then
            vectorField[current.y][current.x] = { x = 0, y = -1 }
        elseif parentDirection == "down" then
            vectorField[current.y][current.x] = { x = 0, y = 1 }
        elseif parentDirection == "left" then
            vectorField[current.y][current.x] = { x = -1, y = 0 }
        elseif parentDirection == "right" then
            vectorField[current.y][current.x] = { x = 1, y = 0 }
        end
    end)


    -- Vector field with some smoothing
    local finalVectorField = {}
    for y = 1, Navigation.sizeY do
        finalVectorField[y] = {}
        for x = 1, Navigation.sizeX do
            finalVectorField[y][x] = { x = 0, y = 0 }
        end
    end

    -- Average over a 3x3 area
    for y = 1, Navigation.sizeY do
        for x = 1, Navigation.sizeX do
            local average = { x = 0, y = 0 }
            for yy = -1, 1 do
                for xx = -1, 1 do
                    if Navigation.Reachable(x + xx - 1, y + yy - 1) then
                        local tileDir = vectorField[y + yy][x + xx]
                        average.x = average.x + tileDir.x
                        average.y = average.y + tileDir.y
                    end
                end
            end
            local len = math.sqrt(average.x * average.x + average.y * average.y)
            average.x = average.x / len
            average.y = average.y / len

            finalVectorField[y][x] = average
        end
    end

    Navigation.SetVectorField(finalVectorField)
    --Navigation.SetVectorField(vectorField)

    print("Navigation built")
end

return {
    Build = Build,
    DistanceToWall = DistanceToWall
}
