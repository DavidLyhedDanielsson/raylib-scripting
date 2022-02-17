#include <raylib.h>
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <math.h>
#include <array>
#include <vector>
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "imgui_impl.h"

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

// Windows clocks in at 260, linux allows 4096
constexpr int MAX_PATH_LENGTH = 256;
std::array<char, MAX_PATH_LENGTH> pathBuffer = {0}; // Helper to avoid dynamic allocation
std::array<char, MAX_PATH_LENGTH> AssetPath(const char *assetName)
{
    snprintf(pathBuffer.data(), MAX_PATH_LENGTH, "%s/%s", DASSET_ROOT, assetName);
    return pathBuffer;
}

Camera camera = {0};
Model model;

std::chrono::time_point<std::chrono::high_resolution_clock> last;

constexpr float TICK_RATE = 1000.0f / 60.0f;
float timer = 0.0f;

lua_State *luaState;
ImGuiContext *guiContext;

// Hacky lua console
std::array<char, 256> inputBuffer;
bool scrollDown = false;
bool addCommandToHistory = false;
std::vector<std::string> history;

void main_loop()
{
    auto now = std::chrono::high_resolution_clock::now();
    float deltaMs = std::chrono::duration<float, std::milli>(now - last).count();
    last = now;
    timer += deltaMs;

    float rotation = (3.141593f * 2.0f) * (timer / 5000.0f);

    camera.position = {cosf(rotation) * 10.0f, 5.0f, sinf(rotation) * 10.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    BeginDrawing();

    ClearBackground(RAYWHITE);

    char buf[128];
    sprintf(buf, "Frame time: %f", deltaMs);
    DrawText(buf, 0, 0, 20, LIGHTGRAY);

    BeginMode3D(camera);
    DrawModel(model, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
    EndMode3D();

    RaylibImGui::Begin();
    ImGui::NewFrame();

    ImGui::Begin("Lua console");
    if (ImGui::InputText("Input", inputBuffer.data(), 256, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        addCommandToHistory = true;
        luaL_loadstring(luaState, inputBuffer.data());
        lua_pcall(luaState, 0, 0, 0);

        if (addCommandToHistory)
        {
            history.push_back(std::string(inputBuffer.data()));
        }

        inputBuffer[0] = '\0';
        ImGui::SetKeyboardFocusHere(-1);

        scrollDown = true;
    }
    ImGui::BeginChild("Output");
    for (int i = 0; i < history.size(); ++i)
    {
        ImGui::Text(history[i].c_str(), i);
    }
    if (scrollDown)
    {
        ImGui::SetScrollHereY(1.0f);
        scrollDown = false;
    }
    ImGui::EndChild();
    ImGui::End();

    ImGui::Render();
    RaylibImGui::End();

    EndDrawing();
}

static int lua_print(lua_State *state)
{
    int args = lua_gettop(state);

    std::string str = std::string(inputBuffer.data()) + ": ";
    for (int i = 1; i <= args; ++i)
    {
        if (lua_isstring(state, i))
        {
            str += lua_tostring(state, i);
        }
        else
        {
            lua_pop(state, 1);
            history.push_back(std::string("No output data"));
        }
    }

    if (!str.empty())
    {
        history.push_back(str);
    }
    addCommandToHistory = false;

    return 0;
}

int main()
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Raylib test");

    model = LoadModel(AssetPath("Bob/glTF/Bob.gltf").data());

    luaState = luaL_newstate();
    luaL_openlibs(luaState);

    // Use our own print function
    const luaL_Reg printarr[] = {{"print", lua_print}, {NULL, NULL}};
    lua_getglobal(luaState, "_G");
    luaL_setfuncs(luaState, printarr, 0);
    lua_pop(luaState, 1);

    guiContext = ImGui::CreateContext();
    last = std::chrono::high_resolution_clock::now();

    RaylibImGui::Init();
#ifdef PLATFORM_WEB
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        main_loop();
    }
#endif

    RaylibImGui::Deinit();
    CloseWindow();

    return 0;
}