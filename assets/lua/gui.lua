if setup == nil then
    setup = true

    selected_entity = nil
end

function raylib()
end

function AddComponentOrPrintError(...)
    local errorMessage = AddComponent(...)
    if errorMessage then
        print("Error when trying to add component:", errorMessage)
    end
end

function imgui()
    SetNextWindowSize({ x = 300, y = GetRenderHeight() })
    SetNextWindowPos({ x = 0, y = 0 })
    Begin("AssetSpawner", 0, WindowFlags.NoTitleBar)

    for key, value in ipairs(Assets) do
        if SmallButton(value) then
            local entity = CreateEntity()
            AddComponentOrPrintError("Render", entity, { assetName = value })
            AddComponentOrPrintError("Transform", entity,
                { position = { x = 0, y = 0, z = 0 }, rotation = { x = 0, y = 0, z = 0 } })
            AddComponentOrPrintError("Tile", entity)
        end
    end

    End()


    Begin("Entity")
    Each(function(entity)
        PushID(entity)
        local flags = TreeNodeFlag.None
        if entity == selected_entity then
            flags = TreeNodeFlag.Selected
        end

        local open = TreeNodeEx("Entity " .. entity, flags)
        SameLine(GetWindowWidth() - 40)
        if SmallButton("X") then
            DestroyEntity(entity)
        else
            if open then
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
                    Tile = { hasComponent = HasComponent("Tile", entity) }
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
                        if not info.hasComponent then
                            if Selectable(componentName) then
                                AddComponentOrPrintError(componentName, entity, info.default)
                            end
                        end
                    end
                    EndCombo()
                end

                TreePop()
            end
        end
        PopID()
    end)
    End()

    if not WantCaptureMouse() then
        if IsMouseButtonPressed(0) then
            local ray = GetMouseRay(GetMousePosition())
            local hit_entity = GetRayCollision(ray)
            selected_entity = hit_entity
        end
    end


    if selected_entity ~= nil then
        Gizmo(selected_entity)
    end
end
