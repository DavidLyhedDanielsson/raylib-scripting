#include <raylib.h>
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <math.h>
#include <array>
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <imgui.h>
#include <imgui_impl_opengl3.h>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

// Windows clocks in at 260, linux allows 4096
constexpr int MAX_PATH_LENGTH = 256;
std::array<char, MAX_PATH_LENGTH> pathBuffer = {0}; // Helper to avoid dynamic allocation
std::array<char, MAX_PATH_LENGTH> AssetPath(const char *assetName)
{
    snprintf(pathBuffer.data(), MAX_PATH_LENGTH, "%s/%s", DASSET_ROOT, assetName);
    return pathBuffer;
}

Camera camera = {0};
Model model;

std::chrono::time_point<std::chrono::high_resolution_clock> last;

constexpr float TICK_RATE = 1000.0f / 60.0f;
float timer = 0.0f;

lua_State *luaState;
ImGuiContext *guiContext;

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

#define SET_KEY_DOWN(KEY) io.KeysDown[KEY] = IsKeyDown(KEY)
void update_imgui()
{
    ImGuiIO &io = ImGui::GetIO();

    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
    {
        HideCursor();
    }
    else
    {
        ShowCursor();
    }

    if (io.WantSetMousePos)
        SetMousePosition(io.MousePos.x, io.MousePos.y);
    else
        io.MousePos = {-FLT_MAX, -FLT_MAX};

    io.MouseDown[0] = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    io.MouseDown[1] = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);
    io.MouseDown[2] = IsMouseButtonDown(MOUSE_MIDDLE_BUTTON);

    if (!IsWindowMinimized())
    {
        io.MousePos = {(float)GetTouchX(), (float)GetTouchY()};
    }

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
}

void main_loop()
{
    auto now = std::chrono::high_resolution_clock::now();
    float deltaMs = std::chrono::duration<float, std::milli>(now - last).count();
    last = now;
    timer += deltaMs;

    float rotation = (3.141593f * 2.0f) * (timer / 5000.0f);

    camera.position = (Vector3){cosf(rotation) * 10.0f, 5.0f, sinf(rotation) * 10.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    update_imgui();

    BeginDrawing();

    ClearBackground(RAYWHITE);

    char buf[128];
    sprintf(buf, "Frame time: %f", deltaMs);
    DrawText(buf, 0, 0, 20, LIGHTGRAY);

    BeginMode3D(camera);
    DrawModel(model, (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
    EndMode3D();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Hello");
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    EndDrawing();
}

int main()
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Raylib test");

    model = LoadModel(AssetPath("Bob/glTF/Bob.gltf").data());

    luaState = luaL_newstate();
    luaL_openlibs(luaState);
    luaL_loadstring(luaState, "print(\"Hello world\")");
    lua_pcall(luaState, 0, 0, 0);

    guiContext = ImGui::CreateContext();
    last = std::chrono::high_resolution_clock::now();

    ImGui_ImplOpenGL3_Init();
#ifdef PLATFORM_WEB
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        main_loop();
    }
#endif

    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    CloseWindow();

    return 0;
}