function AddComponentOrPrintError(...)
    local errorMessage = Entity.AddComponent(...)
    if errorMessage then
        print("Error when trying to add component:", errorMessage)
    end
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

function NewLevel()
    Entity.ClearRegistry()

    local camera = Entity.Create()
    AddComponentOrPrintError("Transform", camera, {
        position = { x = 10, y = 10, z = 10 },
        rotation = { x = 0, y = 0, z = 0 },
    })
    AddComponentOrPrintError("Camera", camera, {
        target = { x = 0.0, y = 0.0, z = 0.0 },
        up = { x = 0.0, y = 1.0, z = 0.0 },
        fovy = 45.0,
        projection = 0,
    })
end

function SaveLevel()
    local file = io.open("../assets/default_level.lua", "w+")
    file:write("return{")
    --print("{")
    for entity, entityInfo in pairs(Entity.DumpAll()) do
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
    local level = dofile("../assets/default_level.lua")
    if level ~= nil then
        Entity.ClearRegistry()

        for _entity, components in pairs(level) do
            local entity = Entity.Create()

            for component, data in pairs(components) do
                AddComponentOrPrintError(component, entity, data)
            end
        end
    else
        print("Couldn't load ../assets/default_level.lua or it didn't contain a table")
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

    cooldown = 0
    placeTrapMode = false

    LoadLevel()
end

function raylib()
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
        AddComponentOrPrintError("Render", entity, { assetName = "Dart" })
        AddComponentOrPrintError("Transform", entity,
            { position = startPosition, rotation = { x = 0, y = 0, z = 0 } })
        AddComponentOrPrintError("Projectile", entity, { damage = 1 })

        Entity.TransformTo(entity, transformTarget)
        local transformedPosition = Entity.Get(entity).Transform.position
        AddComponentOrPrintError("MaxRange", entity, { maxDistance = 5, distanceFrom = transformedPosition })

        local forward = Entity.GetForwardVector(entity);
        AddComponentOrPrintError("Velocity", entity,
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

    if Raylib.IsKeyPressed(Raylib.Key.D) and Raylib.IsKeyDown(Raylib.Key.LEFT_CONTROL) and selectedEntity ~= nil then
        Entity.Duplicate(selectedEntity)
    end
    if Raylib.IsKeyPressed(Raylib.Key.DELETE) and selectedEntity ~= nil then
        Entity.Destroy(selectedEntity)
        if selectedEntity == entity then
            selectedEntity = nil
        end
    end

    if ImGui.BeginMainMenuBar() then
        if ImGui.MenuItem("New", "", false, true) then
            NewLevel()
        end
        if ImGui.MenuItem("Save", "", false, true) then
            SaveLevel()
        end
        if ImGui.MenuItem("Load", "", false, true) then
            LoadLevel()
        end

        if ImGui.BeginMenu("Spawn entity...", "", false, true) then
            searchText = ImGui.InputText("Filter", searchText)

            ImGui.BeginChild("EntityList", { x = 0, y = 200 })
            if #searchText > 0 then
                for key, value in ipairs(Assets) do
                    if string.match(string.lower(value), searchText) then
                        if ImGui.SmallButton(value) then
                            local entity = Entity.Create()
                            AddComponentOrPrintError("Render", entity, { assetName = value })
                            AddComponentOrPrintError("Transform", entity,
                                { position = { x = 0, y = 0, z = 0 }, rotation = { x = 0, y = 0, z = 0 } })
                            AddComponentOrPrintError("Tile", entity)
                        end
                    end
                end
            else
                for key, value in ipairs(Assets) do
                    if ImGui.SmallButton(value) then
                        local entity = Entity.Create()
                        AddComponentOrPrintError("Render", entity, { assetName = value })
                        AddComponentOrPrintError("Transform", entity,
                            { position = { x = 0, y = 0, z = 0 }, rotation = { x = 0, y = 0, z = 0 } })
                        AddComponentOrPrintError("Tile", entity)
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
                    AddComponentOrPrintError("Render", entity,
                        { assetName = "Barrel" })
                    AddComponentOrPrintError("Transform", entity,
                        { position = spawnPosition, rotation = { x = 0, y = 0, z = 0 } })
                    AddComponentOrPrintError("MoveTowards", entity,
                        { target = goalPosition, speed = 1.5 })
                    AddComponentOrPrintError("Velocity", entity,
                        { x = 0, y = 0, z = 0 })
                    AddComponentOrPrintError("Acceleration", entity,
                        { acceleration = { x = 0, y = 0, z = 0 } })
                    AddComponentOrPrintError("Health", entity, { currentHealth = 3 })
                    AddComponentOrPrintError("MaxRange", entity, { maxDistance = 35, distanceFrom = spawnPosition })
                end
            end
        end

        if ImGui.MenuItem("Spawn projectile", "", false, true) then
            local all = Entity.GetAllWithComponent("AreaTracker")
            for _, v in pairs(all) do
                SpawnDarts(v)
            end
        end

        local placeTrapToggled, newPlaceTrapMode = ImGui.Checkbox("Place trap", placeTrapMode)
        placeTrapMode = newPlaceTrapMode

        ImGui.EndMainMenuBar()
    end

    function ImGuiEntity(entity)
        if ImGui.Button("DuplicateEntity") then
            Entity.Duplicate(entity)
        end

        local components = {
            Render = {
                hasComponent = Entity.HasComponent("Render", entity),
                default = { assetName = "Barrel" }
            },
            Transform = {
                hasComponent = Entity.HasComponent("Transform", entity),
                default = { position = { x = 0, y = 0, z = 0 }, rotation = { x = 0, y = 0, z = 0 } }
            },
            Velocity = {
                hasComponent = Entity.HasComponent("Velocity", entity),
                default = { x = 0.0, y = 0.0, z = 0.0 }
            },
            Tile = { hasComponent = Entity.HasComponent("Tile", entity) },
            EnemyGoal = { hasComponent = Entity.HasComponent("EnemyGoal", entity), default = { id = 0 } },
            EnemySpawn = { hasComponent = Entity.HasComponent("EnemySpawn", entity), default = { targetGoal = 0 } },
            Camera = { hasComponent = Entity.HasComponent("Camera", entity) },
            MoveTowards = { hasComponent = Entity.HasComponent("MoveTowards", entity) },
            Projectile = { hasComponent = Entity.HasComponent("Projectile", entity), default = { damage = 1 } },
            Health = { hasComponent = Entity.HasComponent("Health", entity), default = { currentHealth = 2 } },
            MaxRange = {
                hasComponent = Entity.HasComponent("MaxRange", entity),
                -- This has to be configured manually
                default = { maxDistance = 99999999, distanceFrom = { x = 0, y = 0, z = 0 } }
            },
            AreaTracker = {
                hasComponent = Entity.HasComponent("AreaTracker", entity),
                default = { offset = { x = 0, y = 0, z = 0 }, size = { x = 2, y = 2, z = 2 } }
            },
        }

        for componentName, info in pairs(components) do
            if info.hasComponent then
                local open, clicked = ImGui.CollapsingHeaderToggle(componentName, true)
                if open then
                    Entity.ImGuiModify(componentName, entity)
                end

                if not clicked then
                    Entity.RemoveComponent(componentName, entity)
                end
            end
        end

        if ImGui.BeginCombo("##addcomponent", "Add component") then
            for componentName, info in pairs(components) do
                if not info.hasComponent and componentName ~= "Camera" then
                    if ImGui.Selectable(componentName) then
                        AddComponentOrPrintError(componentName, entity, info.default)
                    end
                end
            end
            ImGui.EndCombo()
        end
    end

    ImGui.Begin("Entity")
    ImGui.Text("Selected entity information")
    if selectedEntity ~= nil then
        if Entity.IsValid(selectedEntity) then
            ImGuiEntity(selectedEntity)
        else
            selectedEntity = nil
        end
    end

    for k, _ in ipairs(enemySpawns) do enemySpawns[k] = nil end
    for k, _ in ipairs(enemyGoals) do enemyGoals[k] = nil end

    ImGui.Separator()
    ImGui.BeginChild("AllEntities")
    Entity.Each(function(entity)
        ImGui.PushID(entity)

        -- Store these here so another call to `Each` can be avoided, though it
        -- does tie the logic to the GUI which is iffy
        if Entity.HasComponent("EnemySpawn", entity) then
            table.insert(enemySpawns, entity)
        elseif Entity.HasComponent("EnemyGoal", entity) then
            table.insert(enemyGoals, entity)
        end

        local destroy = false
        if ImGui.SmallButton("X") then
            destroy = true
        end
        ImGui.SameLine()

        local flags = ImGui.TreeNodeFlag.None
        if entity == selectedEntity then
            flags = ImGui.TreeNodeFlag.Selected
        end
        if newSelectedEntity ~= nil and newSelectedEntity == entity then
            ImGui.SetScrollHereY()
        end
        local open = ImGui.TreeNodeEx("Entity " .. entity, flags)
        if open then
            if ImGui.Button("Select") then
                newSelectedEntity = entity
            end
            ImGuiEntity(entity)
            ImGui.TreePop()
        end

        if destroy then
            Entity.Destroy(entity)
            if selectedEntity == entity then
                selectedEntity = nil
            end
        end
        ImGui.PopID()
    end)
    ImGui.EndChild()
    ImGui.End()

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
        if newSelectedEntity ~= nil then
            selectedEntity = newSelectedEntity
            newSelectedEntity = nil
        end

        if not ImGui.WantCaptureMouse() then
            if Raylib.IsMouseButtonPressed(0) then
                local ray = Raylib.GetMouseRay(Raylib.GetMousePosition())
                local hitEntity = Raylib.GetRayCollision(ray)
                newSelectedEntity = hitEntity

                if newSelectedEntity == nil then
                    selectedEntity = nil
                end
            end
        end

        if selectedEntity ~= nil then
            ImGuizmo.Gizmo(selectedEntity, not usingGizmo)
            usingGizmo = ImGuizmo.IsUsing()
        else
            usingGizmo = false
        end
    end
end

function TrackerCallback()
    SpawnDarts()
end
