function AddComponentOrPrintError(...)
    local errorMessage = AddComponent(...)
    if errorMessage then
        print("Error when trying to add component:", errorMessage)
    end
end

function SaveLevel()
    local file = io.open("outfile.lua", "w+")
    file:write("return{")
    --print("{")
    for entity, entityInfo in pairs(DumpEntities()) do
        file:write("[", entity, "]={")
        --print("  ", entity, " = {")
        RecursiveWrite(file, entityInfo)
        file:write("},")
        --print("  },")
    end
    file:write("}")
    --print("}")

    file:close()
end

function LoadLevel()
    local level = dofile("outfile.lua")
    if level ~= nil then
        ClearRegistry()

        for _entity, components in pairs(level) do
            local entity = CreateEntity()

            for component, data in pairs(components) do
                AddComponentOrPrintError(component, entity, data)
            end
        end
    else
        print("Couldn't load outfile.lua or it didn't contain a table")
    end
end

if setup == nil then
    setup = true

    selectedEntity = nil
    newSelectedEntity = nil
    searchText = ""
    usingGizmo = false

    enemySpawns = {}
    enemyGoals = {}

    LoadLevel()
end

function raylib()
end

function imgui()
    if BeginMainMenuBar() then
        if MenuItem("Save", "", false, true) then
            SaveLevel()
        end
        if MenuItem("Load", "", false, true) then
            LoadLevel()
        end

        if BeginMenu("Spawn entity...", "", false, true) then
            searchText = InputText("Filter", searchText)

            BeginChild("EntityList", { x = 0, y = 200 })
            if #searchText > 0 then
                for key, value in ipairs(Assets) do
                    if string.match(string.lower(value), searchText) then
                        if SmallButton(value) then
                            local entity = CreateEntity()
                            AddComponentOrPrintError("Render", entity, { assetName = value })
                            AddComponentOrPrintError("Transform", entity,
                                { position = { x = 0, y = 0, z = 0 }, rotation = { x = 0, y = 0, z = 0 } })
                            AddComponentOrPrintError("Tile", entity)
                        end
                    end
                end
            else
                for key, value in ipairs(Assets) do
                    if SmallButton(value) then
                        local entity = CreateEntity()
                        AddComponentOrPrintError("Render", entity, { assetName = value })
                        AddComponentOrPrintError("Transform", entity,
                            { position = { x = 0, y = 0, z = 0 }, rotation = { x = 0, y = 0, z = 0 } })
                        AddComponentOrPrintError("Tile", entity)
                    end
                end
            end
            EndChild()
            EndMenu()
        end

        if MenuItem("Spawn dude", "", false, true) then
            print("Goals: ", #enemyGoals)
            if #enemyGoals > 0 then
                for _, spawnEntity in ipairs(enemySpawns) do
                    local closestGoal
                    for _, goalEntity in ipairs(enemyGoals) do
                        if closestGoal == nil then
                            closestGoal = goalEntity
                        else
                            print("Too many goals yo")
                        end
                    end

                    local spawnPosition = GetEntity(spawnEntity).Transform.position
                    local goalPosition = GetEntity(closestGoal).Transform.position

                    local entity = CreateEntity()
                    AddComponentOrPrintError("Render", entity,
                        { assetName = "Barrel" })
                    AddComponentOrPrintError("Transform", entity,
                        { position = spawnPosition, rotation = { x = 0, y = 0, z = 0 } })
                    AddComponentOrPrintError("MoveTowards", entity,
                        { target = goalPosition, speed = 0.01 })
                end
            else
                print("Can't spawn any dudes if there are no goals")
            end
        end

        EndMainMenuBar()
    end

    Begin("Entity")
    Text("Selected entity information")
    if selectedEntity ~= nil then
        if Button("DuplicateEntity") then
            DuplicateEntity(selectedEntity)
        end

        local components = {
            Render = {
                hasComponent = HasComponent("Render", selectedEntity),
                default = { assetName = "Barrel" }
            },
            Transform = {
                hasComponent = HasComponent("Transform", selectedEntity),
                default = { position = { x = 0, y = 0, z = 0 }, rotation = { x = 0, y = 0, z = 0 } }
            },
            Velocity = {
                hasComponent = HasComponent("Velocity", selectedEntity),
                default = { x = 0.0, y = 0.0, z = 0.0 }
            },
            Tile = { hasComponent = HasComponent("Tile", selectedEntity) },
            EnemyGoal = { hasComponent = HasComponent("EnemyGoal", selectedEntity) },
            EnemySpawn = { hasComponent = HasComponent("EnemySpawn", selectedEntity) },
            Camera = { hasComponent = HasComponent("Camera", selectedEntity) }
        }

        for componentName, info in pairs(components) do
            if info.hasComponent then
                checked = true
                local open, clicked = CollapsingHeaderToggle(componentName, checked)
                if open then
                    Modify(componentName, selectedEntity)
                end

                if not clicked then
                    RemoveComponent(componentName, selectedEntity)
                end
            end
        end

        if BeginCombo("##addcomponent", "Add component") then
            for componentName, info in pairs(components) do
                if not info.hasComponent and componentName ~= "Camera" then
                    if Selectable(componentName) then
                        AddComponentOrPrintError(componentName, selectedEntity, info.default)
                    end
                end
            end
            EndCombo()
        end
    end

    for k, _ in ipairs(enemySpawns) do enemySpawns[k] = nil end
    for k, _ in ipairs(enemyGoals) do enemyGoals[k] = nil end

    Separator()
    BeginChild("AllEntities")
    Each(function(entity)
        PushID(entity)

        -- Store these here so another call to `Each` can be avoided, though it
        -- does tie the logic to the GUI which is iffy
        if HasComponent("EnemySpawn", entity) then
            table.insert(enemySpawns, entity)
        elseif HasComponent("EnemyGoal", entity) then
            table.insert(enemyGoals, entity)
        end

        local destroy = false
        if SmallButton("X") then
            destroy = true
        end
        SameLine()

        local flags = TreeNodeFlag.None
        if entity == selectedEntity then
            flags = TreeNodeFlag.Selected
        end
        if newSelectedEntity ~= nil and newSelectedEntity == entity then
            SetScrollHereY()
        end
        local open = TreeNodeEx("Entity " .. entity, flags)
        if open then
            if Button("Select") then
                newSelectedEntity = entity
            end
            if Button("Duplicate") then
                DuplicateEntity(entity)
            end

            local components = {
                Render = {
                    hasComponent = HasComponent("Render", entity),
                    default = { assetName = "Barrel" }
                },
                Transform = {
                    hasComponent = HasComponent("Transform", entity),
                    default = { position = { x = 0, y = 0, z = 0 }, rotation = { x = 0, y = 0, z = 0 } }
                },
                Velocity = {
                    hasComponent = HasComponent("Velocity", entity),
                    default = { x = 0.0, y = 0.0, z = 0.0 }
                },
                Tile = { hasComponent = HasComponent("Tile", entity) },
                Camera = { hasComponent = HasComponent("Camera", entity) }
            }

            for componentName, info in pairs(components) do
                if info.hasComponent then
                    checked = true
                    local open, clicked = CollapsingHeaderToggle(componentName, checked)
                    if open then
                        Modify(componentName, entity)
                    end

                    if not clicked then
                        RemoveComponent(componentName, entity)
                    end
                end
            end

            if BeginCombo("##addcomponent", "Add component") then
                for componentName, info in pairs(components) do
                    if not info.hasComponent and componentName ~= "Camera" then
                        if Selectable(componentName) then
                            AddComponentOrPrintError(componentName, entity, info.default)
                        end
                    end
                end
                EndCombo()
            end

            TreePop()
        end

        if destroy then
            DestroyEntity(entity)
            if selectedEntity == entity then
                selectedEntity = nil
            end
        end
        PopID()
    end)
    EndChild()
    End()

    if newSelectedEntity ~= nil then
        selectedEntity = newSelectedEntity
        newSelectedEntity = nil
    end

    if not WantCaptureMouse() then
        if IsMouseButtonPressed(0) then
            local ray = GetMouseRay(GetMousePosition())
            local hitEntity = GetRayCollision(ray)
            newSelectedEntity = hitEntity

            if newSelectedEntity == nil then
                selectedEntity = nil
            end
        end
    end

    if selectedEntity ~= nil then
        Gizmo(selectedEntity, not usingGizmo)
        usingGizmo = IsUsingGizmo()
    else
        usingGizmo = false
    end

    function RecursivePrint(tab, indent)
        if tab == nil then
            print(string.rep("  ", indent), "NIL")
            return
        end
        if type(tab) ~= "table" then
            print(string.rep("  ", indent), tab)
        else
            for key, value in pairs(tab) do
                if type(value) == "table" then
                    print(string.rep("  ", indent), key, " = {")
                    RecursivePrint(value, indent + 1)
                    print(string.rep("  ", indent), "},")
                elseif type(value) == "string" then
                    print(string.rep("  ", indent), key, " = \"", value, "\",")
                else
                    print(string.rep("  ", indent), key, " = ", value, ",")
                end
            end
        end
    end

    function RecursiveWrite(file, tab)
        for key, value in pairs(tab) do
            if type(value) == "table" then
                file:write(key, "={")
                RecursiveWrite(file, value)
                file:write("},")
            elseif type(value) == "string" then
                file:write(key, "=\"", value, "\",")
            else
                file:write(key, "=", value, ",")
            end
        end
    end
end
