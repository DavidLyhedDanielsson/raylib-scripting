local Level = require("level")
local NavigationTools = require("navigation_tools")

local function DrawTextCenter(text, posX, posY, size)
    local width = Raylib.MeasureTextSize(text, size).x
    Raylib.DrawText(text, posX - width / 2, posY, size)
end

local function DrawButtonCenter(text, posX, posY, size, width)
    local targetWidth = width or 150
    local heightPadding = 0

    local measuredSize = Raylib.MeasureTextSize(text, size)
    return Raygui.Button(
        {
            x = posX - targetWidth / 2,
            y = posY - measuredSize.y / 2 - heightPadding,
            width = targetWidth,
            height = measuredSize.y + heightPadding * 2,
        },
        text)
end

WaveState = {
    NOT_STARTED = 0,
    RUNNING = 1,
    FINISHED = 3,
}

function init()
    Level.LoadLevel(StartLevel or "level1")
    NavigationTools.Build()

    local enemySpawns = {}
    Entity.ForEachWithComponent("EnemySpawn", function(entity)
        table.insert(enemySpawns, entity)
    end)

    local enemyGoals = {}
    Entity.ForEachWithComponent("EnemyGoal", function(entity)
        table.insert(enemyGoals, entity)
    end)

    PlayState = {
        waveState = WaveState.NOT_STARTED,

        enemySpawns = enemySpawns,
        enemyGoals = enemyGoals,
    }
end

function raylib2D()
    local width = Raylib.GetScreenWidth()

    if PlayState.waveState == WaveState.NOT_STARTED then
        if DrawButtonCenter("Start", math.floor(width / 2), 32, 32) == 1 then
            PlayState.waveState = WaveState.RUNNING
        end
    end
end
