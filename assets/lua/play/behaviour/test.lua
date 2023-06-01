local EntityTools = require("entity_tools")
local common = require("play.common")

local state = {
    lastState = common.waveStates.NOT_STARTED,
    waveDone = false,

    wave = 1,
    waves = {
        3,
        7,
        15
    }
}

local function Spawn()
    for _, spawnEntity in ipairs(common.playState.enemySpawns) do
        local targetGoal = Entity.Get(spawnEntity).EnemySpawn.targetGoal

        local goalPosition
        for _, goalEntity in ipairs(common.playState.enemyGoals) do
            local components = Entity.Get(goalEntity)
            if components.EnemyGoal.id == targetGoal then
                goalPosition = components.Transform.position
            end
        end

        if goalPosition == nil then
            print("GoalPosition is nil!")
        else
            local spawnPosition = Entity.Get(spawnEntity).Transform.position

            local entity = Entity.Create()
            EntityTools.AddComponentOrPrintError("Render", entity,
                { assetName = "Pot1" })
            EntityTools.AddComponentOrPrintError("Transform", entity,
                { position = spawnPosition, rotation = { x = 0, y = 0, z = 0 } })
            EntityTools.AddComponentOrPrintError("MoveTowards", entity,
                { target = goalPosition, speed = 2.5 })
            EntityTools.AddComponentOrPrintError("Velocity", entity,
                { x = 0, y = 0, z = 0 })
            EntityTools.AddComponentOrPrintError("Acceleration", entity,
                { acceleration = { x = 0, y = 0, z = 0 } })
            EntityTools.AddComponentOrPrintError("Health", entity, { currentHealth = 3 })
        end
    end
end

local function SpawnWave()
    RegisterThread(function()
        local shortest = 400

        for i = 1, state.waves[state.wave] do
            Spawn()
            coroutine.yield(shortest)
        end
        state.waveDone = true
    end)
end

return function()
    if (state.lastState == common.waveStates.NOT_STARTED or state.lastState == common.waveStates.WAVE_FINISHED) and common.playState.waveState == common.waveStates.WAVE_RUNNING then
        SpawnWave()
        state.lastState = common.waveStates.WAVE_RUNNING
    end

    if state.waveDone and common.playState.waveState == common.waveStates.WAVE_RUNNING then
        count = Entity.ComponentCount("Health")
        if count == 0 then
            common.playState.waveState = common.waveStates.WAVE_FINISHED
            state.lastState = common.waveStates.WAVE_FINISHED
            state.waveDone = false

            if state.wave == #state.waves then
                common.playState.waveState = common.waveStates.FINISHED
            else
                state.wave = state.wave + 1
            end
        end
    end
end
