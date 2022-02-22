function init()
    local entity = CreateEntity()
    AddRenderComponent(entity, Asset.Insurgent)
    AddTransformComponent(entity)
    AddVelocityComponent(entity)
end