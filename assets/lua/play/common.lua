local waveStates <const> = {
    NOT_STARTED = 0,
    WAVE_RUNNING = 1,
    WAVE_FINISHED = 2,
    FINISHED = 3,
}

local playState = {
    waveState = waveStates.NOT_STARTED,
    enemySpawns = {},
    enemyGoals = {},
}

return {
    playState = playState,
    waveStates = waveStates,
}
