if setup == nil then
    setup = true

    render_component = false
    transform_component = false
    velocity_component = false

    ordered_assets = {}
    for asset in pairs(Asset) do
        table.insert(ordered_assets, asset)
    end
    table.sort(ordered_assets)

    selected_mesh = 1
end

function raylib()

end

function imgui() 
    Begin("Entity creator")

    _, render_component = Checkbox("Render component", render_component)
    if render_component then
        if BeginCombo("Mesh", ordered_assets[selected_mesh]) then
            local new_selected = -1
            for i = 1, #ordered_assets do
                if Selectable(ordered_assets[i], i == selected_mesh) then
                    new_selected = i
                end
            end
            if new_selected ~= -1 then
            selected_mesh = new_selected 
            end
            EndCombo() 
        end
    end
    _, transform_component = Checkbox("Transform component", transform_component)
    _, velocity_component = Checkbox("Velocity component", velocity_component)

    if Button("Create Entity") then
        local entity = CreateEntity()
        if render_component then
            AddRenderComponent(entity, Asset[ordered_assets[selected_mesh]])
        end
        if transform_component then
            AddTransformComponent(entity)
        end
        if velocity_component then
            AddVelocityComponent(entity)
        end
    end

    End()

    if IsMouseClicked(0) then
        local ray = GetMouseRay(GetMousePos())
        print(ray.position.x, ", ", ray.position.y, ", ", ray.position.z)
        print(ray.direction.x, ", ", ray.direction.y, ", ", ray.direction.z)
        if GetRayCollision(ray) then
            print("Hit")
        else 
            print("No hit")
        end
    end
end