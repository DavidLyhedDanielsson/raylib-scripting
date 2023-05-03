#include "lua_imguizmo_impl.hpp"

#include <entity/camera.hpp>
#include <entity/transform.hpp>
#include <entt/entt.hpp>
#include <external/imgui.hpp>
#include <external/imguizmo.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register.hpp>

#define LuaImGuizmoQuickRegister(X) LuaRegister::PushRegister(lua, #X, ImGuizmo::X)

namespace LuaImGuizmo
{
    void Register(lua_State* lua, entt::registry* registry)
    {
        using namespace LuaRegister;

        lua_createtable(lua, 0, 0);

        LuaImGuizmoQuickRegister(IsUsing);

        PushRegisterMember(
            lua,
            "Gizmo",
            registry,
            +[](entt::registry* registry, lua_State* state, lua_Integer entity, bool startUsing) {
                if(!registry->valid((entt::entity)entity))
                    return;

                auto transformPtr = registry->try_get<Component::Transform>((entt::entity)entity);
                Camera camera;
                registry->view<Component::Camera, Component::Transform>().each(
                    [&](entt::entity, Component::Camera c, Component::Transform t) {
                        camera = c.CreateRaylibCamera(t.position);
                    });

                float one[3] = {1.0f, 1.0f, 1.0f};

                static float matrix[16]{0.0f};

                if(startUsing)
                {
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
                }

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

                if(ImGuizmo::IsUsing())
                {
                    float rotation[3]{0.0f};
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
                }
            });

        lua_setglobal(lua, "ImGuizmo");
    }
}