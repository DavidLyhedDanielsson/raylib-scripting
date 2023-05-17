-- in case input_output is modified, it needs to be nulled since this file is
-- ran ecah frame
package.loaded["level"] = nil
package.loaded["navigation_tools"] = nil
package.loaded["entity_tools"] = nil
package.loaded["gui.entity_window"] = nil
package.loaded["gui.main_menu_bar"] = nil

local Level = require("level")
local NavigationTools = require("navigation_tools")
local EntityTools = require("entity_tools")
local Gui = {
    Entity = require("gui.entity_window"),
    Menu = require("gui.main_menu_bar"),
}


if setup == nil then
    setup = true

    selectedEntities = {}
    newSelectedEntity = nil
    searchText = ""
    usingGizmo = false

    enemySpawns = {}
    enemyGoals = {}

    cooldown = 0
    placeTrapMode = false

    Level.LoadLevel()
    NavigationTools.Build()
end

function raylib()
    for entity in pairs(selectedEntities) do
        if not Entity.IsValid(entity) then
            selectedEntities[entity] = nil
        end
    end

    for entity in pairs(selectedEntities) do
        Raylib.DrawEntityBoundingBox(entity)
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

    if wallMap then
        for y = 1, #wallMap do
            for x = 1, #wallMap[y] do
                local distance = wallMap[y][x].distance
                local pos = Raylib.GetWorldToScreen({
                    x = x + Navigation.offsetX - Navigation.tileSize / 2,
                    y = 0,
                    z = y + Navigation.offsetY - Navigation.tileSize / 2
                })
                if pos.x > 0 and pos.y > 0 then
                    -- if wallMap[y][x].id ~= -1 then
                    --     Raylib.DrawText(wallMap[y][x].id, pos.x, pos.y - 14, 12)
                    --     Raylib.DrawText(wallMap[y][x].distance, pos.x, pos.y, 12)
                    --     if wallMap[y][x].parentDirection == "none" then
                    --         Raylib.DrawText(wallMap[y][x].parentDirection, pos.x, pos.y + 14, 20)
                    --     else
                    --         Raylib.DrawText(wallMap[y][x].parentDirection, pos.x, pos.y + 14, 12)
                    --     end
                    -- end


                    -- if tileMap[y][x].distance ~= 9999 then
                    --     Raylib.DrawText(tileMap[y][x].distance, pos.x, pos.y - 7, 12)
                    --     Raylib.DrawText(tileMap[y][x].distanceToWall, pos.x, pos.y + 7, 12)
                    -- end
                end
            end
        end
    end
end
