local Level = require("level")
local NavigationTools = require("navigation_tools")

function init()
    print(StartLevel)
    Level.LoadLevel(StartLevel or "level1")
    NavigationTools.Build()
end
