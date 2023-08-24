#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <math.h>
#include <numbers>
#include <stdio.h>
#include <vector>

#include <assets.hpp>
#include <component/camera.hpp>
#include <component/transform.hpp>
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
#include <lua_impl/lua_world_impl.hpp>
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

static std::chrono::steady_clock::time_point last;

static lua_State* luaState;

// Lua coroutine support, they simply sleep for the yielded amount of time
struct LuaThread
{
    int refIndex;
    std::chrono::steady_clock::time_point sleepStart;
    dmilliseconds sleepTime;
};
static std::vector<LuaThread> luaThreads;
static entt::registry registry;

#ifndef SKIP_CONSOLE
// Hacky lua console, allows command execution and output to be shown
static std::array<char, 256> inputBuffer;
static bool scrollDown = false;
static bool addCommandToHistory = false;
static std::vector<std::string> history;
#endif

// Support switching which lua file is currently executed
static std::string currentLuaFile = "menu.lua";

void main_loop()
{
    Profiling::NewFrame();

    {
        PROFILE_SCOPE("RunCoroutines");
        // Update running coroutines
        auto threadNow = std::chrono::steady_clock::now();
        for(uint32_t i = 0; i < luaThreads.size(); ++i)
        {
            PROFILE_SCOPE(i);

            LuaThread& thread = luaThreads[i];

            // Sleep time has not yet been reached; do nothing
            auto sleepGoal = thread.sleepStart + thread.sleepTime;
            if(sleepGoal > threadNow)
                continue;

            // Thread state is stored in the registry
            lua_rawgeti(luaState, LUA_REGISTRYINDEX, thread.refIndex);
            lua_State* luaThread = lua_tothread(luaState, -1);
            lua_getglobal(luaThread, "func");

            int nres = 0;
            lua_Integer ret;
            {
                PROFILE_SCOPE("lua_resume");
                ret = lua_resume(luaThread, luaState, 0, &nres);
            }
            if(ret == LUA_OK)
            {
                // Coroutine returned (it didn't yield), so it is done executing
                luaL_unref(luaState, LUA_REGISTRYINDEX, thread.refIndex);
                luaThreads.erase(luaThreads.begin() + i);
                --i;
            }
            else if(ret == LUA_YIELD)
            {
                // Coroutine yielded the number of ms it should sleep
                assert(lua_isnumber(luaThread, nres));
                thread.sleepStart = std::chrono::steady_clock::now();
                thread.sleepTime = dmilliseconds{lua_tointeger(luaThread, nres)};
            }
            else
            {
                // Error :(
                // Coroutine execution is stopped
                std::cerr << "Error when executing coroutine: " << lua_tostring(luaThread, -1)
                          << std::endl;
                luaL_traceback(luaState, luaThread, nullptr, 0);
                if(lua_isstring(luaState, -1))
                {
                    std::cerr << lua_tostring(luaState, -1) << std::endl;
                }

                luaL_unref(luaState, LUA_REGISTRYINDEX, thread.refIndex);
                luaThreads.erase(luaThreads.begin() + i);
                --i;
            }
        }
    }

    // This macro calls the given function and profiles it
    PROFILE_CALL(World::Update);

    {
        PROFILE_SCOPE("BeginDrawing");
        BeginDrawing();
        ClearBackground(DARKGRAY);
    }

    //// Uncomment to show FPS
    // auto now = std::chrono::steady_clock::now();
    // dmilliseconds deltaMs = now - last;
    // last = now;
    // char buf[128];
    // sprintf(buf, "Frame time: %f", deltaMs.count());
    // DrawText(buf, 0, 64, 20, LIGHTGRAY);

    // Convert camera entity to the Raylib camera struct
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

    // Reload the current lua file every frame. This is useful for debugging/development but
    // should be disabled when the game is "released".
    // luaError signals that no lua functions can be called since some error occurred during dofile
    bool luaError = false;
#ifndef DNO_LUA_RELOAD
    Profiling::ProfileCall("Load currentLuaFile", [&]() {
        auto res = luaL_dofile(luaState, LuaFilePath(currentLuaFile.c_str()).data());
        if(res != LUA_OK)
        {
            std::cerr << "Couldn't load " << currentLuaFile << " or error occurred" << std::endl;
            std::cerr << lua_tostring(luaState, -1) << std::endl;
            luaError = true;
        }
    });
#endif

    //// 3D rendering
    PROFILE_CALL(BeginMode3D, camera);
    PROFILE_CALL(World::Draw);

    // Call the raylib3D function in the currently loaded lua file. It needs to be separated from
    // the 2D rendering because Raylib requires 3D code to be wrapped in BeginMode3D/EndMode3D
    if(!luaError)
    {
        lua_getglobal(luaState, "raylib3D");
        if(lua_isfunction(luaState, -1))
        {
            Profiling::ProfileCall("Execute currentLuaFile::raylib3D", [&]() {
                if(lua_pcall(luaState, 0, 0, 0) != LUA_OK)
                {
                    std::cerr << "Error when executing " << currentLuaFile << std::endl;
                    std::cerr << lua_tostring(luaState, -1) << std::endl;
                }
            });
        }
        else
            lua_pop(luaState, 1);
    }

    PROFILE_CALL(EndMode3D);

    //// 2D rendering
    if(!luaError)
    {
        lua_getglobal(luaState, "raylib2D");
        if(lua_isfunction(luaState, -1))
        {
            Profiling::ProfileCall("Execute currentLuaFile::raylib2D", [&]() {
                if(lua_pcall(luaState, 0, 0, 0) != LUA_OK)
                {
                    std::cerr << "Error when executing " << currentLuaFile << std::endl;
                    std::cerr << lua_tostring(luaState, -1) << std::endl;
                }
            });
        }
        else
            lua_pop(luaState, 1);
    }

    PROFILE_CALL(RaylibImGui::Begin);

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

#ifndef SKIP_CONSOLE
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
#endif

    if(!luaError)
    {
        lua_getglobal(luaState, "imgui");
        if(lua_isfunction(luaState, -1))
        {
            Profiling::ProfileCall("Execute lua::imgui", [&]() {
                if(lua_pcall(luaState, 0, 0, 0) != LUA_OK)
                {
                    std::cerr << "Error when executing lua::imgui:" << std::endl;
                    std::cerr << lua_tostring(luaState, -1) << std::endl;
                    lua_pop(luaState, 1);
                    ErrorCheckEndWindowRecover();
                }
            });
        }
        else
            lua_pop(luaState, 1);
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

    Profiling::Draw();

    PROFILE_CALL(RaylibImGui::End);

    if(!ImGui::GetIO().WantCaptureMouse && IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
        UpdateCamera(&camera, CAMERA_THIRD_PERSON);

    // Finally, save the current camera into the camera entity
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

#ifndef SKIP_CONSOLE
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
#endif

static bool keepRunning = true;
void Register()
{
    // Set lua PATH. Works the same as the PATH system variable in Windows and *nix
    {
        lua_getglobal(luaState, "package");
        lua_getfield(luaState, -1, "path");
        std::string path = lua_tostring(luaState, -1);
        lua_pop(luaState, 1);

        if(!path.empty())
            path += ";";
        path += DASSET_ROOT;
        path += "/lua/?.lua";
        lua_pushstring(luaState, path.c_str());
        lua_setfield(luaState, -2, "path");
        lua_pop(luaState, 1);
    }

    // Register lua functions to work with coroutines
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
        [](lua_State*) {
            luaThreads.clear();
            return 0;
        },
        0);
    lua_setglobal(luaState, "StopAllThreads");

    lua_pushcclosure(
        luaState,
        +[](lua_State* lua) {
            lua_pushnil(lua);
            lua_setglobal(lua, "raylib2D");
            lua_pushnil(lua);
            lua_setglobal(lua, "raylib3D");
            lua_pushnil(lua);
            lua_setglobal(lua, "imgui");

            const char* newFile = lua_tostring(lua, -1);
            auto res = luaL_dofile(luaState, LuaFilePath(newFile).data());
            if(res != LUA_OK)
            {
                std::cerr << "Couldn't load " << newFile << " or error occurred ";
                res = luaL_dofile(luaState, LuaFilePath(currentLuaFile.c_str()).data());
                assert(res == LUA_OK);
                return 0;
            }

            lua_getglobal(lua, "init");
            if(lua_isfunction(lua, -1))
                lua_pcall(lua, 0, 0, 0);
            else
                lua_pop(lua, 1);

            currentLuaFile = newFile;

            return 0;
        },
        0);
    lua_setglobal(luaState, "SetLuaFile");

    lua_pushcclosure(
        luaState,
        +[](lua_State*) {
            keepRunning = false;
            return 0;
        },
        0);
    lua_setglobal(luaState, "Exit");
}

int main()
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    //// Init Raylib
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Raylib + ImGui + EnTT");
    RaylibImGui::Init();

    //// Init lua
    // Create lua state and perform initial setup
    luaState = luaL_newstate();
    luaL_openlibs(luaState);

#ifndef SKIP_CONSOLE
    // Redirect output to the console to view it in-game
    const luaL_Reg printarr[] = {{"print", lua_print}, {NULL, NULL}};
    lua_getglobal(luaState, "_G");
    luaL_setfuncs(luaState, printarr, 0);
    lua_pop(luaState, 1);
#endif

    // Register lua functions related to each piece of functionality
    Register();
    LuaWorld::Register(luaState);
    LuaAsset::Register(luaState);
    LuaEntt::Register(luaState, &registry);
    LuaImGui::Register(luaState);
    LuaImGuizmo::Register(luaState, &registry);
    LuaRaylib::Register(luaState, &registry);

    //// General init
    last = std::chrono::steady_clock::now();

    LoadAssets();
    World::state.registry = &registry;
    World::state.lua = luaState;
    World::Init();

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

    // Load this file after all the setup since it might use the world, camera, or something else
    // The file must be valid at startup
    auto res = luaL_loadfile(luaState, LuaFilePath(currentLuaFile.c_str()).data());
    if(res != LUA_OK)
    {
        std::cerr << "Couldn't load " << currentLuaFile << " or error occurred ";
        return 1;
    }
    lua_pcall(luaState, 0, 0, 0);

    lua_getglobal(luaState, "init");
    lua_pcall(luaState, 0, 0, 0);

#ifdef DNO_LUA_RELOAD
    std::cout << "Not reloading lua files (DNO_LUA_RELOAD defined)" << std::endl;
#endif

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    // TODO: Changing this breaks the assumption that the game will run at >= 60 FPS in
    // World::Update
    SetTargetFPS(60);

    while(!WindowShouldClose() && keepRunning)
    {
        main_loop();
    }
#endif

    RaylibImGui::Deinit();
    CloseWindow();

    return 0;
}