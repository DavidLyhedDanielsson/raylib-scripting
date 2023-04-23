#include "lua_raylib_impl.hpp"

#include <entt/entt.hpp>
#include <lua/lua_register_types.hpp>
#include <world.hpp>

#include <entity/camera.hpp>
#include <entity/render.hpp>
#include <entity/transform.hpp>

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
        QuickRegister(GetRenderWidth);
        QuickRegister(GetRenderHeight);

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

        LuaRegister::RegisterMember(
            lua,
            "GetMouseRay",
            registry,
            +[](entt::registry* registry, Vector2 mousePosition) {
                Camera3D camera;
                registry->view<Component::Camera, Component::Transform>().each(
                    [&](const Component::Camera& c, const Component::Transform& t) {
                        camera = c.CreateRaylibCamera(t.position);
                    });
                return GetMouseRay(mousePosition, camera);
            });
        LuaRegister::RegisterMember(
            lua,
            "GetWorldToScreen",
            registry,
            +[](entt::registry* registry, Vector3 world) {
                Camera3D camera;
                registry->view<Component::Camera, Component::Transform>().each(
                    [&](const Component::Camera& c, const Component::Transform& t) {
                        camera = c.CreateRaylibCamera(t.position);
                    });
                return GetWorldToScreen(world, camera);
            });

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
                            Component::Transform{Vector3{0.0f, 0.0f, 0.0f}, QuaternionIdentity()};
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