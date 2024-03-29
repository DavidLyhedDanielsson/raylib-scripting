#include "lua_raylib_impl.hpp"
#include "lua_impl/lua_register.hpp"
#include "raylib.h"
#include <entt/entt.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register_types.hpp>
#include <optional>
#include <world.hpp>

#include <component/camera.hpp>
#include <component/render.hpp>
#include <component/transform.hpp>

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
#define QuickRegister(Func) LuaRegister::PushRegister(lua, #Func, Func);
        lua_createtable(lua, 0, 0);

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

        QuickRegister(BeginScissorMode);
        QuickRegister(EndScissorMode);

        LuaRegister::PushRegisterMember(
            lua,
            "GetWorldToScreen",
            registry,
            +[](entt::registry* registry, lua_State* lua, Vector3 position) {
                entt::entity entity =
                    *registry->view<Component::Camera, Component::Transform>().begin();

                auto camera = registry->get<Component::Camera>(entity);
                auto transform = registry->get<Component::Transform>(entity);

                return GetWorldToScreen(position, camera.CreateRaylibCamera(transform.position));
            });

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
        // QuickRegister(DrawText);
        LuaRegister::PushRegister(
            lua,
            "DrawText",
            +[](lua_State* lua, const char* text, float x, float y, float fontSize) {
                // If x and y are int this doesn't work ????
                DrawText(text, x, y, fontSize, WHITE);
            });

        QuickRegister(IsMouseButtonUp);
        QuickRegister(IsMouseButtonDown);
        QuickRegister(IsMouseButtonPressed);
        QuickRegister(IsMouseButtonReleased);

        QuickRegister(IsKeyPressed);
        QuickRegister(IsKeyDown);
        QuickRegister(IsKeyReleased);
        QuickRegister(IsKeyUp);

        LuaRegister::PushRegister(
            lua,
            "MeasureTextSize",
            +[](const char* text, int fontSize) {
                return MeasureTextEx(GetFontDefault(), text, fontSize, fontSize / 10.0f);
            });

        LuaRegister::PushRegisterMember(
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
        LuaRegister::PushRegisterMember(
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

        LuaRegister::PushRegisterMember(
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

        LuaRegister::PushRegister(
            lua,
            "GetRayCollisionQuad",
            +[](lua_State* lua, Ray ray, Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4)
                -> LuaRegister::Placeholder {
                RayCollision collision = GetRayCollisionQuad(ray, p1, p2, p3, p4);
                if(collision.hit)
                    LuaRegister::LuaSetFunc<Vector3>(lua, collision.point);
                else
                    lua_pushnil(lua);

                return {};
            });

        LuaRegister::PushRegisterMember(
            lua,
            "DrawEntityBoundingBox",
            registry,
            +[](entt::registry* registry, lua_Integer entityL) {
                entt::entity entity = (entt::entity)entityL;

                Component::Render* render = registry->try_get<Component::Render>(entity);
                if(!render)
                    return;

                Component::Transform* transform = registry->try_get<Component::Transform>(entity);
                if(!transform)
                    return;

                auto bbox = BoundingBoxTransform(
                    render->boundingBox,
                    transform->position,
                    MatrixRotateZYX(transform->rotation));
                DrawBoundingBox(bbox, RED);
            });

        lua_pushstring(lua, "Key");
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
        lua_settable(lua, -3);

        lua_setglobal(lua, "Raylib");

#define QuickRegisterGui(Func) LuaRegister::PushRegister(lua, #Func, Gui##Func);
        lua_createtable(lua, 0, 0);

        QuickRegisterGui(Enable);
        QuickRegisterGui(Disable);
        QuickRegisterGui(Lock);
        QuickRegisterGui(Unlock);
        QuickRegisterGui(IsLocked);
        QuickRegisterGui(Fade);
        QuickRegisterGui(SetState);
        QuickRegisterGui(GetState);
        QuickRegisterGui(SetStyle);
        QuickRegisterGui(GetStyle);
        QuickRegisterGui(LoadStyle);
        QuickRegisterGui(LoadStyleDefault);
        QuickRegisterGui(EnableTooltip);
        QuickRegisterGui(DisableTooltip);
        QuickRegisterGui(SetTooltip);

        QuickRegisterGui(Panel);
        LuaRegister::PushRegister(
            lua,
            "ScrollPanel",
            +[](lua_State* lua,
                Rectangle bounds,
                const char* text,
                Rectangle content,
                Vector2* scroll,
                Rectangle* view) {
                int retVal = GuiScrollPanel(bounds, text, content, scroll, view);
                return retVal;
            });

        // QuickRegisterGui(ScrollPanel);

        QuickRegisterGui(Label);
        QuickRegisterGui(Button);
        QuickRegisterGui(LabelButton);
        QuickRegisterGui(Toggle);
        QuickRegisterGui(ToggleGroup);
        QuickRegisterGui(CheckBox);
        QuickRegisterGui(ComboBox);

        QuickRegisterGui(DropdownBox);
        QuickRegisterGui(Spinner);
        QuickRegisterGui(ValueBox);
        // char* not supported, only const char*
        // QuickRegisterGui(TextBox);

        QuickRegisterGui(Slider);
        QuickRegisterGui(SliderBar);
        QuickRegisterGui(ProgressBar);
        QuickRegisterGui(StatusBar);
        QuickRegisterGui(DummyRec);
        QuickRegisterGui(Grid);

        QuickRegisterGui(ListView);
        QuickRegisterGui(ListViewEx);
        QuickRegisterGui(MessageBox);
        // QuickRegisterGui(TextInputBox);

#define RegisterEnum(name, func) \
    lua_pushstring(lua, name);   \
    lua_createtable(lua, 0, 0);  \
    [&]() {                      \
        func                     \
    }();                         \
    lua_settable(lua, -3);

#define ENUM_VARIANT(name, variant)             \
    lua_pushinteger(lua, (lua_Integer)variant); \
    lua_setfield(lua, -2, name);
#define ENUM_VARIANT_(variant)                  \
    lua_pushinteger(lua, (lua_Integer)variant); \
    lua_setfield(lua, -2, #variant);

        RegisterEnum("DefaultProperty", {
            ENUM_VARIANT_(TEXT_SIZE);
            ENUM_VARIANT_(TEXT_SPACING);
            ENUM_VARIANT_(LINE_COLOR);
            ENUM_VARIANT_(BACKGROUND_COLOR);
        });
        RegisterEnum("State", {
            ENUM_VARIANT("NORMAL", STATE_NORMAL);
            ENUM_VARIANT("FOCUSED", STATE_FOCUSED);
            ENUM_VARIANT("PRESSED", STATE_PRESSED);
            ENUM_VARIANT("DISABLED", STATE_DISABLED);
        });
        RegisterEnum("TextAlignment", {
            ENUM_VARIANT("LEFT", TEXT_ALIGN_LEFT);
            ENUM_VARIANT("CENTER", TEXT_ALIGN_CENTER);
            ENUM_VARIANT("RIGHT", TEXT_ALIGN_RIGHT);
        });
        RegisterEnum("Control", {
            ENUM_VARIANT_(DEFAULT);
            ENUM_VARIANT_(LABEL);
            ENUM_VARIANT_(BUTTON);
            ENUM_VARIANT_(TOGGLE);
            ENUM_VARIANT_(SLIDER);
            ENUM_VARIANT_(PROGRESSBAR);
            ENUM_VARIANT_(CHECKBOX);
            ENUM_VARIANT_(COMBOBOX);
            ENUM_VARIANT_(DROPDOWNBOX);
            ENUM_VARIANT_(TEXTBOX);
            ENUM_VARIANT_(VALUEBOX);
            ENUM_VARIANT_(SPINNER);
            ENUM_VARIANT_(LISTVIEW);
            ENUM_VARIANT_(COLORPICKER);
            ENUM_VARIANT_(SCROLLBAR);
            ENUM_VARIANT_(STATUSBAR);
        });
        RegisterEnum("ControlProperty", {
            ENUM_VARIANT_(BORDER_COLOR_NORMAL);
            ENUM_VARIANT_(BASE_COLOR_NORMAL);
            ENUM_VARIANT_(TEXT_COLOR_NORMAL);
            ENUM_VARIANT_(BORDER_COLOR_FOCUSED);
            ENUM_VARIANT_(BASE_COLOR_FOCUSED);
            ENUM_VARIANT_(TEXT_COLOR_FOCUSED);
            ENUM_VARIANT_(BORDER_COLOR_PRESSED);
            ENUM_VARIANT_(BASE_COLOR_PRESSED);
            ENUM_VARIANT_(TEXT_COLOR_PRESSED);
            ENUM_VARIANT_(BORDER_COLOR_DISABLED);
            ENUM_VARIANT_(BASE_COLOR_DISABLED);
            ENUM_VARIANT_(TEXT_COLOR_DISABLED);
            ENUM_VARIANT_(BORDER_WIDTH);
            ENUM_VARIANT_(TEXT_PADDING);
            ENUM_VARIANT_(TEXT_ALIGNMENT);
            ENUM_VARIANT_(RESERVED);
        });
        RegisterEnum("ToggleProperty", { ENUM_VARIANT_(GROUP_PADDING); });
        RegisterEnum("SliderProperty", {
            ENUM_VARIANT("WIDTH", SLIDER_WIDTH);
            ENUM_VARIANT("PADDING", SLIDER_PADDING);
        });
        RegisterEnum("ProgressBarProperty", { ENUM_VARIANT("PADDING", PROGRESS_PADDING); });
        RegisterEnum("ScrollBarProperty", {
            ENUM_VARIANT_(ARROWS_SIZE);
            ENUM_VARIANT_(ARROWS_VISIBLE);
            ENUM_VARIANT("SLIDER_PADDING", SCROLL_SLIDER_PADDING);
            ENUM_VARIANT("SLIDER_SIZE", SCROLL_SLIDER_SIZE);
            ENUM_VARIANT("PADDING", SCROLL_PADDING);
            ENUM_VARIANT("SPEED", SCROLL_SPEED);
        });
        RegisterEnum("CheckBoxProperty", { ENUM_VARIANT("PADDING", CHECK_PADDING); });
        RegisterEnum("ComboBoxProperty", {
            ENUM_VARIANT("BUTTON_WIDTH", COMBO_BUTTON_WIDTH);
            ENUM_VARIANT("BUTTON_SPACING", COMBO_BUTTON_SPACING);
        });
        RegisterEnum("DropdownBoxProperty", {
            ENUM_VARIANT_(ARROW_PADDING);
            ENUM_VARIANT("ITEMS_SPACING", DROPDOWN_ITEMS_SPACING);
        });
        RegisterEnum("TextBoxProperty", {
            ENUM_VARIANT("INNER_PADDING", TEXT_INNER_PADDING);
            ENUM_VARIANT("LINES_SPACING", TEXT_LINES_SPACING);
            ENUM_VARIANT("ALIGNMENT_VERTICAL", TEXT_ALIGNMENT_VERTICAL);
            ENUM_VARIANT("MULTILINE", TEXT_MULTILINE);
            ENUM_VARIANT("WRAP_MODE", TEXT_WRAP_MODE);
        });
        RegisterEnum("SpinnerProperty", {
            ENUM_VARIANT("BUTTON_WIDTH", SPIN_BUTTON_WIDTH);
            ENUM_VARIANT("BUTTON_SPACING", SPIN_BUTTON_SPACING);
        });
        RegisterEnum("ListViewProperty", {
            ENUM_VARIANT("ITEMS_HEIGHT", LIST_ITEMS_HEIGHT);
            ENUM_VARIANT("ITEMS_SPACING", LIST_ITEMS_SPACING);
            ENUM_VARIANT_(SCROLLBAR_WIDTH);
            ENUM_VARIANT_(SCROLLBAR_SIDE);
        });

        lua_setglobal(lua, "Raygui");
    }
}