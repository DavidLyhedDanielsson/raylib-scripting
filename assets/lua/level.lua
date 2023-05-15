local EntityTools = require("entity_tools")

local function WriteFile(file, tab)
    for key, value in pairs(tab) do
        if type(value) == "table" then
            file:write(key, "={")
            WriteFile(file, value)
            file:write("},")
        elseif type(value) == "string" then
            file:write(key, "=\"", value, "\",")
        else
            file:write(key, "=", value, ",")
        end
    end
end

local function SaveLevel()
    local file = io.open("../assets/default_level.lua", "w+")
    if not file then
        print("Couldn't open default_level.lua for writing")
        return
    end
    file:write("return{")
    for entity, entityInfo in pairs(Entity.DumpAll()) do
        file:write("[", entity, "]={")
        WriteFile(file, entityInfo)
        file:write("},")
    end
    file:write("}")

    file:close()

    print("Save completed")
end

local function LoadLevel()
    local level = dofile("../assets/default_level.lua")
    if level ~= nil then
        Entity.ClearRegistry()

        for _entity, components in pairs(level) do
            local entity = Entity.Create()

            for component, data in pairs(components) do
                EntityTools.AddComponentOrPrintError(component, entity, data)
            end
        end
    else
        print("Couldn't load ../assets/default_level.lua or it didn't contain a table")
    end
end

local function NewLevel()
    Entity.ClearRegistry()

    local camera = Entity.Create()
    EntityTools.AddComponentOrPrintError("Transform", camera, {
        position = { x = 10, y = 10, z = 10 },
        rotation = { x = 0, y = 0, z = 0 },
    })
    EntityTools.AddComponentOrPrintError("Camera", camera, {
        target = { x = 0.0, y = 0.0, z = 0.0 },
        up = { x = 0.0, y = 1.0, z = 0.0 },
        fovy = 45.0,
        projection = 0,
    })
end

return {
    SaveLevel = SaveLevel,
    LoadLevel = LoadLevel,
    NewLevel = NewLevel,
}
