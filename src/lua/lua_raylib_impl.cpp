#include "lua_raylib_impl.hpp"

#include <entt/entt.hpp>
#include <external/raylib.hpp>
#include <lua/lua_register_types.hpp>
#include <optional>
#include <world.hpp>

#include <entity/camera.hpp>
#include <entity/render.hpp>
#include <entity/transform.hpp>

// Taken from Raylib's source and modified
BoundingBox GetModelBoundingBox(Model model, Matrix transform)
{
    auto fmin = std::numeric_limits<float>::lowest();
    auto fmax = std::numeric_limits<float>::max();
    Vector3 minVertex{fmax, fmax, fmax};
    Vector3 maxVertex{fmin, fmin, fmin};

    for(int i = 0; i < model.meshCount; ++i)
    {
        Mesh mesh = model.meshes[i];
        if(mesh.vertices != NULL)
        {
            for(int i = 0; i < mesh.vertexCount; i++)
            {
                minVertex = Vector3Min(
                    minVertex,
                    Vector3Transform(
                        {mesh.vertices[i * 3], mesh.vertices[i * 3 + 1], mesh.vertices[i * 3 + 2]},
                        transform));
                maxVertex = Vector3Max(
                    maxVertex,
                    Vector3Transform(
                        {mesh.vertices[i * 3], mesh.vertices[i * 3 + 1], mesh.vertices[i * 3 + 2]},
                        transform));
            }
        }
    }

    // Create the bounding box
    BoundingBox box = {0};
    box.min = minVertex;
    box.max = maxVertex;

    return box;
}

std::optional<entt::entity> GetRayCollision(entt::registry* registry, Ray ray)
{
    std::optional<entt::entity> hitEntity;
    float closestHitDistance = std::numeric_limits<float>::max();
    auto castAgainstEntity = [&](auto entity, auto render) {
        Component::Transform transformComponent;
        if(auto transformPtr = registry->try_get<Component::Transform>(entity); transformPtr)
            transformComponent = *transformPtr;
        else
            transformComponent =
                Component::Transform{Vector3{0.0f, 0.0f, 0.0f}, Vector3{0.0f, 0.0f, 0.0f}};

        auto transform = MatrixMultiply(
            MatrixRotateZYX(transformComponent.rotation),
            MatrixTranslate(
                transformComponent.position.x,
                transformComponent.position.y,
                transformComponent.position.z));

        if(GetRayCollisionBox(ray, GetModelBoundingBox(render.model, transform)).hit)
        {
            RayCollision collision = {};

            for(int i = 0; i < render.model.meshCount; i++)
            {
                RayCollision meshCollision =
                    GetRayCollisionMesh(ray, render.model.meshes[i], transform);

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
    };
    registry->view<Component::Render>().each(castAgainstEntity);

    return hitEntity;
};

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