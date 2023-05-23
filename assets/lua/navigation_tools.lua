if navigationState == nil then
    navigationState = {
        smoothField = true,
        tileSize = 0.5,
        fixDisconnected = false,
    }
end

local function DistanceToWall(tileX, tileY)
    if not Navigation.Reachable(tileX - 1, tileY - 1) then
        return 0
    end

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

local function IsSameWall(x, y, ox, oy, distanceToWall)
    local map = {}
    for y = 0, distanceToWall * 2 + 1 + 1 do
        map[y] = {}
        for x = 0, distanceToWall * 2 + 1 + 1 do
            map[y][x] = 0
        end
    end

    local offsetX = x > ox and 1 or 0
    local offsetY = y > oy and 1 or 0

    for yy = -distanceToWall, distanceToWall do
        for xx = -distanceToWall, distanceToWall do
            if not Navigation.Walkable(x + xx - 1, y + yy - 1) then
                local val = map[yy + distanceToWall + offsetY][xx + distanceToWall + offsetX]
                map[yy + distanceToWall + offsetY][xx + distanceToWall + offsetX] = val + 1
            end
        end
    end

    local offsetX = ox > x and 1 or 0
    local offsetY = oy > y and 1 or 0

    local maxValue = 0

    for yy = -distanceToWall, distanceToWall do
        for xx = -distanceToWall, distanceToWall do
            if not Navigation.Walkable(ox + xx - 1, oy + yy - 1) then
                local val = map[yy + distanceToWall + offsetY][xx + distanceToWall + offsetX]
                --map[yy + distanceToWall + offsetY][xx + distanceToWall + offsetX] = val + 1
                maxValue = math.max(maxValue, val + 1)
            end
        end
    end

    return maxValue == 2
end

function DirectionToWall(x, y, distanceToWall)
    local dirToCurrentWall = { x = 0, y = 0 }
    local sum = 0
    if not Navigation.Walkable(x - 1, y - 1 - distanceToWall) then
        dirToCurrentWall.y = dirToCurrentWall.y - 1
        sum = sum + 1
    end
    if not Navigation.Walkable(x - 1, y - 1 + distanceToWall) then
        dirToCurrentWall.y = dirToCurrentWall.y + 1
        sum = sum + 1
    end
    if not Navigation.Walkable(x - 1 - distanceToWall, y - 1) then
        dirToCurrentWall.x = dirToCurrentWall.x - 1
        sum = sum + 1
    end
    if not Navigation.Walkable(x - 1 + distanceToWall, y - 1) then
        dirToCurrentWall.x = dirToCurrentWall.x + 1
        sum = sum + 1
    end

    if not Navigation.Walkable(x - 1 - distanceToWall, y - 1 - distanceToWall) then
        dirToCurrentWall.x = dirToCurrentWall.x - 1
        dirToCurrentWall.y = dirToCurrentWall.y - 1
        sum = sum + 1
    end
    if not Navigation.Walkable(x - 1 + distanceToWall, y - 1 - distanceToWall) then
        dirToCurrentWall.x = dirToCurrentWall.x + 1
        dirToCurrentWall.y = dirToCurrentWall.y - 1
        sum = sum + 1
    end
    if not Navigation.Walkable(x - 1 - distanceToWall, y - 1 + distanceToWall) then
        dirToCurrentWall.x = dirToCurrentWall.x - 1
        dirToCurrentWall.y = dirToCurrentWall.y + 1
        sum = sum + 1
    end
    if not Navigation.Walkable(x - 1 + distanceToWall, y - 1 + distanceToWall) then
        dirToCurrentWall.x = dirToCurrentWall.x + 1
        dirToCurrentWall.y = dirToCurrentWall.y + 1
        sum = sum + 1
    end

    if sum == 0 then
        return { x = 0, y = 0 }
    end

    dirToCurrentWall.x = dirToCurrentWall.x / sum
    dirToCurrentWall.y = dirToCurrentWall.y / sum
    return dirToCurrentWall
end

local function ForEachNeighbour(x, y, func)
    local neighbours = {
        { x = x,     y = y - 1, direction = "up",    opposite = "down" },
        { x = x,     y = y + 1, direction = "down",  opposite = "up" },
        { x = x - 1, y = y,     direction = "left",  opposite = "right" },
        { x = x + 1, y = y,     direction = "right", opposite = "left" },
    }
    for _, n in ipairs(neighbours) do
        func(n)
    end
end

local function ForEachWalkableNeighbour(x, y, func)
    local neighbours = {
        { x = x,     y = y - 1, direction = "up",    opposite = "down" },
        { x = x,     y = y + 1, direction = "down",  opposite = "up" },
        { x = x - 1, y = y,     direction = "left",  opposite = "right" },
        { x = x + 1, y = y,     direction = "right", opposite = "left" },
    }
    for _, n in ipairs(neighbours) do
        if Navigation.Walkable(n.x - 1, n.y - 1) then
            func(n)
        end
    end
end

local function ForEachAdjacent(x, y, func)
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

function IsAdjacent(cx, cy, wallId)
    for ry = -1, 1, 1 do
        for rx = -1, 1, 1 do
            local x = cx + rx
            local y = cy + ry

            if Navigation.Walkable(x - 1, y - 1) then
                if wallMap[y][x].id == wallId then
                    return true
                end
            elseif wallId == 0 then
                return true -- Special case
            end
        end
    end

    return false
end

local function Build()
    StopAllThreads()
    Navigation.Build(navigationState.tileSize)

    --RegisterThread(function()
    local idCounter = 1

    wallMap = {} -- Map for walking along walls
    tileMap = {} -- Map for simple shorest distance from goal, in tiles

    -- Initialize
    for y = 1, Navigation.sizeY do
        wallMap[y] = {}
        tileMap[y] = {}
        for x = 1, Navigation.sizeX do
            local distanceToWall = DistanceToWall(x, y)

            local id = distanceToWall > 0 and -1 or 0

            tileMap[y][x] = {
                distance = 9999,
                distanceToWall = distanceToWall,
                spawn = false,
                locked = false,
            }
            wallMap[y][x] = {
                distance = 9999,
                parentDirection = "none",
                id = id,
                wallId = -1,
            }
        end
    end


    -- Add all goals to openList to start calculating distance using Dijkstras
    -- with constant cost
    local openList = {}

    Navigation.ForEachTile(function(x, y, tile)
        x = x + 1
        y = y + 1

        if tile.type == Navigation.TileType.GOAL and tileMap[y][x].distanceToWall == 1 then
            wallMap[y][x].wallId = 0
            wallMap[y][x].distance = 0
            wallMap[y][x].id = idCounter
            idCounter = idCounter + 1
            table.insert(openList, { x = x, y = y })
        elseif tile.type == Navigation.TileType.SPAWN then
            tileMap[y][x].spawn = true
        end
    end)

    if #openList == 0 then
        print("ERROR: No goal that is touching a wall found, navigation will not be built")
        return
    end

    local iter = 2
    while iter < 100 do
        -- Dijkstras
        while #openList > 0 do
            local current = table.remove(openList, 1)

            local cDistance = wallMap[current.y][current.x].distance
            local cId
            if current.id ~= nil then
                cId = current.id
                wallMap[current.y][current.x].id = cId
            else
                cId = wallMap[current.y][current.x].id
            end
            local cWallId
            if current.wallId ~= nil then
                cWallId = current.wallId
                wallMap[current.y][current.x].wallId = cWallId
            else
                cWallId = wallMap[current.y][current.x].wallId
            end
            local cDistanceToWall = tileMap[current.y][current.x].distanceToWall


            if not tileMap[current.y][current.x].spawn and not tileMap[current.y][current.x].locked then
                ForEachWalkableNeighbour(current.x, current.y, function(n)
                    if wallMap[n.y][n.x].id == -1 then
                        if IsAdjacent(n.x, n.y, cWallId) then
                            local valid = true
                            if cWallId == 0 then
                                if not IsSameWall(current.x, current.y, n.x, n.y, cDistanceToWall) then
                                    valid = false
                                end
                            else
                                ForEachWalkableNeighbour(n.x, n.y, function(nn)
                                    if wallMap[nn.y][nn.x].id ~= wallMap[current.y][current.x].wallId and tileMap[nn.y][nn.x].distanceToWall == tileMap[n.y][n.x].distanceToWall - 1 then
                                        valid = false
                                        return
                                    end
                                end)
                            end

                            if valid then
                                table.insert(openList, { x = n.x, y = n.y })
                                wallMap[n.y][n.x].id = cId
                                wallMap[n.y][n.x].distance = cDistance + 1
                                wallMap[n.y][n.x].wallId = cWallId
                                wallMap[n.y][n.x].parentDirection = n.opposite
                            end
                        end
                    end
                end)

                --coroutine.yield(10)
            else
                local parentDirection = wallMap[current.y][current.x].parentDirection

                tileMap[current.y][current.x].locked = true

                wallMap[current.y][current.x].id = cId
                wallMap[current.y][current.x].distance = cDistance + 1
                wallMap[current.y][current.x].wallId = cWallId
                wallMap[current.y][current.x].parentDirection = parentDirection

                local next = { x = current.x, y = current.y }
                if parentDirection == "up" then
                    next.y = next.y + 1
                elseif parentDirection == "down" then
                    next.y = next.y - 1
                elseif parentDirection == "left" then
                    next.x = next.x + 1
                elseif parentDirection == "right" then
                    next.x = next.x - 1
                end

                if Navigation.Walkable(next.x - 1, next.y - 1) and wallMap[next.y][next.x].id == -1 then
                    table.insert(openList, { x = next.x, y = next.y })
                    wallMap[next.y][next.x].id = cId
                    wallMap[next.y][next.x].distance = cDistance + 1
                    wallMap[next.y][next.x].wallId = cWallId
                    wallMap[next.y][next.x].parentDirection = parentDirection

                    tileMap[next.y][next.x].locked = true
                end
            end
        end

        local spawnMap = {}
        for y = 1, Navigation.sizeY do
            for x = 1, Navigation.sizeX do
                if tileMap[y][x].distanceToWall == iter - 1 and not tileMap[y][x].spawn then
                    local cId = wallMap[y][x].id

                    if cId ~= -1 then
                        local valid = false
                        ForEachWalkableNeighbour(x, y, function(n)
                            if tileMap[n.y][n.x].distanceToWall == iter and wallMap[n.y][n.x].id == -1 then
                                valid = true
                                return
                            end
                        end)

                        if valid then
                            if not spawnMap[cId] then
                                spawnMap[cId] = { distance = wallMap[y][x].distance, x = x, y = y }
                            elseif spawnMap[cId].distance > wallMap[y][x].distance then
                                spawnMap[cId] = { distance = wallMap[y][x].distance, x = x, y = y }
                            end
                        end
                    end
                end
            end
        end

        local anyAdded = false
        for wallId, spawnData in pairs(spawnMap) do
            -- print("Spawning iter ", iter)
            anyAdded = true
            ForEachWalkableNeighbour(spawnData.x, spawnData.y, function(n)
                if tileMap[n.y][n.x].distanceToWall == iter and wallMap[n.y][n.x].id == -1 then
                    -- print("   ", n.x, "x", n.y)
                    -- print("   ", idCounter)
                    -- print("   ", wallId)
                    table.insert(openList, { x = n.x, y = n.y, wallId = wallId, id = idCounter })
                    wallMap[n.y][n.x].wallId = wallId
                    wallMap[n.y][n.x].distance = 0
                    wallMap[n.y][n.x].parentDirection = n.opposite
                    idCounter = idCounter + 1
                end
            end)
        end

        if not anyAdded then
            iter = iter + 1

            -- TODO: exit loop eventually
        end
    end

    -- "raw" vector field without any smoothing
    local vectorField = {}
    for y = 1, Navigation.sizeY do
        vectorField[y] = {}
        for x = 1, Navigation.sizeX do
            if tileMap[y][x].distanceToWall == 0 then
                local toMap = { x = 0, y = 0 }
                ForEachAdjacent(x, y, function(n)
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
