#include "lua_raylib_impl.hpp"

#include "../camera.hpp"
#include "../world.hpp"
#include "lua_register.hpp"
#include <entt/entt.hpp>
#include <raylib.h>

#include "../entity/render.hpp"
#include "../entity/transform.hpp"

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
    constexpr auto LuaGetFunc<Vector2> = [](lua_State* lua, int& i) {
        lua_getfield(lua, i, "x");
        float x = lua_tonumber(lua, -1);
        lua_getfield(lua, i, "y");
        float y = lua_tonumber(lua, -1);
        lua_pop(lua, 2);
        i++;
        return Vector2{x, y};
    };
    template<>
    constexpr auto LuaGetFunc<Vector3> = [](lua_State* lua, int& i) {
        lua_getfield(lua, i, "x");
        float x = lua_tonumber(lua, -1);
        lua_getfield(lua, i, "y");
        float y = lua_tonumber(lua, -1);
        lua_getfield(lua, i, "z");
        float z = lua_tonumber(lua, -1);
        lua_pop(lua, 3);
        i++;
        return Vector3{x, y, z};
    };
    template<>
    constexpr auto LuaGetFunc<BoundingBox> = [](lua_State* lua, int& i) {
        lua_getfield(lua, i, "min");
        int index = lua_gettop(lua);
        auto min = LuaGetFunc<Vector3>(lua, index);

        lua_getfield(lua, i, "max");
        auto max = LuaGetFunc<Vector3>(lua, index);

        lua_pop(lua, 2);
        i++;

        return BoundingBox{min, max};
    };
    template<>
    constexpr auto LuaGetFunc<Ray> = [](lua_State* lua, int& i) {
        lua_getfield(lua, i, "position");
        int index = lua_gettop(lua);
        auto position = LuaGetFunc<Vector3>(lua, index);

        lua_getfield(lua, i, "direction");
        auto direction = LuaGetFunc<Vector3>(lua, index);

        lua_pop(lua, 2);
        i++;

        return Ray{position, direction};
    };
    template<>
    constexpr auto LuaGetFunc<Color> = [](lua_State* lua, int& i) {
        lua_getfield(lua, i, "r");
        unsigned char r = lua_tointeger(lua, -1);
        lua_getfield(lua, i, "g");
        unsigned char g = lua_tointeger(lua, -1);
        lua_getfield(lua, i, "b");
        unsigned char b = lua_tointeger(lua, -1);
        lua_getfield(lua, i, "a");
        unsigned char a = lua_tointeger(lua, -1);
        lua_pop(lua, 4);
        i++;
        return Color{r, g, b, a};
    };
}

namespace LuaRaylib
{
    void Register(lua_State* lua, entt::registry* registry)
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

        QuickRegister(DrawLine3D);
        QuickRegister(DrawPoint3D);
        QuickRegister(DrawCircle3D);
        QuickRegister(DrawTriangle3D);
        // I just never use TriangleStrip...
        // QuickRegister(DrawTriangleStrip3D);
        QuickRegister(DrawCube);
        QuickRegister(DrawCubeV);
        QuickRegister(DrawCubeWires);
        QuickRegister(DrawCubeWiresV);
        QuickRegister(DrawSphere);
        QuickRegister(DrawSphereEx);
        QuickRegister(DrawSphereWires);
        QuickRegister(DrawCylinder);
        QuickRegister(DrawCylinderEx);
        QuickRegister(DrawCylinderWires);
        QuickRegister(DrawCylinderWiresEx);
        QuickRegister(DrawPlane);
        QuickRegister(DrawRay);
        QuickRegister(DrawGrid);

        LuaRegister::Register(
            lua,
            "GetMouseRay",
            +[](Vector2 mousePosition) { return GetMouseRay(mousePosition, camera); });
        LuaRegister::Register(
            lua,
            "GetWorldToScreen",
            +[](Vector3 world) { return GetWorldToScreen(world, camera); });

        LuaRegister::RegisterMember(
            lua,
            "GetRayCollision",
            registry,
            +[](entt::registry* registry, Ray ray) {
                bool hit = false;
                registry->view<Component::Render>().each([registry,
                                                          ray,
                                                          &hit](auto entity, auto render) {
                    Component::Transform transform;
                    if(auto transformPtr = registry->try_get<Component::Transform>(entity);
                       transformPtr)
                    {
                        transform = *transformPtr;
                    }
                    else
                    {
                        transform =
                            Component::Transform{Vector3(0.0f, 0.0f, 0.0f), QuaternionIdentity()};
                    }

                    const Model& model = render.model;
                    auto boundingBox = GetMeshBoundingBox(model.meshes[0]);
                    auto transformMatrix = MatrixMultiply(
                        QuaternionToMatrix(transform.rotation),
                        MatrixTranslate(
                            transform.position.x,
                            transform.position.y,
                            transform.position.z));
                    auto min = Vector3Transform(boundingBox.min, transformMatrix);
                    auto max = Vector3Transform(boundingBox.max, transformMatrix);

                    if(GetRayCollisionBox(ray, BoundingBox{min, max}).hit)
                    {
                        hit = hit || GetRayCollisionModel(ray, model).hit;
                    }
                });
                return hit;
            });
    }
}