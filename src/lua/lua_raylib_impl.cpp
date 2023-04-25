#include "lua_raylib_impl.hpp"

#include <entt/entt.hpp>
#include <external/raylib.hpp>
#include <lua/lua_register_types.hpp>
#include <optional>
#include <world.hpp>

#include <entity/camera.hpp>
#include <entity/render.hpp>
#include <entity/transform.hpp>

namespace LuaRaylib
{
    std::optional<entt::entity> GetRayCollision(entt::registry* registry, Ray ray)
    {
        std::optional<entt::entity> hitEntity;
        float closestHitDistance = std::numeric_limits<float>::max();
        registry->view<Component::Render>().each([registry, ray, &hitEntity, &closestHitDistance](
                                                     auto entity,
                                                     auto render) {
            Component::Transform transform;
            if(auto transformPtr = registry->try_get<Component::Transform>(entity); transformPtr)
                transform = *transformPtr;
            else
                transform = Component::Transform{Vector3{0.0f, 0.0f, 0.0f}, QuaternionIdentity()};

            auto fmin = std::numeric_limits<float>::lowest();
            auto fmax = std::numeric_limits<float>::max();

            Vector3 min{fmax, fmax, fmax};
            Vector3 max{fmin, fmin, fmin};

            const Model& model = render.model;
            for(uint32_t i = 0; i < model.meshCount; ++i)
            {
                auto boundingBox = GetMeshBoundingBox(model.meshes[i]);
                auto transformMatrix = MatrixMultiply(
                    QuaternionToMatrix(transform.rotation),
                    MatrixTranslate(
                        transform.position.x,
                        transform.position.y,
                        transform.position.z));

                min = Vector3Min(min, Vector3Transform(boundingBox.min, transformMatrix));
                max = Vector3Max(max, Vector3Transform(boundingBox.max, transformMatrix));
            }

            if(GetRayCollisionBox(ray, BoundingBox{min, max}).hit)
            {
                RayCollision collision = {};

                for(int i = 0; i < model.meshCount; i++)
                {
                    RayCollision meshCollision = GetRayCollisionMesh(
                        ray,
                        model.meshes[i],
                        MatrixMultiply(
                            model.transform,
                            MatrixTranslate(
                                transform.position.x,
                                transform.position.y,
                                transform.position.z)));

                    if(meshCollision.hit)
                    {
                        // Save the closest hit mesh
                        if((!collision.hit) || (collision.distance > meshCollision.distance))
                            collision = meshCollision;
                    }
                }

                if(collision.hit && collision.distance < closestHitDistance)
                {
                    hitEntity = entity;
                    closestHitDistance = collision.distance;
                }
            }
        });

        return hitEntity;
    };

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
        QuickRegister(GetMousePosition);

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

        QuickRegister(IsMouseButtonUp);
        QuickRegister(IsMouseButtonDown);
        QuickRegister(IsMouseButtonPressed);
        QuickRegister(IsMouseButtonReleased);

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
            +[](entt::registry* registry, lua_State* lua, Ray ray) -> LuaRegister::Placeholder {
                std::optional<entt::entity> hitOpt = GetRayCollision(registry, ray);
                if(hitOpt)
                    lua_pushinteger(lua, (lua_Integer)hitOpt.value());
                else
                    lua_pushnil(lua);

                return {};
            });
    }
}