function AddComponentOrPrintError(...)
    local errorMessage = AddComponent(...)
    if errorMessage then
        print("Error when trying to add component:", errorMessage)
    end
end

function SaveLevel()
    local file = io.open("../assets/default_level.lua", "w+")
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
    local level = dofile("../assets/default_level.lua")
    if level ~= nil then
        ClearRegistry()

        for _entity, components in pairs(level) do
            local entity = CreateEntity()

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

    LoadLevel()
end

function raylib()
end

function SpawnDarts(transformTarget)
    local offsets = {
        { x = 0.05,  y = 1.286, z = 0.492 },
        { x = -0.05, y = 1.286, z = -0.492 },
        { x = 0,     y = 0.787, z = 0 },
        { x = 0.03,  y = 0.286, z = 0.492 },
        { x = 0.07,  y = 0.286, z = -0.492 },
    }

    for i = 1, 5 do
        local startPosition = { x = offsets[i].x, y = offsets[i].y, z = offsets[i].z }
        local startVelocity = { x = -0.25, y = 0, z = 0 }

        local entity = CreateEntity()
        AddComponentOrPrintError("Render", entity, { assetName = "Dart" })
        AddComponentOrPrintError("Transform", entity,
            { position = startPosition, rotation = { x = 0, y = 3.14 / 2, z = 3.14 / 2 } })
        AddComponentOrPrintError("Velocity", entity, startVelocity)
        AddComponentOrPrintError("Projectile", entity, { damage = 1 })

        TransformTo(entity, transformTarget)
        local transformedPosition = GetEntity(entity).Transform.position
        AddComponentOrPrintError("MaxRange", entity, { maxDistance = 5, distanceFrom = transformedPosition })
    end
end

function imgui()
    local all = GetAllEntitiesWithComponent("AreaTracker")
    if cooldown == 0 then
        for _, v in pairs(all) do
            if TrackerHasEntities(v) then
                SpawnDarts(v)
                cooldown = 30
            end
        end
    else
        cooldown = cooldown - 1
    end

    if IsKeyPressed(Key.D) and IsKeyDown(Key.LEFT_CONTROL) and selectedEntity ~= nil then
        DuplicateEntity(selectedEntity)
    end
    if IsKeyPressed(Key.DELETE) and selectedEntity ~= nil then
        DestroyEntity(selectedEntity)
        if selectedEntity == entity then
            selectedEntity = nil
        end
    end

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
                        { target = goalPosition, speed = 0.03 })
                    AddComponentOrPrintError("Velocity", entity,
                        { x = 0, y = 0, z = 0 })
                    AddComponentOrPrintError("Health", entity, { currentHealth = 3 })
                end
            else
                print("Can't spawn any dudes if there are no goals")
            end
        end

        if MenuItem("Spawn projectile", "", false, true) then
            local all = GetAllEntitiesWithComponent("AreaTracker")
            for _, v in pairs(all) do
                SpawnDarts(v)
            end
        end

        EndMainMenuBar()
    end

    function ImGuiEntity(entity)
        if Button("DuplicateEntity") then
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
            EnemyGoal = { hasComponent = HasComponent("EnemyGoal", entity) },
            EnemySpawn = { hasComponent = HasComponent("EnemySpawn", entity) },
            Camera = { hasComponent = HasComponent("Camera", entity) },
            MoveTowards = { hasComponent = HasComponent("MoveTowards", entity) },
            Projectile = { hasComponent = HasComponent("Projectile", entity), default = { damage = 1 } },
            Health = { hasComponent = HasComponent("Health", entity), default = { currentHealth = 2 } },
            MaxRange = {
                hasComponent = HasComponent("MaxRange", entity),
                -- This has to be configured manually
                default = { maxDistance = 99999999, distanceFrom = { x = 0, y = 0, z = 0 } }
            },
            AreaTracker = {
                hasComponent = HasComponent("AreaTracker", entity),
                default = { offset = { x = 0, y = 0, z = 0 }, size = { x = 2, y = 2, z = 2 } }
            },
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
    end

    Begin("Entity")
    Text("Selected entity information")
    if selectedEntity ~= nil then
        if ValidEntity(selectedEntity) then
            ImGuiEntity(selectedEntity)
        else
            selectedEntity = nil
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
            ImGuiEntity(entity)
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

function TrackerCallback()
    SpawnDarts()
end
