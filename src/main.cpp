#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <math.h>
#include <numbers>
#include <raylib.h>
#include <stdio.h>
#include <vector>
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}
#include <entt/entt.hpp>

#include "assets.hpp"
#include "imgui_impl.h"
#include "imgui_internal.hpp" // NOT the imgui_internal from imgui
#include "lua_entt_impl.hpp"
#include "lua_imgui_impl.hpp"
#include "world.hpp"

#ifdef PLATFORM_WEB
    #include <emscripten/emscripten.h>
#endif

// Global variables for simplicity
Camera camera = {0};

std::chrono::time_point<std::chrono::high_resolution_clock> last;

lua_State* luaState;
entt::registry registry;

// Hacky lua console
std::array<char, 256> inputBuffer;
bool scrollDown = false;
bool addCommandToHistory = false;
std::vector<std::string> history;

ImGuiContext* luaContext;

void main_loop()
{
    World::Update();

    // Calculate time delta
    auto now = std::chrono::high_resolution_clock::now();
    float deltaMs = std::chrono::duration<float, std::milli>(now - last).count();
    last = now;

    UpdateCamera(&camera);

    BeginDrawing();

    ClearBackground(RAYWHITE);

    char buf[128];
    sprintf(buf, "Frame time: %f", deltaMs);
    DrawText(buf, 0, 0, 20, LIGHTGRAY);

    BeginMode3D(camera);
    World::Draw();
    EndMode3D();

    RaylibImGui::Begin();

    World::DrawImgui();

    ImGui::Begin("Lua console");
    ImGui::SameLine();
    if(ImGui::InputText("Input", inputBuffer.data(), 256, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        addCommandToHistory = true;
        luaL_loadstring(luaState, inputBuffer.data());
        lua_pcall(luaState, 0, 0, 0);

        if(addCommandToHistory)
        {
            history.push_back(std::string(inputBuffer.data()));
        }

        inputBuffer[0] = '\0';
        ImGui::SetKeyboardFocusHere(-1);

        scrollDown = true;
    }
    // Console output
    ImGui::BeginChild("Output");
    for(int i = 0; i < history.size(); ++i)
    {
        ImGui::Text(history[i].c_str(), i);
    }
    if(scrollDown)
    {
        ImGui::SetScrollHereY(1.0f);
        scrollDown = false;
    }
    ImGui::EndChild();
    ImGui::End();

    RaylibImGui::End();

    auto backupContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(luaContext);
    RaylibImGui::Begin();

    auto res = luaL_loadfile(luaState, AssetPath("lua/gui.lua").data());
    if(res != LUA_OK)
    {
        std::cerr << "Couldn't load gui.lua or error occurred";
    }
    bool guiError = false;
    if(lua_pcall(luaState, 0, 0, 0) == LUA_OK)
    {
        if(ValidStackSize(luaContext))
            RaylibImGui::End();
        else
            guiError = true;
    }
    else
    {
        guiError = true;

        std::cerr << lua_tostring(luaState, -1) << std::endl;
        lua_pop(luaState, 1);
    }

    if(guiError)
    {
        std::cerr << "Invalid imgui state after gui.lua, check all Begin and End calls"
                  << std::endl;
        ImGui::DestroyContext(luaContext);
        RaylibImGui::Init();
        luaContext = ImGui::GetCurrentContext();
    }

    ImGui::SetCurrentContext(backupContext);

    EndDrawing();
}

// Function to write to the console output window instead of stdout when `print` is used in lua
static int lua_print(lua_State* state)
{
    int args = lua_gettop(state);

    std::string str = std::string(inputBuffer.data()) + ": ";
    for(int i = 1; i <= args; ++i)
    {
        if(lua_isstring(state, i))
        {
            str += lua_tostring(state, i);
        }
        else
        {
            lua_pop(state, 1);
            history.push_back(std::string("No output data"));
        }
    }

    if(!str.empty())
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

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Raylib test");

    camera.position = {50.0f, 50.0f, 50.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    SetCameraMode(camera, CAMERA_FREE);

    // Create lua state and perform initial setup
    luaState = luaL_newstate();
    luaL_openlibs(luaState);

    RaylibImGui::Init();
    luaContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(nullptr);

    // Use our own print function
    const luaL_Reg printarr[] = {{"print", lua_print}, {NULL, NULL}};
    lua_getglobal(luaState, "_G");
    luaL_setfuncs(luaState, printarr, 0);
    lua_pop(luaState, 1);

    last = std::chrono::high_resolution_clock::now();

    LoadAssets();
    RaylibImGui::Init();

    World::Init(&registry);

    register_entt(luaState, &registry);
    LuaImgui::register_imgui(luaState);

    auto res = luaL_loadfile(luaState, AssetPath("lua/main.lua").data());
    if(res != LUA_OK)
    {
        std::cerr << "Couldn't load main.lua or error occurred";
        return 1;
    }
    lua_pcall(luaState, 0, 0, 0);

    lua_getglobal(luaState, "init");
    lua_pcall(luaState, 0, 0, 0);

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    SetTargetFPS(60);

    while(!WindowShouldClose())
    {
        main_loop();
    }
#endif

    RaylibImGui::Deinit();
    CloseWindow();

    return 0;
}