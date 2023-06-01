local Level = require("level")
local NavigationTools = require("navigation_tools")
local common = require("play.common")

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

function init()
    Level.LoadLevel(StartLevel or "level1")
    NavigationTools.Build()

    common.playState.enemySpawns = {}
    common.playState.enemyGoals = {}

    Entity.ForEachWithComponent("EnemySpawn", function(entity)
        table.insert(common.playState.enemySpawns, entity)
    end)

    Entity.ForEachWithComponent("EnemyGoal", function(entity)
        table.insert(common.playState.enemyGoals, entity)
    end)

    common.playState.waveState = common.waveStates.NOT_STARTED
end

function raylib2D()
    local width = Raylib.GetScreenWidth()

    if common.playState.waveState == common.waveStates.NOT_STARTED or common.playState.waveState == common.waveStates.WAVE_FINISHED then
        if DrawButtonCenter("Start", math.floor(width / 2), 32, 32) == 1 then
            common.playState.waveState = common.waveStates.WAVE_RUNNING
        end
    end

    if common.playState.waveState == common.waveStates.FINISHED then
        if DrawButtonCenter("Yay you win", math.floor(width / 2), 32, 32) == 1 then
            Entity.ClearRegistry()
            SetLuaFile("menu.lua")
        end
    end
end
