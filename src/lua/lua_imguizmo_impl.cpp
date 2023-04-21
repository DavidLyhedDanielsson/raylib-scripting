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
                auto transformPtr = registry->try_get<Component::Transform>((entt::entity)entity);
                Camera camera;
                registry->view<Component::Camera, Component::Transform>().each(
                    [&](entt::entity, Component::Camera c, Component::Transform t) {
                        camera = c.CreateRaylibCamera(t.position);
                    });

                float zero[3] = {0.0f, 0.0f, 0.0f};
                float one[3] = {1.0f, 1.0f, 1.0f};
                float matrix[16] = {};

                ImGuizmo::RecomposeMatrixFromComponents(
                    &transformPtr->position.x,
                    zero,
                    one,
                    matrix);

                auto viewMatrix = MatrixTranspose(GetCameraMatrix(camera));
                auto projMatrix = MatrixTranspose(MatrixPerspective(
                    camera.fovy * DEG2RAD,
                    GetScreenWidth() / (double)GetScreenHeight(),
                    RL_CULL_DISTANCE_NEAR,
                    RL_CULL_DISTANCE_FAR));

                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetRect(0.0f, 0.0f, GetScreenWidth(), GetScreenHeight());
                ImGuizmo::Enable(true);
                ImGuizmo::Manipulate(
                    &viewMatrix.m0,
                    &projMatrix.m0,
                    ImGuizmo::OPERATION::TRANSLATE,
                    ImGuizmo::MODE::WORLD,
                    matrix);

                ImGuizmo::DecomposeMatrixToComponents(matrix, &transformPtr->position.x, zero, one);
            });
    }
}