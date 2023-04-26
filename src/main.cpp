#include <array>
#include <chrono>
#include <cmath>
#include <external/raylib.hpp>
#include <imgui/imgui_internal.hpp> // NOT the imgui_internal from imgui
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
#include "imgui/imgui_impl.hpp"
#include "lua/lua_asset_impl.hpp"
#include "lua/lua_entity_reflection_impl.hpp"
#include "lua/lua_entt_impl.hpp"
#include "lua/lua_imgui_impl.hpp"
#include "lua/lua_imguizmo_impl.hpp"
#include "lua/lua_raylib_impl.hpp"
#include "world.hpp"
#include <entity/camera.hpp>
#include <entity/transform.hpp>

// #include <Windows.h>
#ifdef PLATFORM_WEB
    #include <emscripten/emscripten.h>
#endif

// Global variables for simplicity

std::chrono::time_point<std::chrono::high_resolution_clock> last;

lua_State* luaState;
entt::registry registry;

// Hacky lua console
std::array<char, 256> inputBuffer;
bool scrollDown = false;
bool addCommandToHistory = false;
std::vector<std::string> history;

void main_loop()
{
    World::Update();

    // Calculate time delta
    auto now = std::chrono::high_resolution_clock::now();
    float deltaMs = std::chrono::duration<float, std::milli>(now - last).count();
    last = now;

    BeginDrawing();

    ClearBackground(RAYWHITE);

    char buf[128];
    sprintf(buf, "Frame time: %f", deltaMs);
    DrawText(buf, 0, 0, 20, LIGHTGRAY);

    auto res = luaL_loadfile(luaState, AssetPath("lua/gui.lua").data());
    bool guiError = false;
    if(res != LUA_OK)
    {
        std::cerr << "Couldn't load gui.lua or error occurred" << std::endl;
        std::cerr << lua_tostring(luaState, -1) << std::endl;
        guiError = true;
    }
    if(lua_pcall(luaState, 0, 0, 0) != LUA_OK)
    {
        std::cerr << "Error when executing gui.lua" << std::endl;
        std::cerr << lua_tostring(luaState, -1) << std::endl;
        guiError = true;
    }

    Camera camera;
    for(auto [entity, transform, cameraComponent] :
        registry.view<Component::Transform, Component::Camera>().each())
    {
        camera = Camera{
            .position = transform.position,
            .target = cameraComponent.target,
            .up = cameraComponent.up,
            .fovy = cameraComponent.fovy,
            .projection = cameraComponent.projection,
        };
    }

    BeginMode3D(camera);
    World::Draw();

    if(!guiError)
    {
        lua_getglobal(luaState, "raylib");
        if(lua_pcall(luaState, 0, 0, 0) != LUA_OK)
        {
            std::cerr << "Error when executing gui.lua:raylib" << std::endl;
            std::cerr << lua_tostring(luaState, -1) << std::endl;
            guiError = true;
        }
    }

    EndMode3D();

    RaylibImGui::Begin();

    World::DrawImgui();
    // Imgui might update the camera
    for(auto [entity, transform, cameraComponent] :
        registry.view<Component::Transform, Component::Camera>().each())
    {
        camera = Camera{
            .position = transform.position,
            .target = cameraComponent.target,
            .up = cameraComponent.up,
            .fovy = cameraComponent.fovy,
            .projection = cameraComponent.projection,
        };
    }

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
    for(uint32_t i = 0; i < history.size(); ++i)
    {
        ImGui::Text(history[i].c_str(), i);
    }

    static size_t last_history_size = 0;
    if(scrollDown || last_history_size != history.size())
    {
        ImGui::SetScrollHereY(1.0f);
        scrollDown = false;
    }
    last_history_size = history.size();
    ImGui::EndChild();
    ImGui::End();

    lua_getglobal(luaState, "imgui");
    if(!lua_pcall(luaState, 0, 0, 0) == LUA_OK)
    {
        std::cerr << lua_tostring(luaState, -1) << std::endl;
        lua_pop(luaState, 1);
        ErrorCheckEndWindowRecover();
    }

    // Camera might be modified by lua
    for(auto [entity, transform, cameraComponent] :
        registry.view<Component::Transform, Component::Camera>().each())
    {
        camera = Camera{
            .position = transform.position,
            .target = cameraComponent.target,
            .up = cameraComponent.up,
            .fovy = cameraComponent.fovy,
            .projection = cameraComponent.projection,
        };
    }

    RaylibImGui::End();

    if(!ImGui::GetIO().WantCaptureMouse && IsMouseButtonDown(1))
        UpdateCamera(&camera, CAMERA_THIRD_PERSON);

    for(auto [entity, transform, cameraComponent] :
        registry.view<Component::Transform, Component::Camera>().each())
    {
        transform.position = camera.position;
        cameraComponent = {
            .target = camera.target,
            .up = camera.up,
            .fovy = camera.fovy,
            .projection = camera.projection,
        };
    }

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
    const int screenWidth = 1280;
    const int screenHeight = 720;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(screenWidth, screenHeight, "Raylib test");
    RaylibImGui::Init();

    // Create lua state and perform initial setup
    luaState = luaL_newstate();
    luaL_openlibs(luaState);
    // Use our own print function
    const luaL_Reg printarr[] = {{"print", lua_print}, {NULL, NULL}};
    lua_getglobal(luaState, "_G");
    luaL_setfuncs(luaState, printarr, 0);
    lua_pop(luaState, 1);

    last = std::chrono::high_resolution_clock::now();

    LoadAssets();

    World::Init(&registry);

    Vector3 cameraStartPosition = {
        50.0f,
        50.0f,
        50.0f,
    };
    auto cameraComponent = Component::Camera{
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
    auto cameraEntity = registry.create();
    registry.emplace<Component::Camera>(cameraEntity, cameraComponent);
    registry.emplace<Component::Transform>(
        cameraEntity,
        Component::Transform{
            .position{10.0f, 10.0f, 10.0f},
            .rotation{0.0f, 0.0f, 0.0f},
        });

    LuaAsset::Register(luaState);
    LuaEntt::Register(luaState, &registry);
    LuaImGui::Register(luaState);
    LuaImGuizmo::Register(luaState, &registry);
    LuaRaylib::Register(luaState, &registry);
    LuaEntityReflection::Register(luaState, &registry);

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