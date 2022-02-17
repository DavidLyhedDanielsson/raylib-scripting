#include <raylib.h>
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <math.h>
#include <array>
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

    ImGui::Begin("Hello");
    ImGui::End();

    ImGui::Render();
    RaylibImGui::End();

    EndDrawing();
}

int main()
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Raylib test");

    model = LoadModel(AssetPath("Bob/glTF/Bob.gltf").data());

    luaState = luaL_newstate();
    luaL_openlibs(luaState);
    luaL_loadstring(luaState, "print(\"Hello world\")");
    lua_pcall(luaState, 0, 0, 0);

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