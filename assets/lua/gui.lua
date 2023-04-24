if setup == nil then
    setup = true
end

function raylib()
end

function imgui()
    SetNextWindowSize({ x = 300, y = GetRenderHeight() })
    SetNextWindowPos({ x = 0, y = 0 })
    Begin("Test", 0, WindowFlags.NoTitleBar)

    for key, value in ipairs(Assets) do
        if SmallButton(value) then
            local entity = CreateEntity()
            AddRenderComponent(entity, value)
            AddTransformComponentAt(entity, 0, 0, 0)
            AddTileComponent(entity)
        end
    end

    End()

    Begin("Entity")
    Each(function(entity)
        if TreeNode("Entity " .. entity) then
            PushStyleColor(Col.Button, { x = 0xcc / 255.0, y = 0x24 / 255.0, z = 0x1d / 255.0, w = 1.0 });
            PushStyleColor(Col.ButtonHovered, { x = 0xd1 / 255.0, y = 0x39 / 255.0, z = 0x33 / 255.0, w = 1.0 });
            PushStyleColor(Col.ButtonActive, { x = 0xb7 / 255.0, y = 0x20 / 255.0, z = 0x1a / 255.0, w = 1.0 });
            local deleteEntity = Button("DELETE ENTITY")
            PopStyleColor(3)

            local hasComponents = true
            ModifyEntityOrElse(entity, function()
                Text("No components in this entity")
                hasComponents = false
            end)

            if hasComponents then
                if Button("DuplicateEntity") then
                    DuplicateEntity(entity)
                end
            end

            local components = {
                render = HasComponent("Render", entity),
                transform = HasComponent("Transform", entity),
                velocity = HasComponent("Velocity", entity),
                tile = HasComponent("Tile", entity)
            }

            -- TODO: camel or pascal or what?
            local anyMissing = not components.render
                or not components.transform
                or not components.velocity
                or not components.tile

            if anyMissing then
                if BeginCombo("##addcomponent", "Add component") then
                    -- TODO: Render component
                    if not components.render then
                        if Selectable("Render") then
                            AddComponent(entity, "Render", "Barrel")
                        end
                    end
                    if not components.transform then
                        if Selectable("Transform") then
                            AddComponent(entity, "Transform", {
                                position = { x = 0, y = 0, z = 0 }
                            })
                        end
                    end
                    if not components.velocity then
                        if Selectable("Velocity") then
                            AddComponent(entity, "Velocity", {
                                x = 0.0, y = 0.0, z = 0.0
                            })
                        end
                    end
                    if not components.tile then
                        if Selectable("Tile") then
                            AddComponent(entity, "Tile")
                        end
                    end

                    EndCombo()
                end
            else
                BeginDisabled();
                BeginCombo("##addcomponent", "No more components available");
                EndDisabled();
            end

            TreePop()
        end
    end)
    End()
end
