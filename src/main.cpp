#include <iostream>
#include <raylib.h>

int main() {

    InitWindow(800, 600, "Ludum Dare 58");
    Model model = LoadModel("../assets/char.obj");

    Camera3D camera = {0};
    camera.position = (Vector3){10, 10, 10};
    camera.target = (Vector3){0, 0, 0};
    camera.up = (Vector3){0, 1, 0};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose()) {

        // Update loop
        UpdateCamera(&camera, CAMERA_ORBITAL);

        // Draw
        BeginDrawing();
        ClearBackground(DARKGRAY);

        // Draw 3D
        BeginMode3D(camera);

        DrawGrid(10, 1.0f);
        DrawPlane(Vector3{0, 0, 0}, Vector2{20, 20}, GRAY);

        // DrawModel(model, (Vector3){0, 0, 0}, 1.0f, WHITE);

        DrawCapsule(Vector3{0, 0, 0}, Vector3{0, 2, 0}, 0.5f, 10, 10, RED);

        DrawLine3D(Vector3{0, 0, 0}, Vector3{2, 0, 0}, RED);
        DrawLine3D(Vector3{0, 0, 0}, Vector3{0, 2, 0}, GREEN);
        DrawLine3D(Vector3{0, 0, 0}, Vector3{0, 0, 2}, BLUE);

        EndMode3D();

        // Draw UI
        DrawFPS(10, 10);

        EndDrawing();
    }

    UnloadModel(model);
    CloseWindow();
}