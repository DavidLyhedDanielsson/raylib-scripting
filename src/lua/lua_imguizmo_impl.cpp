#include "lua_imguizmo_impl.hpp"

#include "lua_register.hpp"
#include <imgui.h>
// Needs to be after imgui
#include <ImGuizmo.h>
#include <config.h> // TODO: Path is terrible :(
#include <entity/camera.hpp>
#include <entity/transform.hpp>
#include <entt/entt.hpp>
#include <external/raylib.hpp>

#define LuaImguizmoQuickRegister(X) LuaRegister::Register(lua, #X, ImGuizmo::X)

namespace LuaImGuizmo
{
    void Register(lua_State* lua, entt::registry* registry)
    {
        using namespace LuaRegister;

        RegisterMember(
            lua,
            "Gizmo",
            registry,
            +[](entt::registry* registry, lua_State* state, lua_Integer entity) {
                if(!registry->valid((entt::entity)entity))
                    return;

                auto transformPtr = registry->try_get<Component::Transform>((entt::entity)entity);
                Camera camera;
                registry->view<Component::Camera, Component::Transform>().each(
                    [&](entt::entity, Component::Camera c, Component::Transform t) {
                        camera = c.CreateRaylibCamera(t.position);
                    });

                float zero[3] = {0.0f, 0.0f, 0.0f};
                float one[3] = {1.0f, 1.0f, 1.0f};
                float matrix[16] = {};

                // ImGuizmo uses degrees a a different handedness than Raylib
                float rotation[3] = {
                    transformPtr->rotation.x * RAD2DEG,
                    transformPtr->rotation.y * RAD2DEG,
                    transformPtr->rotation.z * RAD2DEG,
                };

                ImGuizmo::RecomposeMatrixFromComponents(
                    &transformPtr->position.x,
                    rotation,
                    one,
                    matrix);

                auto viewMatrix = MatrixTranspose(GetCameraMatrix(camera));
                auto projMatrix = MatrixTranspose(MatrixPerspective(
                    camera.fovy * DEG2RAD,
                    GetScreenWidth() / (double)GetScreenHeight(),
                    RL_CULL_DISTANCE_NEAR,
                    RL_CULL_DISTANCE_FAR));

                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetRect(0.0f, 0.0f, GetRenderWidth(), GetRenderHeight());
                ImGuizmo::Enable(true);
                ImGuizmo::Manipulate(
                    &viewMatrix.m0,
                    &projMatrix.m0,
                    ImGuizmo::OPERATION::TRANSLATE | ImGuizmo::OPERATION::ROTATE,
                    ImGuizmo::MODE::WORLD,
                    matrix);

                ImGuizmo::DecomposeMatrixToComponents(
                    matrix,
                    &transformPtr->position.x,
                    rotation,
                    one);

                transformPtr->rotation = {
                    rotation[0] * DEG2RAD,
                    rotation[1] * DEG2RAD,
                    rotation[2] * DEG2RAD,
                };
            });
    }
}