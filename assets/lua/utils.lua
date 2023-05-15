function RecursivePrint(tab, indent)
    indent = indent or 0
    if tab == nil then
        print(string.rep("  ", indent), "NIL")
        return
    end
    if type(tab) ~= "table" then
        print(string.rep("  ", indent), tab)
    else
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
end

return {
    RecursivePrint = RecursivePrint,
}
