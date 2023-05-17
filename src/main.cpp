#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <math.h>
#include <numbers>
#include <stdio.h>
#include <vector>

#include <assets.hpp>
#include <entity/camera.hpp>
#include <entity/transform.hpp>
#include <entt/entt.hpp>
#include <external/imgui.hpp>
#include <external/lua.hpp>
#include <external/raylib.hpp>
#include <imgui_error_check.hpp>
#include <lua_impl/lua_asset_impl.hpp>
#include <lua_impl/lua_entt_impl.hpp>
#include <lua_impl/lua_imgui_impl.hpp>
#include <lua_impl/lua_imguizmo_impl.hpp>
#include <lua_impl/lua_raylib_impl.hpp>
#include <profiling.hpp>
#include <raylib_imgui.hpp>
#include <world.hpp>

// #include <Windows.h>
#ifdef PLATFORM_WEB
    #include <emscripten/emscripten.h>
#endif

// Global variables for simplicity
using dnanoseconds = std::chrono::duration<double, std::nano>;
using dmicroseconds = std::chrono::duration<double, std::micro>;
using dmilliseconds = std::chrono::duration<double, std::milli>;
using dseconds = std::chrono::duration<double>;

std::chrono::steady_clock::time_point last;

lua_State* luaState;
struct LuaThread
{
    int refIndex;
    std::chrono::steady_clock::time_point sleepStart;
    dmilliseconds sleepTime;
};
static std::vector<LuaThread> luaThreads;
entt::registry registry;

// Hacky lua console
std::array<char, 256> inputBuffer;
bool scrollDown = false;
bool addCommandToHistory = false;
std::vector<std::string> history;

void main_loop()
{
    Profiling::NewFrame();

    auto threadNow = std::chrono::steady_clock::now();
    for(uint32_t i = 0; i < luaThreads.size(); ++i)
    {
        LuaThread& thread = luaThreads[i];

        auto sleepGoal = thread.sleepStart + thread.sleepTime;
        if(sleepGoal > threadNow)
            continue;

        assert(lua_rawgeti(luaState, LUA_REGISTRYINDEX, thread.refIndex) == LUA_TTHREAD);
        lua_State* luaThread = lua_tothread(luaState, -1);
        assert(lua_getglobal(luaThread, "func") == LUA_TFUNCTION);

        int nres = 0;
        auto ret = lua_resume(luaThread, luaState, 0, &nres);
        if(ret == LUA_OK)
        {
            luaL_unref(luaState, LUA_REGISTRYINDEX, thread.refIndex);
            luaThreads.erase(luaThreads.begin() + i);
            --i;
        }
        else if(ret == LUA_YIELD)
        {
            assert(lua_isnumber(luaThread, nres));
            thread.sleepStart = std::chrono::steady_clock::now();
            thread.sleepTime = dmilliseconds{lua_tointeger(luaThread, nres)};
        }
        else
        {
            assert(false);
        }
    }
    // Not sure about this syntax but let's see
    PROFILE_CALL(World::Update, luaState);

    // Calculate time delta
    auto now = std::chrono::steady_clock::now();
    dmilliseconds deltaMs = now - last;
    last = now;

    BeginDrawing();

    ClearBackground(DARKGRAY);

    char buf[128];
    sprintf(buf, "Frame time: %f", deltaMs);
    DrawText(buf, 0, 0, 20, LIGHTGRAY);

    Profiling::Start("Load editor.lua");
    auto res = luaL_loadfile(luaState, AssetPath("lua/editor.lua").data());
    bool guiError = false;
    if(res != LUA_OK)
    {
        std::cerr << "Couldn't load editor.lua or error occurred" << std::endl;
        std::cerr << lua_tostring(luaState, -1) << std::endl;
        guiError = true;
    }
    Profiling::End();
    Profiling::ProfileCall("Execute editor.lua", [&]() {
        if(!guiError && lua_pcall(luaState, 0, 0, 0) != LUA_OK)
        {
            std::cerr << "Error when executing editor.lua" << std::endl;
            std::cerr << lua_tostring(luaState, -1) << std::endl;
            guiError = true;
        }
    });

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

    PROFILE_CALL(BeginMode3D, camera);
    PROFILE_CALL(World::Draw);

    Profiling::ProfileCall("Execute editor.lua::raylib", [&]() {
        if(!guiError)
        {
            lua_getglobal(luaState, "raylib");
            if(lua_pcall(luaState, 0, 0, 0) != LUA_OK)
            {
                std::cerr << "Error when executing editor.lua:raylib" << std::endl;
                std::cerr << lua_tostring(luaState, -1) << std::endl;
                guiError = true;
            }
        }
    });

    PROFILE_CALL(EndMode3D);

    PROFILE_CALL(RaylibImGui::Begin);

    PROFILE_CALL(World::DrawImgui);

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

    ImGui::SetNextWindowPos({400.0f, 400.0f}, ImGuiCond_Once);
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

    Profiling::ProfileCall("Execute editor.lua::imgui", [&]() {
        lua_getglobal(luaState, "imgui");
        if(lua_pcall(luaState, 0, 0, 0) != LUA_OK)
        {
            std::cerr << lua_tostring(luaState, -1) << std::endl;
            lua_pop(luaState, 1);
            ErrorCheckEndWindowRecover();
        }
    });

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

    Profiling::Draw();

    PROFILE_CALL(RaylibImGui::End);

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

    PROFILE_CALL(EndDrawing);

    Profiling::EndFrame();
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

    lua_getglobal(luaState, "package");

    lua_getfield(luaState, -1, "path");
    std::string val = lua_tostring(luaState, -1);
    lua_pop(luaState, 1);

    if(!val.empty())
        val += ";";
    val += DASSET_ROOT;
    val += "/lua/?.lua";
    lua_pushstring(luaState, val.c_str());
    lua_setfield(luaState, -2, "path");
    lua_pop(luaState, 1);

    lua_pushcclosure(
        luaState,
        [](lua_State* lua) {
            lua_State* thread = lua_newthread(lua);

            assert(lua_isthread(lua, -1));
            assert(lua_isfunction(lua, -2));
            lua_rotate(lua, -2, 1);
            assert(lua_isfunction(lua, -1));
            lua_xmove(lua, thread, 1);
            assert(lua_isfunction(thread, -1));
            lua_setglobal(thread, "func");

            assert(lua_isthread(lua, -1));
            int ref = luaL_ref(lua, LUA_REGISTRYINDEX);
            luaThreads.push_back(LuaThread{
                .refIndex = ref,
                .sleepStart = std::chrono::steady_clock::now(),
                .sleepTime{0},
            });

            return 0;
        },
        0);
    lua_setglobal(luaState, "RegisterThread");
    lua_pushcclosure(
        luaState,
        [](lua_State* lua) {
            luaThreads.clear();
            return 0;
        },
        0);
    lua_setglobal(luaState, "StopAllThreads");

    last = std::chrono::steady_clock::now();

    LoadAssets();

    World::Init(&registry);

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

    World::Register(luaState);
    LuaAsset::Register(luaState);
    LuaEntt::Register(luaState, &registry);
    LuaImGui::Register(luaState);
    LuaImGuizmo::Register(luaState, &registry);
    LuaRaylib::Register(luaState, &registry);

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