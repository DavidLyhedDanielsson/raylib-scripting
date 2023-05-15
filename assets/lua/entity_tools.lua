local function AddComponentOrPrintError(...)
    local errorMessage = Entity.AddComponent(...)
    if errorMessage then
        print("Error when trying to add component:", errorMessage)
    end
end

return {
    AddComponentOrPrintError = AddComponentOrPrintError,
}
