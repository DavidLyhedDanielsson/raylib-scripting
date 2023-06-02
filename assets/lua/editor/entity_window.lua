local EntityTools = require("entity_tools")

local defaultComponents = {
    Render = { assetName = "Barrel" },
    Transform = { position = { x = 0, y = 0, z = 0 }, rotation = { x = 0, y = 0, z = 0 } },
    Velocity = { x = 0.0, y = 0.0, z = 0.0 },
    Tile = {},
    EnemyGoal = { id = 0 },
    EnemySpawn = { id = 0, goalId = 0 },
    MoveTowards = { vectorFieldId = 0, speed = 1 },
    Projectile = { damage = 1 },
    Health = { currentHealth = 2 },
    MaxRange = { maxDistance = 99999999, distanceFrom = { x = 0, y = 0, z = 0 } },
    AreaTracker = { offset = { x = 0, y = 0, z = 0 }, size = { x = 2, y = 2, z = 2 } },
    Walkable = {},
    NavGate = { allowedGoalId = 0 },
    Behaviour = { script = "" }
}

local function Multiple(entities)
    if ImGui.BeginCombo("##addcomponent", "Add component") then
        for componentName, default in pairs(defaultComponents) do
            if ImGui.Selectable(componentName) then
                for entity in pairs(entities) do
                    if not Entity.HasComponent(componentName, entity) then
                        EntityTools.AddComponentOrPrintError(componentName, entity, default)
                    end
                end
            end
        end
        ImGui.EndCombo()
    end
end

local function Single(entity)
    if ImGui.Button("DuplicateEntity") then
        Entity.Duplicate(entity)
    end

    local components = {
        Render = Entity.HasComponent("Render", entity),
        Transform = Entity.HasComponent("Transform", entity),
        Velocity = Entity.HasComponent("Velocity", entity),
        Tile = Entity.HasComponent("Tile", entity),
        EnemyGoal = Entity.HasComponent("EnemyGoal", entity),
        EnemySpawn = Entity.HasComponent("EnemySpawn", entity),
        Camera = Entity.HasComponent("Camera", entity),
        MoveTowards = Entity.HasComponent("MoveTowards", entity),
        Projectile = Entity.HasComponent("Projectile", entity),
        Health = Entity.HasComponent("Health", entity),
        MaxRange = Entity.HasComponent("MaxRange", entity),
        AreaTracker = Entity.HasComponent("AreaTracker", entity),
        Walkable = Entity.HasComponent("Walkable", entity),
        NavGate = Entity.HasComponent("NavGate", entity),
        Behaviour = Entity.HasComponent("Behaviour", entity),
    }

    for componentName, hasComponent in pairs(components) do
        if hasComponent then
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
        for componentName, hasComponent in pairs(components) do
            if not hasComponent and componentName ~= "Camera" then
                if ImGui.Selectable(componentName) then
                    EntityTools.AddComponentOrPrintError(componentName, entity, defaultComponents[componentName])
                end
            end
        end
        ImGui.EndCombo()
    end
end

local function Window(selectedEntities)
    ImGui.Begin("Entity")
    ImGui.Text("Selected entity information")
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

    if selectedEntitiesCount > 0 then
        if selectedEntitiesCount == 1 then
            Single(firstEntity)
        else
            Multiple(selectedEntities)
        end
    end

    ImGui.Separator()
    ImGui.BeginChild("AllEntities")
    Entity.Each(function(entity)
        ImGui.PushID(entity)

        local destroy = false
        if ImGui.SmallButton("X") then
            destroy = true
        end
        ImGui.SameLine()

        local flags = ImGui.TreeNodeFlag.None

        if selectedEntities[entity] then
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
            Single(entity)
            ImGui.TreePop()
        end

        if destroy then
            Entity.Destroy(entity)
            if selectedEntities[entity] then
                selectedEntities[entity] = nil
            end
        end
        ImGui.PopID()
    end)
    ImGui.EndChild()
    ImGui.End()
end

return {
    Window = Window
}
