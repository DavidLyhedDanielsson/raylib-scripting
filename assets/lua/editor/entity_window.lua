local EntityTools = require("entity_tools")

local function Multiple(entities)
    local defaultComponents = {
        Render = { assetName = "Barrel" },
        Transform = { position = { x = 0, y = 0, z = 0 }, rotation = { x = 0, y = 0, z = 0 } },
        Velocity = { x = 0.0, y = 0.0, z = 0.0 },
        Tile = {},
        EnemyGoal = { id = 0 },
        EnemySpawn = { targetGoal = 0 },
        MoveTowards = {},
        Projectile = { damage = 1 },
        Health = { currentHealth = 2 },
        MaxRange = { maxDistance = 99999999, distanceFrom = { x = 0, y = 0, z = 0 } },
        AreaTracker = { offset = { x = 0, y = 0, z = 0 }, size = { x = 2, y = 2, z = 2 } },
        Walkable = {},
        Obstacle = {},
    }

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
        Walkable = { hasComponent = Entity.HasComponent("Walkable", entity) },
        Obstacle = { hasComponent = Entity.HasComponent("Obstacle", entity) },
        Behaviour = { hasComponent = Entity.HasComponent("Behaviour", entity), default = { script = "" } },
    }

    -- Debugging code that is unlikely to be useful again
    -- if components.Walkable.hasComponent then
    --     ImGui.Text("Distance to wall: ")
    --     ImGui.SameLine()

    --     local position = Entity.Get(entity).Transform.position
    --     local tilePos = Navigation.GetTileSpace({ x = position.x, y = position.z })
    --     ImGui.Text(DistanceToWall(tilePos.x, tilePos.y))
    -- end

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
                    EntityTools.AddComponentOrPrintError(componentName, entity, info.default)
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
