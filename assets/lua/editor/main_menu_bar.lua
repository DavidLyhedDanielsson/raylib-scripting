local EntityTools = require("entity_tools")
local NavigationTools = require("navigation_tools")
local Level = require("level")

if menuBarState == nil then
    menuBarState = {
        drawTileInfo = false,
        drawWalkerInfo = false,
    }

    searchText = ""
    saveLevelName = ""
    loadLevelName = ""

    drawFieldId = 0
    drawFieldGoalId = 0
end

local function SpawnEnemies()
    Profiling.Start("LUA::SpawnEnemies")
    for i, spawnEntity in ipairs(enemySpawns) do
        Profiling.Start(tostring(i) .. ": Get")
        local sComponent = Entity.Get(spawnEntity).EnemySpawn
        local spawnId = sComponent.id
        local goalId = sComponent.goalId
        local spawnPosition = Entity.Get(spawnEntity).Transform.position
        Profiling.End()

        Profiling.Start(tostring(i) .. ": Create")
        local entity = Entity.Create()
        Profiling.End()

        Profiling.Start(tostring(i) .. ": AddComponent")
        EntityTools.AddComponentOrPrintError("Render", entity,
            { assetName = "Pot1" })
        EntityTools.AddComponentOrPrintError("Transform", entity,
            { position = spawnPosition, rotation = { x = 0, y = 0, z = 0 } })
        EntityTools.AddComponentOrPrintError("MoveTowards", entity,
            { vectorFieldId = (spawnId << 16) | goalId, speed = 1.5 })
        EntityTools.AddComponentOrPrintError("Velocity", entity,
            { x = 0, y = 0, z = 0 })
        EntityTools.AddComponentOrPrintError("Acceleration", entity,
            { acceleration = { x = 0, y = 0, z = 0 } })
        EntityTools.AddComponentOrPrintError("Health", entity, { currentHealth = 3 })
        Profiling.End()
    end
    Profiling.End()
end

local function Window()
    if ImGui.BeginMainMenuBar() then
        if ImGui.BeginMenu("File", true) then
            if ImGui.MenuItem("New", "", false, true) then
                Level.NewLevel()
            end
            if ImGui.BeginMenu("Save as", true) then
                if ImGui.Button("Save") and #saveLevelName > 0 then
                    Level.SaveLevel(saveLevelName)
                    saveLevelName = ""
                end
                ImGui.SameLine()
                saveLevelName = ImGui.InputText("##saveLevelName", saveLevelName)
                ImGui.EndMenu()
            end
            if ImGui.BeginMenu("Load", true) then
                if ImGui.Button("Load") and #loadLevelName > 0 then
                    Level.LoadLevel(loadLevelName)
                    loadLevelName = ""
                end
                ImGui.SameLine()
                loadLevelName = ImGui.InputText("##loadLevelName", loadLevelName)
                ImGui.EndMenu()
            end
            if ImGui.MenuItem("Exit editor", "", false, true) then
                SetLuaFile("menu.lua")
                Entity.ClearRegistry()
            end
            ImGui.EndMenu()
        end

        if ImGui.BeginMenu("Spawn", "", false, true) then
            if ImGui.MenuItem("Enemy", "", false, true) then
                for _, spawnEntity in ipairs(enemySpawns) do
                    local sComponent = Entity.Get(spawnEntity).EnemySpawn
                    local spawnId = sComponent.id
                    local goalId = sComponent.goalId
                    local spawnPosition = Entity.Get(spawnEntity).Transform.position

                    local entity = Entity.Create()
                    EntityTools.AddComponentOrPrintError("Render", entity,
                        { assetName = "Pot1" })
                    EntityTools.AddComponentOrPrintError("Transform", entity,
                        { position = spawnPosition, rotation = { x = 0, y = 0, z = 0 } })
                    EntityTools.AddComponentOrPrintError("MoveTowards", entity,
                        { vectorFieldId = (spawnId << 16) | goalId, speed = 1.5 })
                    EntityTools.AddComponentOrPrintError("Velocity", entity,
                        { x = 0, y = 0, z = 0 })
                    EntityTools.AddComponentOrPrintError("Acceleration", entity,
                        { acceleration = { x = 0, y = 0, z = 0 } })
                    EntityTools.AddComponentOrPrintError("Health", entity, { currentHealth = 3 })
                end
            end

            if ImGui.MenuItem("Wave", "", false, true) then
                RegisterThread(function()
                    for _ = 0, 50 do
                        SpawnEnemies()
                        coroutine.yield(600)
                    end
                end)
            end

            if ImGui.MenuItem("Projectile", "", false, true) then
                local all = Entity.GetAllWithComponent("AreaTracker")
                for _, v in pairs(all) do
                    SpawnDarts(v)
                end
            end

            ImGui.Separator()

            ImGui.Text("Entity")
            searchText = ImGui.InputText("Search", searchText)

            ImGui.BeginChild("EntityList", { x = 0, y = 200 })
            if #searchText > 0 then
                for key, value in ipairs(Assets.Meshes) do
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
                for key, value in ipairs(Assets.Meshes) do
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

        if ImGui.BeginMenu("Nav", true) then
            if ImGui.MenuItem("Build", "", false, true) then
                NavigationTools.Build()
            end
            if ImGui.MenuItem("Toggle tile rendering", "", Navigation.drawTiles, true) then
                Navigation.drawTiles = not Navigation.drawTiles
            end
            if ImGui.MenuItem("Toggle field rendering", "", Navigation.drawField, true) then
                if Navigation.drawField then
                    Navigation.drawField = nil
                else
                    Navigation.drawField = 0
                end
            end
            _, drawFieldId = ImGui.InputInt("Spawn ID", drawFieldId, 1)
            _, drawFieldGoalId = ImGui.InputInt("Goal ID", drawFieldGoalId, 1)
            if (Navigation.drawField) then
                Navigation.drawField = (drawFieldId << 16) | drawFieldGoalId
            end
            ImGui.Separator()
            if ImGui.MenuItem("Smooth field", "", navigationState.smoothField, true) then
                navigationState.smoothField = not navigationState.smoothField
            end

            _, navigationState.tileSize = ImGui.InputFloat("Tile size", navigationState.tileSize, 0.1, 0.1)
            _, Navigation.ksi = ImGui.InputFloat("KSI", Navigation.ksi, 0.1, 0.1)
            _, Navigation.avoidanceLookAhead = ImGui.DragFloat("Avoidance look-ahead", Navigation.avoidanceLookAhead, 0.1,
                0.0, 10.0)
            _, Navigation.obstacleLookAhead = ImGui.DragFloat("Obstacle look-ahead", Navigation.obstacleLookAhead, 0.1,
                0.0, 10.0)

            ImGui.Separator()

            local selectedRadio = -1
            if menuBarState.drawTileInfo then
                selectedRadio = 1
            elseif menuBarState.drawWalkerInfo then
                selectedRadio = 2
            else
                selectedRadio = 0
            end

            ImGui.Text("Map debug info")
            if ImGui.RadioButtonMult("Nothing", selectedRadio, 0) then
                menuBarState.drawTileInfo = false
                menuBarState.drawWalkerInfo = false
            end
            if ImGui.RadioButtonMult("Tile info", selectedRadio, 1) then
                menuBarState.drawTileInfo = true
                menuBarState.drawWalkerInfo = false
            end
            if ImGui.RadioButtonMult("Walker info", selectedRadio, 2) then
                menuBarState.drawTileInfo = false
                menuBarState.drawWalkerInfo = true
            end
            ImGui.EndMenu()
        end

        _, placeTrapMode = ImGui.Checkbox("Place trap", placeTrapMode)
        _, placeFloorMode = ImGui.Checkbox("Place floor", placeFloorMode)

        if placeFloorMode then
            ImGui.SetNextWindowPos({ x = 0, y = 38 })
            ImGui.SetNextWindowSize({ x = 1200, y = 38 })
            ImGui.Begin("More", true,
                ImGui.WindowFlags.NoDecoration | ImGui.WindowFlags.MenuBar | ImGui.WindowFlags.NoBackground)

            if ImGui.BeginMenuBar() then
                ImGui.Text("Place:")
                if ImGui.RadioButton("Normal", placeFloorType == PlaceFloorType.NORMAL) then
                    placeFloorType = PlaceFloorType.NORMAL
                end
                if ImGui.RadioButton("Spawn", placeFloorType == PlaceFloorType.SPAWN) then
                    placeFloorType = PlaceFloorType.SPAWN
                end
                if ImGui.RadioButton("Goal", placeFloorType == PlaceFloorType.GOAL) then
                    placeFloorType = PlaceFloorType.GOAL
                end
                if ImGui.RadioButton("Unwalkable", placeFloorType == PlaceFloorType.UNWALKABLE) then
                    placeFloorType = PlaceFloorType.UNWALKABLE
                end
                ImGui.EndMenuBar()
            end

            ImGui.End()
        end

        ImGui.EndMainMenuBar()
    end
end

return {
    Window = Window
}
