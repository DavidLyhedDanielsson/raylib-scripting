local EntityTools = require("entity_tools")

local function SpawnWave()
    RegisterThread(function()
        local function Spawn()
            for _, spawnEntity in ipairs(PlayState.enemySpawns) do
                local targetGoal = Entity.Get(spawnEntity).EnemySpawn.targetGoal

                local goalPosition
                for _, goalEntity in ipairs(PlayState.enemyGoals) do
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

        local shortest = 400

        for i = 0, 10 do
            Spawn()
            coroutine.yield(shortest)
        end
    end)
end

local state = {
    lastState = WaveState.NOT_STARTED
}

return function()
    if state.lastState == WaveState.NOT_STARTED and PlayState.waveState == WaveState.RUNNING then
        SpawnWave()
        state.lastState = WaveState.RUNNING
    end
end
