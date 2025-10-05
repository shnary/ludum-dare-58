#include <raylib.h>
#include "game.h"

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "MazeKiller3D - LD58");
    SetTargetFPS(60);
    
    GameState game = {0};
    InitGame(&game);
    
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        
        // Check if exit was selected from menu
        if (game.mode == MAIN_MENU && game.menuSelection == 1 && 
            (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))) {
            break;
        }
        
        // Disable cursor only during gameplay
        if (game.mode == PLAYING) {
            if (!IsCursorHidden()) DisableCursor();
        } else {
            if (IsCursorHidden()) EnableCursor();
        }
        
        UpdateGame(&game, deltaTime);
        DrawGame(&game);
    }
    
    if (IsSoundReady(game.collectSound)) UnloadSound(game.collectSound);
    if (IsSoundReady(game.stabSound)) UnloadSound(game.stabSound);
    if (IsSoundReady(game.purchaseSound)) UnloadSound(game.purchaseSound);
    if (IsMusicReady(game.horrorMusic)) {
        if (IsMusicStreamPlaying(game.horrorMusic)) {
            StopMusicStream(game.horrorMusic);
        }
        UnloadMusicStream(game.horrorMusic);
    }
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
