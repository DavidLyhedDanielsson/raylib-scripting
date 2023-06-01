-- in case input_output is modified, it needs to be nulled since this file is
-- ran ecah frame
package.loaded["level"] = nil
package.loaded["navigation_tools"] = nil
package.loaded["entity_tools"] = nil
package.loaded["editor.entity_window"] = nil
package.loaded["editor.main_menu_bar"] = nil

local Level = require("level")
local NavigationTools = require("navigation_tools")
local EntityTools = require("entity_tools")
local Gui = {
    Entity = require("editor.entity_window"),
    Menu = require("editor.main_menu_bar"),
}

function init()
    setup = true

    selectedEntities = {}
    newSelectedEntity = nil
    usingGizmo = false

    enemySpawns = {}
    enemyGoals = {}

    cooldown = 0
    placeTrapMode = false
    placeFloorMode = false

    PlaceFloorType = {
        NORMAL = 0,
        SPAWN = 1,
        GOAL = 2,
        UNWALKABLE = 3,
    }
    placeFloorType = PlaceFloorType.NORMAL

    if StartLevel then
        Level.LoadLevel(StartLevel or "level1")
        NavigationTools.Build()
    end
end

function raylib3D()
    for entity in pairs(selectedEntities) do
        if not Entity.IsValid(entity) then
            selectedEntities[entity] = nil
        end
    end

    for entity in pairs(selectedEntities) do
        Raylib.DrawEntityBoundingBox(entity)
    end

    if placeFloorMode then
        if Raylib.IsKeyDown(Raylib.Key.LEFT_SHIFT) then
            Raylib.DrawGrid(500, 1)
        else
            Raylib.DrawGrid(500, 2)
        end
    end
end

function SpawnDarts(transformTarget)
    local offsets = {
        { x = 0.492,  y = 1.286, z = 0.05 },
        { x = -0.492, y = 1.286, z = -0.05 },
        { x = 0,      y = 0.787, z = 0 },
        { x = 0.492,  y = 0.286, z = 0.03 },
        { x = -0.492, y = 0.286, z = 0.07 },
    }

    for i = 1, 5 do
        local startPosition = { x = offsets[i].x, y = offsets[i].y, z = offsets[i].z }
        local velocity = 20

        local entity = Entity.Create()
        EntityTools.AddComponentOrPrintError("Render", entity, { assetName = "Dart" })
        EntityTools.AddComponentOrPrintError("Transform", entity,
            { position = startPosition, rotation = { x = 0, y = 0, z = 0 } })
        EntityTools.AddComponentOrPrintError("Projectile", entity, { damage = 1 })

        Entity.TransformTo(entity, transformTarget)
        local transformedPosition = Entity.Get(entity).Transform.position
        EntityTools.AddComponentOrPrintError("MaxRange", entity, { maxDistance = 5, distanceFrom = transformedPosition })

        local forward = Entity.GetForwardVector(entity);
        EntityTools.AddComponentOrPrintError("Velocity", entity,
            { x = forward.x * velocity, y = forward.y * velocity, z = forward.z * velocity })
    end
end

function imgui()
    local all = Entity.GetAllWithComponent("AreaTracker")
    if cooldown == 0 then
        for _, v in pairs(all) do
            if Entity.TrackerHasEntities(v) then
                SpawnDarts(v)
                cooldown = 30
            end
        end
    else
        cooldown = cooldown - 1
    end

    Gui.Menu.Window()

    if Raylib.IsKeyPressed(Raylib.Key.D) and Raylib.IsKeyDown(Raylib.Key.LEFT_CONTROL) then
        for k in pairs(selectedEntities) do
            Entity.Duplicate(k)
        end
    end
    if Raylib.IsKeyPressed(Raylib.Key.DELETE) then
        for k in pairs(selectedEntities) do
            Entity.Destroy(k)
        end

        for k in pairs(selectedEntities) do selectedEntities[k] = nil end
    end

    for entity in pairs(selectedEntities) do
        if not Entity.IsValid(entity) then
            selectedEntities[entity] = nil
        end
    end

    for k, _ in ipairs(enemySpawns) do enemySpawns[k] = nil end
    Entity.ForEachWithComponent("EnemySpawn", function(entity)
        table.insert(enemySpawns, entity)
    end)

    for k, _ in ipairs(enemyGoals) do enemyGoals[k] = nil end
    Entity.ForEachWithComponent("EnemyGoal", function(entity)
        table.insert(enemyGoals, entity)
    end)

    Gui.Entity.Window(selectedEntities)

    if placeTrapMode then
        if Raylib.IsMouseButtonPressed(0) then
            local ray = Raylib.GetMouseRay(Raylib.GetMousePosition())
            local hitEntity = Raylib.GetRayCollision(ray)
            if hitEntity ~= nil then
                local assetName = Entity.Get(hitEntity).Render.assetName
                if assetName == "Wall" then
                    Entity.ReplaceComponent("Render", hitEntity, { assetName = "Trap_Wall" })
                    Entity.AddComponent("AreaTracker", hitEntity,
                        { offset = { x = 0, y = 1, z = 0 }, size = { x = 2, y = 2, z = 4 } })
                elseif assetName == "Trap_Wall" then
                    Entity.ReplaceComponent("Render", hitEntity, { assetName = "Wall" })
                    Entity.RemoveComponent("AreaTracker", hitEntity)
                end
            end
        end
    elseif placeFloorMode then
        if not ImGui.WantCaptureMouse() then
            if Raylib.IsMouseButtonDown(0) then
                local ray = Raylib.GetMouseRay(Raylib.GetMousePosition())
                if not Raylib.GetRayCollision(ray) then
                    -- Should be big enough
                    local min = -500
                    local max = 500
                    local p1 = { x = min, y = 0, z = min }
                    local p2 = { x = max, y = 0, z = min }
                    local p3 = { x = max, y = 0, z = max }
                    local p4 = { x = min, y = 0, z = max }
                    local hitPosition = Raylib.GetRayCollisionQuad(ray, p1, p2, p3, p4)
                    if hitPosition ~= nil then
                        local entity = Entity.Create()
                        EntityTools.AddComponentOrPrintError("Render", entity, { assetName = "Floor_Standard" })
                        local position
                        if Raylib.IsKeyDown(Raylib.Key.LEFT_SHIFT) then
                            position = {
                                x = math.floor(hitPosition.x + 0.5),
                                y = 0,
                                z = math.floor(hitPosition.z + 0.5),
                            }
                        else
                            position = {
                                x = math.floor(hitPosition.x / 2) * 2 + 1,
                                y = 0,
                                z = math.floor(hitPosition.z / 2) * 2 + 1,
                            }
                        end

                        EntityTools.AddComponentOrPrintError("Transform", entity,
                            {
                                position = position,
                                rotation = { x = 0, y = 0, z = 0 }
                            })

                        EntityTools.AddComponentOrPrintError("Tile", entity)

                        if placeFloorType == PlaceFloorType.NORMAL or placeFloorType == PlaceFloorType.SPAWN or placeFloorType == PlaceFloorType.GOAL then
                            EntityTools.AddComponentOrPrintError("Walkable", entity)
                        end
                        if placeFloorType == PlaceFloorType.SPAWN then
                            EntityTools.AddComponentOrPrintError("EnemySpawn", entity, { targetGoal = 0 })
                        end
                        if placeFloorType == PlaceFloorType.GOAL then
                            EntityTools.AddComponentOrPrintError("EnemyGoal", entity, { id = 0 })
                        end
                    end
                end
            elseif Raylib.IsMouseButtonDown(1) then
                local ray = Raylib.GetMouseRay(Raylib.GetMousePosition())
                local hitEntity = Raylib.GetRayCollision(ray)
                if hitEntity ~= nil then
                    Entity.Destroy(hitEntity)
                end
            end
        end
    else
        if not ImGui.WantCaptureMouse() then
            if Raylib.IsMouseButtonPressed(0) then
                local ray = Raylib.GetMouseRay(Raylib.GetMousePosition())
                local hitEntity = Raylib.GetRayCollision(ray)
                newSelectedEntity = hitEntity

                if Raylib.IsKeyDown(Raylib.Key.LEFT_CONTROL) or Raylib.IsKeyDown(Raylib.Key.LEFT_SHIFT) then
                    if newSelectedEntity ~= nil then
                        selectedEntities[newSelectedEntity] = true
                    end
                else
                    for k in pairs(selectedEntities) do
                        selectedEntities[k] = nil
                    end

                    if newSelectedEntity ~= nil then
                        selectedEntities[newSelectedEntity] = true
                    end
                end
            end
        end

        local firstEntity
        local selectedEntitiesCount = 0
        for k in pairs(selectedEntities) do
            if not firstEntity then
                firstEntity = k
            end
            selectedEntitiesCount = selectedEntitiesCount + 1
            if selectedEntitiesCount > 1 then
                break
            end
        end

        if selectedEntitiesCount == 1 then
            ImGuizmo.Gizmo(firstEntity, not usingGizmo)
            usingGizmo = ImGuizmo.IsUsing()
        else
            usingGizmo = false
        end
    end

    if menuBarState.drawTileInfo or menuBarState.drawWalkerInfo then
        for y = 1, #walkerMap do
            for x = 1, #walkerMap[y] do
                local pos = Raylib.GetWorldToScreen({
                    x = x * Navigation.tileSize + Navigation.offsetX - Navigation.tileSize / 2,
                    y = 0.25,
                    z = y * Navigation.tileSize + Navigation.offsetY - Navigation.tileSize / 2
                })
                if pos.x > 0 and pos.y > 0 then
                    if menuBarState.drawWalkerInfo then
                        if walkerMap[y][x].id ~= -1 then
                            Raylib.DrawText(walkerMap[y][x].id, pos.x, pos.y - 14, 12)
                            Raylib.DrawText(walkerMap[y][x].distance, pos.x, pos.y, 12)
                            Raylib.DrawText(walkerMap[y][x].wallId, pos.x, pos.y + 14, 12)
                            --Raylib.DrawText(wallMap[y][x].distanceAlongWall, pos.x, pos.y + 28, 12)
                            if walkerMap[y][x].parentDirection == "none" then
                                Raylib.DrawText(walkerMap[y][x].parentDirection, pos.x, pos.y + 28, 20)
                            else
                                Raylib.DrawText(walkerMap[y][x].parentDirection, pos.x, pos.y + 28, 12)
                            end
                        end
                    elseif menuBarState.drawTileInfo then
                        Raylib.DrawText(x .. "x" .. y, pos.x, pos.y - 14, 12)
                        Raylib.DrawText(tileMap[y][x].distance, pos.x, pos.y + 0, 12)
                        Raylib.DrawText(tileMap[y][x].distanceToWall, pos.x, pos.y + 14, 12)
                        if tileMap[y][x].locked then
                            Raylib.DrawText("locked", pos.x, pos.y + 28, 12)
                        end
                    end
                end
            end
        end
    end
end
