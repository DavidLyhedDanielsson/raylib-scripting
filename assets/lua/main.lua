function init()
    MainState = {
        current = "main"
    }

    scrollIndex = 0
    active = 0

    levelList = {
        scroll = { x = 0, y = 0 },
        view = { x = 0, y = 0, width = 0, height = 0 },
        minY = 0.3,
        width = 0.5,
        height = 0.5,
    }
end

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

local function DrawLevelList()
    local screenHeight = Raylib.GetScreenHeight()
    local screenWidth = Raylib.GetScreenWidth()

    local levelCount = #Levels

    local width = math.floor(levelList.width * screenWidth)
    local height = math.floor(levelList.height * screenHeight)

    local midX = math.floor(width / 2)
    local minX = midX
    local minY = math.floor(levelList.minY * screenHeight)

    local panelContentHeight = levelCount * 25 + 20

    local levelButtonLPadding = 20
    local levelButtonRPadding = levelButtonLPadding
    if panelContentHeight > height then
        local scrollBarWidth = Raygui.GetStyle(Raygui.Control.LISTVIEW, Raygui.ListViewProperty.SCROLLBAR_WIDTH);

        levelButtonRPadding = levelButtonRPadding + scrollBarWidth
    end

    local clickedLevel = nil

    Raylib.BeginScissorMode(minX, minY, width, height)
    _, levelList.scroll, levelList.view = Raygui.ScrollPanel(
        { x = minX, y = minY, width = width, height = height },
        nil,
        { x = 0, y = 0, width = 200, height = panelContentHeight },
        levelList.scroll,
        levelList.view)

    for i = 1, levelCount do
        if Raygui.Button(
                {
                    x = minX + levelButtonLPadding,
                    y = minY + 10 + levelList.scroll.y + (i - 1) * 25,
                    width = width - (levelButtonLPadding + levelButtonRPadding),
                    height = 20
                },
                "Level " .. i) == 1 then
            local mousePos = Raylib.GetMousePosition()
            if mousePos.x >= minX and mousePos.x <= minX + width and mousePos.y >= minY and mousePos.y <= minY + height then
                clickedLevel = "level" .. i
            end
        end
    end
    Raylib.EndScissorMode()
    return clickedLevel
end

local function HandleBackButton()
    local screenHeight = Raylib.GetScreenHeight()
    if Raygui.Button(
            {
                x = 20,
                y = screenHeight - 40,
                width = 150,
                height = 30,
            },
            "Back to menu") == 1 then
        MainState.current = "main"
    end
end

function raylib2D()
    Raygui.SetStyle(Raygui.Control.BUTTON, Raygui.DefaultProperty.TEXT_SIZE, 24)
    local screenWidth = Raylib.GetScreenWidth()
    local screenHeight = Raylib.GetScreenHeight()

    local midX = screenWidth / 2

    DrawTextCenter("MassTD", midX, 0.1 * screenHeight, 64)

    if MainState.current == "main" then
        if DrawButtonCenter("Play", midX, 200, 32) == 1 then
            MainState.current = "play"
            Levels = Assets.GetLevels()
        end
        if DrawButtonCenter("Editor", midX, 280, 32) == 1 then
            MainState.current = "editor"
            Levels = Assets.GetLevels()
        end
        if DrawButtonCenter("Exit", midX, 360, 32) == 1 then
            Exit()
        end
    elseif MainState.current == "play" then
        DrawLevelList()
        HandleBackButton()
    elseif MainState.current == "editor" then
        local clickedLevel = DrawLevelList()
        if clickedLevel then
            StartLevel = clickedLevel
            SetLuaFile("editor.lua")
        end

        if DrawButtonCenter("New level", midX, (levelList.minY + levelList.height) * screenHeight + 30, 32, 0.2 * screenWidth) == 1 then
            StartLevel = ""
            SetLuaFile("editor.lua")
        end

        HandleBackButton()
    else
        MainState.current = "main"
    end
end
