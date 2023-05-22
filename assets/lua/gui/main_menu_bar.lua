local EntityTools = require("entity_tools")
local NavigationTools = require("navigation_tools")
local Level = require("level")

if menuBarState == nil then
    menuBarState = {
        drawTileInfo = false,
        drawWallInfo = false,
    }
end

local function Window()
    if ImGui.BeginMainMenuBar() then
        if ImGui.MenuItem("New", "", false, true) then
            Level.NewLevel()
        end
        if ImGui.MenuItem("Save", "", false, true) then
            Level.SaveLevel()
        end
        if ImGui.MenuItem("Load", "", false, true) then
            Level.LoadLevel()
        end
        if ImGui.BeginMenu("Nav", true) then
            if ImGui.MenuItem("Build", "", false, true) then
                NavigationTools.Build()
            end
            ImGui.Separator()
            if ImGui.MenuItem("Smooth field", "", navigationState.smoothField, true) then
                navigationState.smoothField = not navigationState.smoothField
            end
            if ImGui.MenuItem("Fix disconnected", "", navigationState.fixDisconnected, true) then
                navigationState.fixDisconnected = not navigationState.fixDisconnected
            end

            _, navigationState.tileSize = ImGui.InputFloat("Tile size", navigationState.tileSize, 0.1, 0.1)
            _, Navigation.ksi = ImGui.InputFloat("KSI", Navigation.ksi, 0.1, 0.1)

            ImGui.Separator()
            if ImGui.MenuItem("Draw field", "", Navigation.draw, true) then
                Navigation.draw = not Navigation.draw
            end

            local selectedRadio = -1
            if menuBarState.drawTileInfo then
                selectedRadio = 1
            elseif menuBarState.drawWallInfo then
                selectedRadio = 2
            else
                selectedRadio = 0
            end

            if ImGui.RadioButtonMult("Nothing", selectedRadio, 0) then
                menuBarState.drawTileInfo = false
                menuBarState.drawWallInfo = false
            end
            if ImGui.RadioButtonMult("Draw tile info", selectedRadio, 1) then
                menuBarState.drawTileInfo = true
                menuBarState.drawWallInfo = false
            end
            if ImGui.RadioButtonMult("Draw wall info", selectedRadio, 2) then
                menuBarState.drawTileInfo = false
                menuBarState.drawWallInfo = true
            end
            ImGui.EndMenu()
        end

        if ImGui.BeginMenu("Spawn entity...", "", false, true) then
            searchText = ImGui.InputText("Filter", searchText)

            ImGui.BeginChild("EntityList", { x = 0, y = 200 })
            if #searchText > 0 then
                for key, value in ipairs(Assets) do
                    if string.match(string.lower(value), searchText) then
                        if ImGui.SmallButton(value) then
                            local entity = Entity.Create()
                            EntityTools.AddComponentOrPrintError("Render", entity, { assetName = value })
                            EntityTools.AddComponentOrPrintError("Transform", entity,
                                { position = { x = 0, y = 0, z = 0 }, rotation = { x = 0, y = 0, z = 0 } })
                            EntityTools.AddComponentOrPrintError("Tile", entity)
                        end
                    end
                end
            else
                for key, value in ipairs(Assets) do
                    if ImGui.SmallButton(value) then
                        local entity = Entity.Create()
                        EntityTools.AddComponentOrPrintError("Render", entity, { assetName = value })
                        EntityTools.AddComponentOrPrintError("Transform", entity,
                            { position = { x = 0, y = 0, z = 0 }, rotation = { x = 0, y = 0, z = 0 } })
                        EntityTools.AddComponentOrPrintError("Tile", entity)
                    end
                end
            end
            ImGui.EndChild()
            ImGui.EndMenu()
        end

        if ImGui.MenuItem("Spawn dude", "", false, true) then
            for _, spawnEntity in ipairs(enemySpawns) do
                local targetGoal = Entity.Get(spawnEntity).EnemySpawn.targetGoal

                local goalPosition
                for _, goalEntity in ipairs(enemyGoals) do
                    local components = Entity.Get(goalEntity)
                    if components.EnemyGoal.id == targetGoal then
                        goalPosition = components.Transform.position
                    end
                end

                if goalPosition == nil then
                    print("GoalPosition is nil!")
                else
                    local spawnPosition = Entity.Get(spawnEntity).Transform.position

                    local entity = Entity.Create()
                    EntityTools.AddComponentOrPrintError("Render", entity,
                        { assetName = "Pot1" })
                    EntityTools.AddComponentOrPrintError("Transform", entity,
                        { position = spawnPosition, rotation = { x = 0, y = 0, z = 0 } })
                    EntityTools.AddComponentOrPrintError("MoveTowards", entity,
                        { target = goalPosition, speed = 2.5 })
                    EntityTools.AddComponentOrPrintError("Velocity", entity,
                        { x = 0, y = 0, z = 0 })
                    EntityTools.AddComponentOrPrintError("Acceleration", entity,
                        { acceleration = { x = 0, y = 0, z = 0 } })
                    EntityTools.AddComponentOrPrintError("Health", entity, { currentHealth = 3 })
                end
            end
        end

        if ImGui.MenuItem("Spawn wave", "", false, true) then
            RegisterThread(function()
                function Spawn()
                    for _, spawnEntity in ipairs(enemySpawns) do
                        local targetGoal = Entity.Get(spawnEntity).EnemySpawn.targetGoal

                        local goalPosition
                        for _, goalEntity in ipairs(enemyGoals) do
                            local components = Entity.Get(goalEntity)
                            if components.EnemyGoal.id == targetGoal then
                                goalPosition = components.Transform.position
                            end
                        end

                        if goalPosition == nil then
                            print("GoalPosition is nil!")
                        else
                            local spawnPosition = Entity.Get(spawnEntity).Transform.position

                            local entity = Entity.Create()
                            EntityTools.AddComponentOrPrintError("Render", entity,
                                { assetName = "Pot1" })
                            EntityTools.AddComponentOrPrintError("Transform", entity,
                                { position = spawnPosition, rotation = { x = 0, y = 0, z = 0 } })
                            EntityTools.AddComponentOrPrintError("MoveTowards", entity,
                                { target = goalPosition, speed = 2.5 })
                            EntityTools.AddComponentOrPrintError("Velocity", entity,
                                { x = 0, y = 0, z = 0 })
                            EntityTools.AddComponentOrPrintError("Acceleration", entity,
                                { acceleration = { x = 0, y = 0, z = 0 } })
                            EntityTools.AddComponentOrPrintError("Health", entity, { currentHealth = 3 })
                        end
                    end
                end

                local shortest = 500

                for i = 0, 30 do
                    Spawn()
                    coroutine.yield(shortest)
                end
            end)
        end

        if ImGui.MenuItem("Spawn projectile", "", false, true) then
            local all = Entity.GetAllWithComponent("AreaTracker")
            for _, v in pairs(all) do
                SpawnDarts(v)
            end
        end

        _, placeTrapMode = ImGui.Checkbox("Place trap", placeTrapMode)
        _, placeFloorMode = ImGui.Checkbox("Place floor", placeFloorMode)

        ImGui.EndMainMenuBar()
    end
end

return {
    Window = Window
}
