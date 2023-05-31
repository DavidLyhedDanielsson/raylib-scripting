#pragma once

#include <external/imgui.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register.hpp>

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
    inline constexpr auto GetDefault<Color> = Color{1, 1, 1, 1};
    template<>
    inline constexpr auto GetDefault<Rectangle> = Rectangle{0.0f, 0.0f, 0.0f, 0.0f};

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
        lua_createtable(lua, 0, 3);
        lua_pushnumber(lua, v.x);
        lua_setfield(lua, -2, "x");
        lua_pushnumber(lua, v.y);
        lua_setfield(lua, -2, "y");
        lua_pushnumber(lua, v.z);
        lua_setfield(lua, -2, "z");
    };
    template<>
    constexpr auto LuaSetFunc<Vector4> = [](lua_State* lua, Vector4 v) {
        lua_createtable(lua, 0, 4);
        lua_pushnumber(lua, v.x);
        lua_setfield(lua, -2, "x");
        lua_pushnumber(lua, v.y);
        lua_setfield(lua, -2, "y");
        lua_pushnumber(lua, v.z);
        lua_setfield(lua, -2, "z");
        lua_pushnumber(lua, v.w);
        lua_setfield(lua, -2, "w");
    };
    template<>
    constexpr auto LuaSetFunc<Ray> = [](lua_State* lua, Ray v) {
        lua_createtable(lua, 0, 2);
        lua_pushstring(lua, "position");
        LuaSetFunc<Vector3>(lua, v.position);
        lua_settable(lua, -3);
        lua_pushstring(lua, "direction");
        LuaSetFunc<Vector3>(lua, v.direction);
        lua_settable(lua, -3);
    };
    template<>
    constexpr auto LuaSetFunc<Rectangle> = [](lua_State* lua, Rectangle rec) {
        lua_createtable(lua, 0, 4);
        lua_pushnumber(lua, rec.x);
        lua_setfield(lua, -2, "x");
        lua_pushnumber(lua, rec.y);
        lua_setfield(lua, -2, "y");
        lua_pushnumber(lua, rec.width);
        lua_setfield(lua, -2, "width");
        lua_pushnumber(lua, rec.height);
        lua_setfield(lua, -2, "height");
    };

    template<>
    constexpr auto LuaGetFunc<Vector2> = [](lua_State* lua, int i) {
        assert(i >= 1); // Can't use relative since stack is modified
        lua_getfield(lua, i, "x");
        float x = lua_tonumber(lua, -1);
        lua_getfield(lua, i, "y");
        float y = lua_tonumber(lua, -1);
        lua_pop(lua, 2);
        return Vector2{x, y};
    };
    template<>
    constexpr auto LuaGetFunc<Vector3> = [](lua_State* lua, int i) {
        assert(i >= 1); // Can't use relative since stack is modified
        lua_getfield(lua, i, "x");
        float x = lua_tonumber(lua, -1);
        lua_getfield(lua, i, "y");
        float y = lua_tonumber(lua, -1);
        lua_getfield(lua, i, "z");
        float z = lua_tonumber(lua, -1);
        lua_pop(lua, 3);
        return Vector3{x, y, z};
    };
    template<>
    constexpr auto LuaGetFunc<BoundingBox> = [](lua_State* lua, int i) {
        assert(i >= 1); // Can't use relative since stack is modified
        lua_getfield(lua, i, "min");
        int index = lua_gettop(lua);
        auto min = LuaGetFunc<Vector3>(lua, index);

        lua_getfield(lua, i, "max");
        auto max = LuaGetFunc<Vector3>(lua, index);

        lua_pop(lua, 2);

        return BoundingBox{min, max};
    };
    template<>
    constexpr auto LuaGetFunc<Ray> = [](lua_State* lua, int i) {
        assert(i >= 1); // Can't use relative since stack is modified
        lua_getfield(lua, i, "position");
        int index = lua_gettop(lua);
        auto position = LuaGetFunc<Vector3>(lua, index);

        lua_getfield(lua, i, "direction");
        auto direction = LuaGetFunc<Vector3>(lua, index + 1);

        lua_pop(lua, 2);

        return Ray{position, direction};
    };
    template<>
    constexpr auto LuaGetFunc<Rectangle> = [](lua_State* lua, int i) {
        assert(i >= 1); // Can't use relative since stack is modified
        lua_getfield(lua, i, "x");
        float x = lua_tonumber(lua, -1);
        lua_getfield(lua, i, "y");
        float y = lua_tonumber(lua, -1);
        lua_getfield(lua, i, "width");
        float width = lua_tonumber(lua, -1);
        lua_getfield(lua, i, "height");
        float height = lua_tonumber(lua, -1);
        lua_pop(lua, 4);
        return Rectangle{x, y, width, height};
    };
    template<>
    constexpr auto LuaGetFunc<Color> = [](lua_State* lua, int i) {
        assert(i >= 1); // Can't use relative since stack is modified
        lua_getfield(lua, i, "r");
        unsigned char r = lua_tointeger(lua, -1);
        lua_getfield(lua, i, "g");
        unsigned char g = lua_tointeger(lua, -1);
        lua_getfield(lua, i, "b");
        unsigned char b = lua_tointeger(lua, -1);
        lua_getfield(lua, i, "a");
        unsigned char a = lua_tointeger(lua, -1);
        lua_pop(lua, 4);
        return Color{r, g, b, a};
    };

    template<>
    inline auto GetDefault<ImVec2> = ImVec2(0.0f, 0.0f);
    template<>
    inline auto GetDefault<ImVec4> = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    template<>
    constexpr auto LuaSetFunc<ImVec2> = [](lua_State* lua, ImVec2 v) {
        lua_createtable(lua, 0, 2);
        lua_pushnumber(lua, v.x);
        lua_setfield(lua, -2, "x");
        lua_pushnumber(lua, v.y);
        lua_setfield(lua, -2, "y");
    };

    template<>
    constexpr auto LuaSetFunc<ImVec4> = [](lua_State* lua, ImVec4 v) {
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

    template<>
    constexpr auto LuaGetFunc<ImVec2> = [](lua_State* lua, int i) {
        assert(i >= 1); // Can't use relative since stack is modified
        lua_getfield(lua, i, "x");
        float x = lua_tonumber(lua, -1);
        lua_getfield(lua, i, "y");
        float y = lua_tonumber(lua, -1);
        lua_pop(lua, 2);
        return ImVec2{x, y};
    };

    template<>
    constexpr auto LuaGetFunc<ImVec4> = [](lua_State* lua, int i) {
        assert(i >= 1); // Can't use relative since stack is modified
        lua_getfield(lua, i, "x");
        float x = lua_tonumber(lua, -1);
        lua_getfield(lua, i, "y");
        float y = lua_tonumber(lua, -1);
        lua_getfield(lua, i, "z");
        float z = lua_tonumber(lua, -1);
        lua_getfield(lua, i, "w");
        float w = lua_tonumber(lua, -1);
        lua_pop(lua, 4);
        return ImVec4{x, y, z, w};
    };
}
