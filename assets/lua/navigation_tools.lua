local function DistanceToWall(tileX, tileY)
    for radius = 1, 4 / Navigation.tileSize do
        for ry = -radius, radius do
            for rx = -radius, radius do
                local x = tileX + rx
                local y = tileY + ry

                if not Navigation.Reachable(x - 1, y - 1) then
                    if math.abs(rx) + math.abs(ry) <= radius then
                        return radius
                    end
                end
            end
        end
    end

    return 4 / Navigation.tileSize
end

local function Build()
    Navigation.Build(0.5)

    local tileMap = {}
    for y = 1, Navigation.sizeY do
        tileMap[y] = {}
        for x = 1, Navigation.sizeX do
            tileMap[y][x] = { distance = 9999 }
        end
    end

    local openList = {}

    Navigation.ForEachTile(function(x, y, tile)
        x = x + 1
        y = y + 1

        if tile.type == Navigation.TileType.GOAL then
            tileMap[y][x].distance = 0
            table.insert(openList, { x = x, y = y })
        end
    end)

    while #openList > 0 do
        local current = table.remove(openList, 1)

        local currentDistance = tileMap[current.y][current.x].distance
        local distanceToWall = DistanceToWall(current.x, current.y)

        local distance = currentDistance + 2 / (distanceToWall / (4 / Navigation.tileSize))

        local neighbours = {
            { x = current.x,     y = current.y - 1 },
            { x = current.x,     y = current.y + 1 },
            { x = current.x - 1, y = current.y },
            { x = current.x + 1, y = current.y },
        }
        for _, n in ipairs(neighbours) do
            if Navigation.Reachable(n.x - 1, n.y - 1) then
                if tileMap[n.y][n.x].distance > distance then
                    tileMap[n.y][n.x].distance = distance
                    table.insert(openList, { x = n.x, y = n.y })
                end
            end
        end
    end

    local vectorField = {}
    for y = 1, Navigation.sizeY do
        vectorField[y] = {}
        for x = 1, Navigation.sizeX do
            vectorField[y][x] = { x = 0, y = 0 }
        end
    end

    Navigation.ForEachTile(function(x, y, _)
        x = x + 1
        y = y + 1

        local upD = Navigation.Reachable(x - 1, y - 1 - 1) and tileMap[y - 1][x].distance or 9999
        local downD = Navigation.Reachable(x - 1, y - 1 + 1) and tileMap[y + 1][x].distance or 9999
        local leftD = Navigation.Reachable(x - 1 - 1, y - 1) and tileMap[y][x - 1].distance or 9999
        local rightD = Navigation.Reachable(x - 1 + 1, y - 1) and tileMap[y][x + 1].distance or 9999

        if upD == 9999 and downD == 9999 and leftD == 9999 and rightD == 9999 then
            return
        end

        local closestD = math.min(upD, math.min(downD, math.min(leftD, rightD)))
        local finalDirection = { x = 0, y = 0 }
        -- Basic
        -- if closestD == upD then
        --     finalDirection.y = finalDirection.y - 1
        -- elseif closestD == downD then
        --     finalDirection.y = finalDirection.y + 1
        -- elseif closestD == leftD then
        --     finalDirection.x = finalDirection.x - 1
        -- elseif closestD == rightD then
        --     finalDirection.x = finalDirection.x + 1
        -- end
        -- vectorField[y][x] = finalDirection

        -- Average
        local sets = 0
        if closestD == upD then
            finalDirection.y = finalDirection.y - 1
            sets = sets + 1
        end
        if closestD == downD then
            finalDirection.y = finalDirection.y + 1
            sets = sets + 1
        end
        if closestD == leftD then
            finalDirection.x = finalDirection.x - 1
            sets = sets + 1
        end
        if closestD == rightD then
            finalDirection.x = finalDirection.x + 1
            sets = sets + 1
        end

        vectorField[y][x] = { x = finalDirection.x / sets, y = finalDirection.y / sets }
    end)

    Navigation.SetVectorField(vectorField)

    print("Navigation built")
end

return {
    Build = Build,
}
