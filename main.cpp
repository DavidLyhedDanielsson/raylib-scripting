#include "raylib.h"
#include <filesystem>

const static std::filesystem::path ASSET_ROOT(DASSET_ROOT);

int main()
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Raylib test");

    SetTargetFPS(60);

    Camera camera = {0};
    camera.position = (Vector3){5.0f, 5.0f, 5.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Model model = LoadModel((ASSET_ROOT / "Bob/glTF/Bob.gltf").c_str());

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        DrawModel(model, (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
        EndMode3D();

        EndDrawing();
    }

    CloseWindow();

    return 0;
}