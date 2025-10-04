#include <iostream>
#include <raylib.h>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

int main() {

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Ludum Dare 58");
    SetTargetFPS(60);

    // Player state
    Vector2 playerPos = { 8.0f, 8.0f };
    float playerAngle = 0.0f;
    const float moveSpeed = 3.0f;
    const float rotSpeed = 2.0f;
    const float mouseSensitivity = 0.003f;

    // 60 degree field of view.
    const float FOV = 60.0f * DEG2RAD;

    while (!WindowShouldClose()) {
        float deltaTiem = GetFrameTime();

        // MOUSE LOOK
        Vector2 mouseDelta = GetMouseDelta();
        playerAngle -= mouseDelta.x * mouseSensitivity;
        
        // Calculate direction vectors
        Vector2 dirVec = { cosf(playerAngle), sinf(playerAngle) };

        // Update loop

        // Draw
        {
            BeginDrawing();
            ClearBackground(DARKGRAY);

            // Draw UI
            DrawFPS(10, 10);

            EndDrawing();
        }
    }

    CloseWindow();
}