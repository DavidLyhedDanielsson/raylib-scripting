#include "lua_raylib_impl.hpp"

#include <entt/entt.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register_types.hpp>
#include <optional>
#include <world.hpp>

#include <entity/camera.hpp>
#include <entity/render.hpp>
#include <entity/transform.hpp>

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
#define QuickRegister(Func) LuaRegister::GlobalRegister(lua, #Func, Func);

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

        QuickRegister(IsKeyPressed);
        QuickRegister(IsKeyDown);
        QuickRegister(IsKeyReleased);
        QuickRegister(IsKeyUp);

        LuaRegister::GlobalRegisterMember(
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
        LuaRegister::GlobalRegisterMember(
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

        LuaRegister::GlobalRegisterMember(
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

        lua_createtable(lua, 0, 0);

        lua_pushinteger(lua, (lua_Integer)KEY_APOSTROPHE);
        lua_setfield(lua, -2, "APOSTROPHE");
        lua_pushinteger(lua, (lua_Integer)KEY_COMMA);
        lua_setfield(lua, -2, "COMMA");
        lua_pushinteger(lua, (lua_Integer)KEY_MINUS);
        lua_setfield(lua, -2, "MINUS");
        lua_pushinteger(lua, (lua_Integer)KEY_PERIOD);
        lua_setfield(lua, -2, "PERIOD");
        lua_pushinteger(lua, (lua_Integer)KEY_SLASH);
        lua_setfield(lua, -2, "SLASH");
        lua_pushinteger(lua, (lua_Integer)KEY_ZERO);
        lua_setfield(lua, -2, "ZERO");
        lua_pushinteger(lua, (lua_Integer)KEY_ONE);
        lua_setfield(lua, -2, "ONE");
        lua_pushinteger(lua, (lua_Integer)KEY_TWO);
        lua_setfield(lua, -2, "TWO");
        lua_pushinteger(lua, (lua_Integer)KEY_THREE);
        lua_setfield(lua, -2, "THREE");
        lua_pushinteger(lua, (lua_Integer)KEY_FOUR);
        lua_setfield(lua, -2, "FOUR");
        lua_pushinteger(lua, (lua_Integer)KEY_FIVE);
        lua_setfield(lua, -2, "FIVE");
        lua_pushinteger(lua, (lua_Integer)KEY_SIX);
        lua_setfield(lua, -2, "SIX");
        lua_pushinteger(lua, (lua_Integer)KEY_SEVEN);
        lua_setfield(lua, -2, "SEVEN");
        lua_pushinteger(lua, (lua_Integer)KEY_EIGHT);
        lua_setfield(lua, -2, "EIGHT");
        lua_pushinteger(lua, (lua_Integer)KEY_NINE);
        lua_setfield(lua, -2, "NINE");
        lua_pushinteger(lua, (lua_Integer)KEY_SEMICOLON);
        lua_setfield(lua, -2, "SEMICOLON");
        lua_pushinteger(lua, (lua_Integer)KEY_EQUAL);
        lua_setfield(lua, -2, "EQUAL");
        lua_pushinteger(lua, (lua_Integer)KEY_A);
        lua_setfield(lua, -2, "A");
        lua_pushinteger(lua, (lua_Integer)KEY_B);
        lua_setfield(lua, -2, "B");
        lua_pushinteger(lua, (lua_Integer)KEY_C);
        lua_setfield(lua, -2, "C");
        lua_pushinteger(lua, (lua_Integer)KEY_D);
        lua_setfield(lua, -2, "D");
        lua_pushinteger(lua, (lua_Integer)KEY_E);
        lua_setfield(lua, -2, "E");
        lua_pushinteger(lua, (lua_Integer)KEY_F);
        lua_setfield(lua, -2, "F");
        lua_pushinteger(lua, (lua_Integer)KEY_G);
        lua_setfield(lua, -2, "G");
        lua_pushinteger(lua, (lua_Integer)KEY_H);
        lua_setfield(lua, -2, "H");
        lua_pushinteger(lua, (lua_Integer)KEY_I);
        lua_setfield(lua, -2, "I");
        lua_pushinteger(lua, (lua_Integer)KEY_J);
        lua_setfield(lua, -2, "J");
        lua_pushinteger(lua, (lua_Integer)KEY_K);
        lua_setfield(lua, -2, "K");
        lua_pushinteger(lua, (lua_Integer)KEY_L);
        lua_setfield(lua, -2, "L");
        lua_pushinteger(lua, (lua_Integer)KEY_M);
        lua_setfield(lua, -2, "M");
        lua_pushinteger(lua, (lua_Integer)KEY_N);
        lua_setfield(lua, -2, "N");
        lua_pushinteger(lua, (lua_Integer)KEY_O);
        lua_setfield(lua, -2, "O");
        lua_pushinteger(lua, (lua_Integer)KEY_P);
        lua_setfield(lua, -2, "P");
        lua_pushinteger(lua, (lua_Integer)KEY_Q);
        lua_setfield(lua, -2, "Q");
        lua_pushinteger(lua, (lua_Integer)KEY_R);
        lua_setfield(lua, -2, "R");
        lua_pushinteger(lua, (lua_Integer)KEY_S);
        lua_setfield(lua, -2, "S");
        lua_pushinteger(lua, (lua_Integer)KEY_T);
        lua_setfield(lua, -2, "T");
        lua_pushinteger(lua, (lua_Integer)KEY_U);
        lua_setfield(lua, -2, "U");
        lua_pushinteger(lua, (lua_Integer)KEY_V);
        lua_setfield(lua, -2, "V");
        lua_pushinteger(lua, (lua_Integer)KEY_W);
        lua_setfield(lua, -2, "W");
        lua_pushinteger(lua, (lua_Integer)KEY_X);
        lua_setfield(lua, -2, "X");
        lua_pushinteger(lua, (lua_Integer)KEY_Y);
        lua_setfield(lua, -2, "Y");
        lua_pushinteger(lua, (lua_Integer)KEY_Z);
        lua_setfield(lua, -2, "Z");
        lua_pushinteger(lua, (lua_Integer)KEY_LEFT_BRACKET);
        lua_setfield(lua, -2, "LEFT_BRACKET");
        lua_pushinteger(lua, (lua_Integer)KEY_BACKSLASH);
        lua_setfield(lua, -2, "BACKSLASH");
        lua_pushinteger(lua, (lua_Integer)KEY_RIGHT_BRACKET);
        lua_setfield(lua, -2, "RIGHT_BRACKET");
        lua_pushinteger(lua, (lua_Integer)KEY_GRAVE);
        lua_setfield(lua, -2, "GRAVE");
        // Function keys
        lua_pushinteger(lua, (lua_Integer)KEY_SPACE);
        lua_setfield(lua, -2, "SPACE");
        lua_pushinteger(lua, (lua_Integer)KEY_ESCAPE);
        lua_setfield(lua, -2, "ESCAPE");
        lua_pushinteger(lua, (lua_Integer)KEY_ENTER);
        lua_setfield(lua, -2, "ENTER");
        lua_pushinteger(lua, (lua_Integer)KEY_TAB);
        lua_setfield(lua, -2, "TAB");
        lua_pushinteger(lua, (lua_Integer)KEY_BACKSPACE);
        lua_setfield(lua, -2, "BACKSPACE");
        lua_pushinteger(lua, (lua_Integer)KEY_INSERT);
        lua_setfield(lua, -2, "INSERT");
        lua_pushinteger(lua, (lua_Integer)KEY_DELETE);
        lua_setfield(lua, -2, "DELETE");
        lua_pushinteger(lua, (lua_Integer)KEY_RIGHT);
        lua_setfield(lua, -2, "RIGHT");
        lua_pushinteger(lua, (lua_Integer)KEY_LEFT);
        lua_setfield(lua, -2, "LEFT");
        lua_pushinteger(lua, (lua_Integer)KEY_DOWN);
        lua_setfield(lua, -2, "DOWN");
        lua_pushinteger(lua, (lua_Integer)KEY_UP);
        lua_setfield(lua, -2, "UP");
        lua_pushinteger(lua, (lua_Integer)KEY_PAGE_UP);
        lua_setfield(lua, -2, "PAGE_UP");
        lua_pushinteger(lua, (lua_Integer)KEY_PAGE_DOWN);
        lua_setfield(lua, -2, "PAGE_DOWN");
        lua_pushinteger(lua, (lua_Integer)KEY_HOME);
        lua_setfield(lua, -2, "HOME");
        lua_pushinteger(lua, (lua_Integer)KEY_END);
        lua_setfield(lua, -2, "END");
        lua_pushinteger(lua, (lua_Integer)KEY_CAPS_LOCK);
        lua_setfield(lua, -2, "CAPS_LOCK");
        lua_pushinteger(lua, (lua_Integer)KEY_SCROLL_LOCK);
        lua_setfield(lua, -2, "SCROLL_LOCK");
        lua_pushinteger(lua, (lua_Integer)KEY_NUM_LOCK);
        lua_setfield(lua, -2, "NUM_LOCK");
        lua_pushinteger(lua, (lua_Integer)KEY_PRINT_SCREEN);
        lua_setfield(lua, -2, "PRINT_SCREEN");
        lua_pushinteger(lua, (lua_Integer)KEY_PAUSE);
        lua_setfield(lua, -2, "PAUSE");
        lua_pushinteger(lua, (lua_Integer)KEY_F1);
        lua_setfield(lua, -2, "F1");
        lua_pushinteger(lua, (lua_Integer)KEY_F2);
        lua_setfield(lua, -2, "F2");
        lua_pushinteger(lua, (lua_Integer)KEY_F3);
        lua_setfield(lua, -2, "F3");
        lua_pushinteger(lua, (lua_Integer)KEY_F4);
        lua_setfield(lua, -2, "F4");
        lua_pushinteger(lua, (lua_Integer)KEY_F5);
        lua_setfield(lua, -2, "F5");
        lua_pushinteger(lua, (lua_Integer)KEY_F6);
        lua_setfield(lua, -2, "F6");
        lua_pushinteger(lua, (lua_Integer)KEY_F7);
        lua_setfield(lua, -2, "F7");
        lua_pushinteger(lua, (lua_Integer)KEY_F8);
        lua_setfield(lua, -2, "F8");
        lua_pushinteger(lua, (lua_Integer)KEY_F9);
        lua_setfield(lua, -2, "F9");
        lua_pushinteger(lua, (lua_Integer)KEY_F10);
        lua_setfield(lua, -2, "F10");
        lua_pushinteger(lua, (lua_Integer)KEY_F11);
        lua_setfield(lua, -2, "F11");
        lua_pushinteger(lua, (lua_Integer)KEY_F12);
        lua_setfield(lua, -2, "F12");
        lua_pushinteger(lua, (lua_Integer)KEY_LEFT_SHIFT);
        lua_setfield(lua, -2, "LEFT_SHIFT");
        lua_pushinteger(lua, (lua_Integer)KEY_LEFT_CONTROL);
        lua_setfield(lua, -2, "LEFT_CONTROL");
        lua_pushinteger(lua, (lua_Integer)KEY_LEFT_ALT);
        lua_setfield(lua, -2, "LEFT_ALT");
        lua_pushinteger(lua, (lua_Integer)KEY_LEFT_SUPER);
        lua_setfield(lua, -2, "LEFT_SUPER");
        lua_pushinteger(lua, (lua_Integer)KEY_RIGHT_SHIFT);
        lua_setfield(lua, -2, "RIGHT_SHIFT");
        lua_pushinteger(lua, (lua_Integer)KEY_RIGHT_CONTROL);
        lua_setfield(lua, -2, "RIGHT_CONTROL");
        lua_pushinteger(lua, (lua_Integer)KEY_RIGHT_ALT);
        lua_setfield(lua, -2, "RIGHT_ALT");
        lua_pushinteger(lua, (lua_Integer)KEY_RIGHT_SUPER);
        lua_setfield(lua, -2, "RIGHT_SUPER");
        lua_pushinteger(lua, (lua_Integer)KEY_KB_MENU);
        lua_setfield(lua, -2, "KB_MENU");
        // Keypad keys
        lua_pushinteger(lua, (lua_Integer)KEY_KP_0);
        lua_setfield(lua, -2, "KP_0");
        lua_pushinteger(lua, (lua_Integer)KEY_KP_1);
        lua_setfield(lua, -2, "KP_1");
        lua_pushinteger(lua, (lua_Integer)KEY_KP_2);
        lua_setfield(lua, -2, "KP_2");
        lua_pushinteger(lua, (lua_Integer)KEY_KP_3);
        lua_setfield(lua, -2, "KP_3");
        lua_pushinteger(lua, (lua_Integer)KEY_KP_4);
        lua_setfield(lua, -2, "KP_4");
        lua_pushinteger(lua, (lua_Integer)KEY_KP_5);
        lua_setfield(lua, -2, "KP_5");
        lua_pushinteger(lua, (lua_Integer)KEY_KP_6);
        lua_setfield(lua, -2, "KP_6");
        lua_pushinteger(lua, (lua_Integer)KEY_KP_7);
        lua_setfield(lua, -2, "KP_7");
        lua_pushinteger(lua, (lua_Integer)KEY_KP_8);
        lua_setfield(lua, -2, "KP_8");
        lua_pushinteger(lua, (lua_Integer)KEY_KP_9);
        lua_setfield(lua, -2, "KP_9");
        lua_pushinteger(lua, (lua_Integer)KEY_KP_DECIMAL);
        lua_setfield(lua, -2, "KP_DECIMAL");
        lua_pushinteger(lua, (lua_Integer)KEY_KP_DIVIDE);
        lua_setfield(lua, -2, "KP_DIVIDE");
        lua_pushinteger(lua, (lua_Integer)KEY_KP_MULTIPLY);
        lua_setfield(lua, -2, "KP_MULTIPLY");
        lua_pushinteger(lua, (lua_Integer)KEY_KP_SUBTRACT);
        lua_setfield(lua, -2, "KP_SUBTRACT");
        lua_pushinteger(lua, (lua_Integer)KEY_KP_ADD);
        lua_setfield(lua, -2, "KP_ADD");
        lua_pushinteger(lua, (lua_Integer)KEY_KP_ENTER);
        lua_setfield(lua, -2, "KP_ENTER");
        lua_pushinteger(lua, (lua_Integer)KEY_KP_EQUAL);
        lua_setfield(lua, -2, "KP_EQUAL");

        lua_setglobal(lua, "Key");
    }
}