if setup == nil then
    setup = true
end

function raylib()
end

function imgui()
    SetNextWindowSize({ x = 200, y = GetScreenHeight() })
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
end
