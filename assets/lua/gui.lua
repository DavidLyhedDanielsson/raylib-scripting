if setup == nil then
    setup = true

    selected_entity = nil

    searchText = ""

    usingGizmo = false
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
    searchText = InputText("Filter", searchText)

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
    End()

    SetNextWindowPos({ x = 300, y = 0 })
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
        Gizmo(selected_entity, not usingGizmo)
        usingGizmo = IsUsingGizmo()
    else
        usingGizmo = false
    end

    function RecursivePrint(tab, indent)
        if tab == nil then
            print(string.rep("  ", indent), "NIL")
            return
        end
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

    SetNextWindowPos({ x = 800, y = 0 })
    Begin("Tools")
    if Button("Save") then
        local all = DumpEntities()

        file = io.open("outfile.lua", "w+")

        file:write("return{")
        --print("{")
        for entity, entityInfo in pairs(all) do
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
    if Button("Load") then
        level = nil
        level = dofile("outfile.lua")

        if level ~= nil then
            RecursivePrint(level, 0)
            ClearRegistry()

            for _entity, components in pairs(level) do
                local entity = CreateEntity()

                for component, data in pairs(components) do
                    -- if component == "Camera" then
                    --     print("SKIPPING CAMERA")
                    -- else
                    AddComponentOrPrintError(component, entity, data)
                    -- end
                end
            end
        else
            print("Nope")
        end
    end
    End()
end
