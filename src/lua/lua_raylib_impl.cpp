#include "lua_raylib_impl.hpp"

#include "lua_register.hpp"
#include <raylib.h>

namespace LuaRegister
{
    template<>
    inline constexpr auto GetDefault<Vector2> = Vector2{0.0f, 0.0f};
    template<>
    inline constexpr auto GetDefault<Vector3> = Vector3{0.0f, 0.0f, 0.0f};
    template<>
    inline constexpr auto GetDefault<Vector4> = Vector4{0.0f, 0.0f, 0.0f, 0.0f};
    template<>
    inline constexpr auto GetDefault<Ray> = Ray{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

    template<>
    constexpr auto LuaSetFunc<Vector2> = [](lua_State* lua, Vector2 v) {
        lua_createtable(lua, 0, 2);
        lua_pushnumber(lua, v.x);
        lua_setfield(lua, -2, "x");
        lua_pushnumber(lua, v.y);
        lua_setfield(lua, -2, "y");
    };
    template<>
    constexpr auto LuaSetFunc<Vector3> = [](lua_State* lua, Vector3 v) {
        lua_createtable(lua, 0, 2);
        lua_pushnumber(lua, v.x);
        lua_setfield(lua, -2, "x");
        lua_pushnumber(lua, v.y);
        lua_setfield(lua, -2, "y");
        lua_pushnumber(lua, v.z);
        lua_setfield(lua, -2, "z");
    };
    template<>
    constexpr auto LuaSetFunc<Vector4> = [](lua_State* lua, Vector4 v) {
        lua_createtable(lua, 0, 2);
        lua_pushnumber(lua, v.x);
        lua_setfield(lua, -2, "x");
        lua_pushnumber(lua, v.y);
        lua_setfield(lua, -2, "y");
        lua_pushnumber(lua, v.z);
        lua_setfield(lua, -2, "z");
        lua_pushnumber(lua, v.w);
        lua_setfield(lua, -2, "w");
    };
}

namespace LuaRaylib
{
    void Register(lua_State* lua)
    {
#define QuickRegister(Func) LuaRegister::Register(lua, #Func, Func);

        // Screen
        QuickRegister(GetScreenWidth);
        QuickRegister(GetScreenHeight);
        QuickRegister(GetMonitorCount);
        QuickRegister(GetCurrentMonitor);
        QuickRegister(GetMonitorPosition);
        QuickRegister(GetMonitorWidth);
        QuickRegister(GetMonitorHeight);

        // Window control
        QuickRegister(ToggleFullscreen);
        QuickRegister(CloseWindow);

        QuickRegister(SetWindowTitle);
        QuickRegister(SetWindowPosition);
        QuickRegister(SetWindowMonitor);
        QuickRegister(SetWindowMinSize);
        QuickRegister(SetWindowSize);

        QuickRegister(ShowCursor);
        QuickRegister(HideCursor);
        QuickRegister(IsCursorHidden);
        QuickRegister(EnableCursor);
        QuickRegister(DisableCursor);
        QuickRegister(IsCursorOnScreen);

        QuickRegister(SetTargetFPS);
        QuickRegister(GetFPS);
        QuickRegister(GetFrameTime);
        QuickRegister(GetTime);

        // Window getters
        QuickRegister(IsWindowMinimized);
        QuickRegister(IsWindowMaximized);
        QuickRegister(IsWindowFocused);
        QuickRegister(IsWindowResized);
        QuickRegister(GetWindowPosition);
        QuickRegister(GetWindowScaleDPI);
        QuickRegister(GetMonitorName);
    }
}