#include "imgui_impl.h"
#include <raylib.h>

#include <imgui_impl_opengl3.h>

#define FOR_ALL_KEYS(X)   \
    X(KEY_APOSTROPHE);    \
    X(KEY_COMMA);         \
    X(KEY_MINUS);         \
    X(KEY_PERIOD);        \
    X(KEY_SLASH);         \
    X(KEY_ZERO);          \
    X(KEY_ONE);           \
    X(KEY_TWO);           \
    X(KEY_THREE);         \
    X(KEY_FOUR);          \
    X(KEY_FIVE);          \
    X(KEY_SIX);           \
    X(KEY_SEVEN);         \
    X(KEY_EIGHT);         \
    X(KEY_NINE);          \
    X(KEY_SEMICOLON);     \
    X(KEY_EQUAL);         \
    X(KEY_A);             \
    X(KEY_B);             \
    X(KEY_C);             \
    X(KEY_D);             \
    X(KEY_E);             \
    X(KEY_F);             \
    X(KEY_G);             \
    X(KEY_H);             \
    X(KEY_I);             \
    X(KEY_J);             \
    X(KEY_K);             \
    X(KEY_L);             \
    X(KEY_M);             \
    X(KEY_N);             \
    X(KEY_O);             \
    X(KEY_P);             \
    X(KEY_Q);             \
    X(KEY_R);             \
    X(KEY_S);             \
    X(KEY_T);             \
    X(KEY_U);             \
    X(KEY_V);             \
    X(KEY_W);             \
    X(KEY_X);             \
    X(KEY_Y);             \
    X(KEY_Z);             \
    X(KEY_SPACE);         \
    X(KEY_ESCAPE);        \
    X(KEY_ENTER);         \
    X(KEY_TAB);           \
    X(KEY_BACKSPACE);     \
    X(KEY_INSERT);        \
    X(KEY_DELETE);        \
    X(KEY_RIGHT);         \
    X(KEY_LEFT);          \
    X(KEY_DOWN);          \
    X(KEY_UP);            \
    X(KEY_PAGE_UP);       \
    X(KEY_PAGE_DOWN);     \
    X(KEY_HOME);          \
    X(KEY_END);           \
    X(KEY_CAPS_LOCK);     \
    X(KEY_SCROLL_LOCK);   \
    X(KEY_NUM_LOCK);      \
    X(KEY_PRINT_SCREEN);  \
    X(KEY_PAUSE);         \
    X(KEY_F1);            \
    X(KEY_F2);            \
    X(KEY_F3);            \
    X(KEY_F4);            \
    X(KEY_F5);            \
    X(KEY_F6);            \
    X(KEY_F7);            \
    X(KEY_F8);            \
    X(KEY_F9);            \
    X(KEY_F10);           \
    X(KEY_F11);           \
    X(KEY_F12);           \
    X(KEY_LEFT_SHIFT);    \
    X(KEY_LEFT_CONTROL);  \
    X(KEY_LEFT_ALT);      \
    X(KEY_LEFT_SUPER);    \
    X(KEY_RIGHT_SHIFT);   \
    X(KEY_RIGHT_CONTROL); \
    X(KEY_RIGHT_ALT);     \
    X(KEY_RIGHT_SUPER);   \
    X(KEY_KB_MENU);       \
    X(KEY_LEFT_BRACKET);  \
    X(KEY_BACKSLASH);     \
    X(KEY_RIGHT_BRACKET); \
    X(KEY_GRAVE);         \
    X(KEY_KP_0);          \
    X(KEY_KP_1);          \
    X(KEY_KP_2);          \
    X(KEY_KP_3);          \
    X(KEY_KP_4);          \
    X(KEY_KP_5);          \
    X(KEY_KP_6);          \
    X(KEY_KP_7);          \
    X(KEY_KP_8);          \
    X(KEY_KP_9);          \
    X(KEY_KP_DECIMAL);    \
    X(KEY_KP_DIVIDE);     \
    X(KEY_KP_MULTIPLY);   \
    X(KEY_KP_SUBTRACT);   \
    X(KEY_KP_ADD);        \
    X(KEY_KP_ENTER);      \
    X(KEY_KP_EQUAL);

void RaylibImGui::Init()
{
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
}

#define SET_KEY_DOWN(KEY) io.KeysDown[KEY] = IsKeyDown(KEY)
void RaylibImGui::Begin()
{
    ImGuiIO &io = ImGui::GetIO();

    if (!(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange))
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
    }

    // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
    if (io.WantSetMousePos)
    {
        SetMousePosition(io.MousePos.x, io.MousePos.y);
    }

    if (!IsWindowMinimized())
    {
        io.MousePos = {(float)GetTouchX(), (float)GetTouchY()};
    }
    else
    {
        io.MousePos = {-FLT_MAX, -FLT_MAX};
    }

    io.MouseDown[0] = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    io.MouseDown[1] = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);
    io.MouseDown[2] = IsMouseButtonDown(MOUSE_MIDDLE_BUTTON);

    io.KeyCtrl = IsKeyDown(KEY_RIGHT_CONTROL) || IsKeyDown(KEY_LEFT_CONTROL);
    io.KeyShift = IsKeyDown(KEY_RIGHT_SHIFT) || IsKeyDown(KEY_LEFT_SHIFT);
    io.KeyAlt = IsKeyDown(KEY_RIGHT_ALT) || IsKeyDown(KEY_LEFT_ALT);
    io.KeySuper = IsKeyDown(KEY_RIGHT_SUPER) || IsKeyDown(KEY_LEFT_SUPER);

    if (GetMouseWheelMove() > 0)
        io.MouseWheel += 1;
    else if (GetMouseWheelMove() < 0)
        io.MouseWheel -= 1;

    FOR_ALL_KEYS(SET_KEY_DOWN);

    int keyPressed = GetKeyPressed();
    if (keyPressed > 0)
    {
        io.AddInputCharacter(keyPressed);
    }

    io.DisplaySize.x = (float)GetScreenWidth();
    io.DisplaySize.y = (float)GetScreenHeight();

    // Avoids having to pass in delta time
    double currentTime = GetTime();
    io.DeltaTime = lastTime > 0.0 ? (float)(currentTime - lastTime) : (float)(1.0f / 60.0f);
    lastTime = currentTime;

    ImGui_ImplOpenGL3_NewFrame();
}

void RaylibImGui::End()
{
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}