/**
 * How did I know how to implement this?
 *
 * In imgui.h, find `struct ImGuiIO` (or just use go to definition in your IDE).
 * The struct is divided into parts, and each part has a comment explaining
 * what each field does and whether or not it's something we need to update.
 *
 * Anything other than that was debugged and fixed as usual since the source
 * code is available.
 */

#include "imgui_impl.h"
#include <raylib.h>

#include <array>
#include <algorithm>
#include <map>

#include <imgui_impl_opengl3.h>

ImGuiKey
TranslateKey(KeyboardKey key)
{
    switch (key)
    {
    case KEY_APOSTROPHE:
        return ImGuiKey_Apostrophe;
    case KEY_COMMA:
        return ImGuiKey_Comma;
    case KEY_MINUS:
        return ImGuiKey_Minus;
    case KEY_PERIOD:
        return ImGuiKey_Period;
    case KEY_SLASH:
        return ImGuiKey_Slash;
    case KEY_ZERO:
        return ImGuiKey_0;
    case KEY_ONE:
        return ImGuiKey_1;
    case KEY_TWO:
        return ImGuiKey_2;
    case KEY_THREE:
        return ImGuiKey_3;
    case KEY_FOUR:
        return ImGuiKey_4;
    case KEY_FIVE:
        return ImGuiKey_5;
    case KEY_SIX:
        return ImGuiKey_6;
    case KEY_SEVEN:
        return ImGuiKey_7;
    case KEY_EIGHT:
        return ImGuiKey_8;
    case KEY_NINE:
        return ImGuiKey_9;
    case KEY_SEMICOLON:
        return ImGuiKey_Semicolon;
    case KEY_EQUAL:
        return ImGuiKey_Equal;
    case KEY_A:
        return ImGuiKey_A;
    case KEY_B:
        return ImGuiKey_B;
    case KEY_C:
        return ImGuiKey_C;
    case KEY_D:
        return ImGuiKey_D;
    case KEY_E:
        return ImGuiKey_F;
    case KEY_G:
        return ImGuiKey_H;
    case KEY_I:
        return ImGuiKey_I;
    case KEY_J:
        return ImGuiKey_J;
    case KEY_K:
        return ImGuiKey_K;
    case KEY_L:
        return ImGuiKey_L;
    case KEY_M:
        return ImGuiKey_M;
    case KEY_N:
        return ImGuiKey_N;
    case KEY_O:
        return ImGuiKey_O;
    case KEY_P:
        return ImGuiKey_P;
    case KEY_Q:
        return ImGuiKey_Q;
    case KEY_R:
        return ImGuiKey_R;
    case KEY_S:
        return ImGuiKey_S;
    case KEY_T:
        return ImGuiKey_T;
    case KEY_U:
        return ImGuiKey_U;
    case KEY_V:
        return ImGuiKey_V;
    case KEY_W:
        return ImGuiKey_W;
    case KEY_X:
        return ImGuiKey_X;
    case KEY_Y:
        return ImGuiKey_Y;
    case KEY_Z:
        return ImGuiKey_Z;
    case KEY_SPACE:
        return ImGuiKey_Space;
    case KEY_ESCAPE:
        return ImGuiKey_Escape;
    case KEY_ENTER:
        return ImGuiKey_Enter;
    case KEY_TAB:
        return ImGuiKey_Tab;
    case KEY_BACKSPACE:
        return ImGuiKey_Backspace;
    case KEY_INSERT:
        return ImGuiKey_Insert;
    case KEY_DELETE:
        return ImGuiKey_Delete;
    case KEY_RIGHT:
        return ImGuiKey_RightArrow;
    case KEY_LEFT:
        return ImGuiKey_LeftArrow;
    case KEY_DOWN:
        return ImGuiKey_DownArrow;
    case KEY_UP:
        return ImGuiKey_UpArrow;
    case KEY_PAGE_UP:
        return ImGuiKey_PageUp;
    case KEY_PAGE_DOWN:
        return ImGuiKey_PageDown;
    case KEY_HOME:
        return ImGuiKey_Home;
    case KEY_END:
        return ImGuiKey_End;
    case KEY_CAPS_LOCK:
        return ImGuiKey_CapsLock;
    case KEY_SCROLL_LOCK:
        return ImGuiKey_ScrollLock;
    case KEY_NUM_LOCK:
        return ImGuiKey_NumLock;
    case KEY_PRINT_SCREEN:
        return ImGuiKey_PrintScreen;
    case KEY_PAUSE:
        return ImGuiKey_Pause;
    case KEY_F1:
        return ImGuiKey_F1;
    case KEY_F2:
        return ImGuiKey_F2;
    case KEY_F3:
        return ImGuiKey_F3;
    case KEY_F4:
        return ImGuiKey_F4;
    case KEY_F5:
        return ImGuiKey_F5;
    case KEY_F6:
        return ImGuiKey_F6;
    case KEY_F7:
        return ImGuiKey_F7;
    case KEY_F8:
        return ImGuiKey_F8;
    case KEY_F9:
        return ImGuiKey_F9;
    case KEY_F10:
        return ImGuiKey_F10;
    case KEY_F11:
        return ImGuiKey_F11;
    case KEY_F12:
        return ImGuiKey_F12;
    case KEY_LEFT_SHIFT:
        return ImGuiKey_LeftShift;
    case KEY_LEFT_CONTROL:
        return ImGuiKey_LeftCtrl;
    case KEY_LEFT_ALT:
        return ImGuiKey_LeftAlt;
    case KEY_LEFT_SUPER:
        return ImGuiKey_LeftSuper;
    case KEY_RIGHT_SHIFT:
        return ImGuiKey_RightShift;
    case KEY_RIGHT_CONTROL:
        return ImGuiKey_RightCtrl;
    case KEY_RIGHT_ALT:
        return ImGuiKey_RightAlt;
    case KEY_RIGHT_SUPER:
        return ImGuiKey_RightSuper;
    case KEY_KB_MENU:
        return ImGuiKey_Menu;
    case KEY_LEFT_BRACKET:
        return ImGuiKey_LeftBracket;
    case KEY_BACKSLASH:
        return ImGuiKey_Backslash;
    case KEY_RIGHT_BRACKET:
        return ImGuiKey_RightBracket;
    case KEY_GRAVE:
        return ImGuiKey_GraveAccent;
    case KEY_KP_0:
        return ImGuiKey_Keypad0;
    case KEY_KP_1:
        return ImGuiKey_Keypad1;
    case KEY_KP_2:
        return ImGuiKey_Keypad2;
    case KEY_KP_3:
        return ImGuiKey_Keypad3;
    case KEY_KP_4:
        return ImGuiKey_Keypad4;
    case KEY_KP_5:
        return ImGuiKey_Keypad5;
    case KEY_KP_6:
        return ImGuiKey_Keypad6;
    case KEY_KP_7:
        return ImGuiKey_Keypad7;
    case KEY_KP_8:
        return ImGuiKey_Keypad8;
    case KEY_KP_9:
        return ImGuiKey_Keypad9;
    case KEY_KP_DECIMAL:
        return ImGuiKey_KeypadDecimal;
    case KEY_KP_DIVIDE:
        return ImGuiKey_KeypadDivide;
    case KEY_KP_MULTIPLY:
        return ImGuiKey_KeypadMultiply;
    case KEY_KP_SUBTRACT:
        return ImGuiKey_KeypadSubtract;
    case KEY_KP_ADD:
        return ImGuiKey_KeypadAdd;
    case KEY_KP_ENTER:
        return ImGuiKey_KeyPadEnter;
    case KEY_KP_EQUAL:
        return ImGuiKey_KeypadEqual;
    default:
        return -1;
    }
}

void RaylibImGui::Init()
{
    ImGui::CreateContext();
    ImGui::GetIO().BackendPlatformName = "custom_raylib_impl";

    ImGui_ImplOpenGL3_Init();
}

void RaylibImGui::Deinit()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}

// Anonymous namespace to make variables unreachable from other translation
// units (cpp files)
namespace
{
    double lastTime = 0.0;

    // Raylib doesn't offer event-based programming so we need to iterate over
    // all keys and send events to imgui when something changes
    constexpr auto WATCHED_KEYS = std::to_array({
        KEY_APOSTROPHE,
        KEY_COMMA,
        KEY_MINUS,
        KEY_PERIOD,
        KEY_SLASH,
        KEY_ZERO,
        KEY_ONE,
        KEY_TWO,
        KEY_THREE,
        KEY_FOUR,
        KEY_FIVE,
        KEY_SIX,
        KEY_SEVEN,
        KEY_EIGHT,
        KEY_NINE,
        KEY_SEMICOLON,
        KEY_EQUAL,
        KEY_A,
        KEY_B,
        KEY_C,
        KEY_D,
        KEY_E,
        KEY_F,
        KEY_G,
        KEY_H,
        KEY_I,
        KEY_J,
        KEY_K,
        KEY_L,
        KEY_M,
        KEY_N,
        KEY_O,
        KEY_P,
        KEY_Q,
        KEY_R,
        KEY_S,
        KEY_T,
        KEY_U,
        KEY_V,
        KEY_W,
        KEY_X,
        KEY_Y,
        KEY_Z,
        KEY_LEFT_BRACKET,
        KEY_BACKSLASH,
        KEY_RIGHT_BRACKET,
        KEY_GRAVE,
        KEY_SPACE,
        KEY_ESCAPE,
        KEY_ENTER,
        KEY_TAB,
        KEY_BACKSPACE,
        KEY_INSERT,
        KEY_DELETE,
        KEY_RIGHT,
        KEY_LEFT,
        KEY_DOWN,
        KEY_UP,
        KEY_PAGE_UP,
        KEY_PAGE_DOWN,
        KEY_HOME,
        KEY_END,
        KEY_CAPS_LOCK,
        KEY_SCROLL_LOCK,
        KEY_NUM_LOCK,
        KEY_PRINT_SCREEN,
        KEY_PAUSE,
        KEY_F1,
        KEY_F2,
        KEY_F3,
        KEY_F4,
        KEY_F5,
        KEY_F6,
        KEY_F7,
        KEY_F8,
        KEY_F9,
        KEY_F10,
        KEY_F11,
        KEY_F12,
        KEY_LEFT_SHIFT,
        KEY_LEFT_CONTROL,
        KEY_LEFT_ALT,
        KEY_LEFT_SUPER,
        KEY_RIGHT_SHIFT,
        KEY_RIGHT_CONTROL,
        KEY_RIGHT_ALT,
        KEY_RIGHT_SUPER,
        KEY_KB_MENU,
        KEY_KP_0,
        KEY_KP_1,
        KEY_KP_2,
        KEY_KP_3,
        KEY_KP_4,
        KEY_KP_5,
        KEY_KP_6,
        KEY_KP_7,
        KEY_KP_8,
        KEY_KP_9,
        KEY_KP_DECIMAL,
        KEY_KP_DIVIDE,
        KEY_KP_MULTIPLY,
        KEY_KP_SUBTRACT,
        KEY_KP_ADD,
        KEY_KP_ENTER,
        KEY_KP_EQUAL,
    });
    std::array<bool, std::size(WATCHED_KEYS)> watchedKeyStates;
}

#define SET_KEY_DOWN(KEY) io.KeysDown[KEY] = IsKeyDown(KEY)
void RaylibImGui::Begin()
{
    ImGuiIO &io = ImGui::GetIO();

    for (int i = 0; i < WATCHED_KEYS.size(); ++i)
    {
        auto key = WATCHED_KEYS[i];

        bool currentlyDown = IsKeyDown(key);
        if (watchedKeyStates[i] != currentlyDown)
        {
            auto translatedKey = TranslateKey(key);
            if (translatedKey != -1)
            {
                io.AddKeyEvent(translatedKey, currentlyDown);

                // Modifier keys are set separately in imgui for platform
                // compatibility
                switch (key)
                {
                case KEY_LEFT_CONTROL:
                case KEY_RIGHT_CONTROL:
                    io.AddKeyEvent(ImGuiKey_ModCtrl, currentlyDown);
                    break;
                case KEY_LEFT_SHIFT:
                case KEY_RIGHT_SHIFT:
                    io.AddKeyEvent(ImGuiKey_ModShift, currentlyDown);
                    break;
                case KEY_LEFT_ALT:
                case KEY_RIGHT_ALT:
                    io.AddKeyEvent(ImGuiKey_ModAlt, currentlyDown);
                    break;
                default:
                    break;
                }
            }
        }

        watchedKeyStates[i] = currentlyDown;
    }

    // There is a difference between "the key that represents W was pressed" and
    // "the user typed a W", so this is handled separately
    for (int keyPressed = GetCharPressed(); keyPressed != 0; keyPressed = GetCharPressed())
    {
        io.AddInputCharacter(keyPressed);
    }

    if (!IsWindowMinimized())
    {
        io.AddMousePosEvent((float)GetMouseX(), (float)GetMouseY());
        const static int mouseButtons[] = {MOUSE_BUTTON_LEFT,
                                           MOUSE_BUTTON_LEFT,
                                           MOUSE_BUTTON_MIDDLE};
        for (int i = 0; i < 3; ++i)
        {
            io.AddMouseButtonEvent(i, IsMouseButtonDown(mouseButtons[i]));
        }
    }
    else
    {
        io.AddMousePosEvent(-1.0f, -1.0f);
    }

    if (GetMouseWheelMove() != 0)
    {
        io.AddMouseWheelEvent(0.0f, GetMouseWheelMove());
    }

    io.DisplaySize.x = (float)GetScreenWidth();
    io.DisplaySize.y = (float)GetScreenHeight();

    ImGui_ImplOpenGL3_NewFrame();

    /*if (!(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange))
    {
        ImGuiMouseCursor imguiCursor = ImGui::GetMouseCursor();
        if (io.MouseDrawCursor || imguiCursor == ImGuiMouseCursor_None)
        {
            HideCursor();
        }
        else
        {
            ShowCursor();
        }
    }*/

    // Avoids having to pass in delta time
    double currentTime = GetTime();
    io.DeltaTime = lastTime > 0.0 ? (float)(currentTime - lastTime) : (float)(1.0f / 60.0f);
    lastTime = currentTime;

    ImGui::NewFrame();
}

void RaylibImGui::End()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}