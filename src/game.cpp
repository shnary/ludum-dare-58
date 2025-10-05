#include "game.h"
#include "map.h"
#include "collectible.h"
#include "enemy.h"
#include <raymath.h>
#include <cmath>
#include <stdlib.h>
#include <time.h>

void InitGame(GameState* game) {
    srand(time(NULL));
    
    // Only initialize audio and load resources once
    static bool audioInitialized = false;
    if (!audioInitialized) {
        InitAudioDevice();
        game->collectSound = LoadSound("assets/collect.wav");
        game->stabSound = LoadSound("assets/stab.wav");
        game->purchaseSound = LoadSound("assets/purchase.wav");
        game->jumpscareSound = LoadSound("assets/jumpscare.wav");
        game->horrorMusic = LoadMusicStream("assets/horror-music.mp3");
        
        if (!IsSoundReady(game->collectSound)) {
            TraceLog(LOG_ERROR, "Failed to load collect.wav");
        }
        if (!IsSoundReady(game->stabSound)) {
            TraceLog(LOG_ERROR, "Failed to load stab.wav");
        }
        if (!IsSoundReady(game->purchaseSound)) {
            TraceLog(LOG_ERROR, "Failed to load purchase.wav");
        }
        if (!IsSoundReady(game->jumpscareSound)) {
            TraceLog(LOG_ERROR, "Failed to load jumpscare.wav");
        }
        if (!IsMusicReady(game->horrorMusic)) {
            TraceLog(LOG_ERROR, "Failed to load horror-music.mp3");
        }
        SetSoundVolume(game->collectSound, 0.5f);
        SetSoundVolume(game->stabSound, 0.7f);
        SetSoundVolume(game->purchaseSound, 0.6f);
        SetSoundVolume(game->jumpscareSound, 0.2f);
        SetMusicVolume(game->horrorMusic, 0.4f);
        game->horrorMusic.looping = true;
        
        audioInitialized = true;
    }
    
    // Stop music if playing before resetting
    if (IsMusicReady(game->horrorMusic) && IsMusicStreamPlaying(game->horrorMusic)) {
        StopMusicStream(game->horrorMusic);
    }
    
    // Start music in main menu
    if (IsMusicReady(game->horrorMusic)) {
        PlayMusicStream(game->horrorMusic);
    }
    
    game->player.angle = 0.0f;
    game->player.mouseSensitivity = 0.003f;
    game->player.hasSpeedBoost = false;
    game->player.boostTimer = 0.0f;
    game->player.goldMultiplier = 1.0f;
    game->player.baseSpeed = 3.0f;
    game->player.moveSpeed = game->player.baseSpeed;
    
    game->totalGold = 0;
    game->animTime = 0.0f;
    game->FOV = 60.0f * DEG2RAD;
    game->currentLevel = 1;
    game->mode = MAIN_MENU;
    game->loadingTimer = 0.0f;
    game->selectedPerk = -1;
    game->stabEffectTimer = 0.0f;
    game->isBeingAttacked = false;
    game->shopContinuePressed = false;
    game->menuSelection = 0;
    game->showEnemyOnMinimap = false;
}

void InitLevel(GameState* game, int level) {
    game->currentLevel = level;
    game->stabEffectTimer = 0.0f;
    game->isBeingAttacked = false;
    
    // Set map and door cost based on level
    if (level == 1) {
        currentMap = (const int*)worldMap1;
        currentMapWidth = MAP_WIDTH;
        currentMapHeight = MAP_HEIGHT;
        game->player.position = { 8.0f, 8.0f };
        game->doorCost = 50;
    } else if (level == 2) {
        currentMap = (const int*)worldMap2;
        currentMapWidth = MAP_WIDTH_2;
        currentMapHeight = MAP_HEIGHT_2;
        game->player.position = { 10.0f, 10.0f };
        game->doorCost = 80;
    } else if (level == 3) {
        currentMap = (const int*)worldMap3;
        currentMapWidth = MAP_WIDTH_2;
        currentMapHeight = MAP_HEIGHT_2;
        game->player.position = { 10.0f, 10.0f };
        game->doorCost = 120;
    } else if (level == 4) {
        currentMap = (const int*)worldMap4;
        currentMapWidth = MAP_WIDTH_2;
        currentMapHeight = MAP_HEIGHT_2;
        game->player.position = { 10.0f, 10.0f };
        game->doorCost = 150;
    } else if (level == 5) {
        currentMap = (const int*)worldMap5;
        currentMapWidth = MAP_WIDTH_2;
        currentMapHeight = MAP_HEIGHT_2;
        game->player.position = { 10.0f, 10.0f };
        game->doorCost = 200;
    }
    
    game->collectibles = InitCollectibles(level);
    InitEnemy(&game->enemy, game->player.position, currentMapWidth, currentMapHeight, level);
    game->mode = PLAYING;
    
    // Start music when level begins (if not already playing)
    if (IsMusicReady(game->horrorMusic) && !IsMusicStreamPlaying(game->horrorMusic)) {
        PlayMusicStream(game->horrorMusic);
    }
}

void GenerateShopPerks(GameState* game) {
    // Perk 0: Gold Multiplier
    game->shopPerks[0].type = 0;
    game->shopPerks[0].name = "Gold Collector";
    game->shopPerks[0].description = "Collect +50% more gold";
    game->shopPerks[0].cost = 30 + (rand() % 20);
    game->shopPerks[0].value = 0.5f;
    
    // Perk 1: Speed Boost
    game->shopPerks[1].type = 1;
    game->shopPerks[1].name = "Speed Runner";
    game->shopPerks[1].description = "+1.0 movement speed";
    game->shopPerks[1].cost = 20 + (rand() % 15);
    game->shopPerks[1].value = 1.0f;
    
    // Perk 2: Boost Duration or Enemy Radar
    if (!game->showEnemyOnMinimap && game->currentLevel >= 3) {
        // Enemy Radar (only if not already purchased, appears after level 2)
        game->shopPerks[2].type = 3;
        game->shopPerks[2].name = "Enemy Radar";
        game->shopPerks[2].description = "Reveals enemy on minimap";
        game->shopPerks[2].cost = 40 + (rand() % 20);
        game->shopPerks[2].value = 1.0f;
    } else {
        // Boost Duration
        game->shopPerks[2].type = 2;
        game->shopPerks[2].name = "Long Boost";
        game->shopPerks[2].description = "Speed boosts last +5s";
        game->shopPerks[2].cost = 25 + (rand() % 15);
        game->shopPerks[2].value = 5.0f;
    }
}

void UpdateGame(GameState* game, float deltaTime) {
    game->animTime += deltaTime;
    
    // Update music stream only when playing
    if (IsMusicReady(game->horrorMusic) && IsMusicStreamPlaying(game->horrorMusic)) {
        UpdateMusicStream(game->horrorMusic);
    }
    
    if (game->mode == MAIN_MENU) {
        // Menu navigation
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            game->menuSelection = 0;
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            game->menuSelection = 1;
        }
        
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (game->menuSelection == 0) {
                // Start game
                InitLevel(game, 1);
            } else {
                // Exit game
                // Note: Actual exit handled in main.cpp
            }
        }
        return;
    }
    
    if (game->mode == GAME_WON) {
        // Press R to restart
        if (IsKeyPressed(KEY_R)) {
            InitGame(game);
        }
        return;
    }
    
    if (game->mode == GAME_LOST) {
        // Press SPACE to return to main menu
        if (IsKeyPressed(KEY_SPACE)) {
            InitGame(game);
        }
        return;
    }
    
    if (game->mode == LOADING) {
        game->loadingTimer += deltaTime;
        if (game->loadingTimer >= 1.5f) {  // Faster loading
            if (game->currentLevel > MAX_LEVELS) {
                game->mode = GAME_WON;
                // Stop music when game is won
                if (IsMusicReady(game->horrorMusic)) {
                    StopMusicStream(game->horrorMusic);
                }
            } else {
                game->mode = SHOP;
                game->loadingTimer = 0.0f;
                GenerateShopPerks(game);
            }
        }
        return;
    }
    
    if (game->mode == SHOP) {
        // Shop input handling
        if (IsKeyPressed(KEY_ONE)) game->selectedPerk = 0;
        if (IsKeyPressed(KEY_TWO)) game->selectedPerk = 1;
        if (IsKeyPressed(KEY_THREE)) game->selectedPerk = 2;
        
        // Buy perk
        if (game->selectedPerk >= 0 && IsKeyPressed(KEY_ENTER)) {
            Perk* perk = &game->shopPerks[game->selectedPerk];
            if (game->totalGold >= perk->cost) {
                game->totalGold -= perk->cost;
                
                if (IsSoundReady(game->purchaseSound)) {
                    PlaySound(game->purchaseSound);
                }
                
                if (perk->type == 0) {
                    game->player.goldMultiplier += perk->value;
                } else if (perk->type == 1) {
                    game->player.baseSpeed += perk->value;
                    game->player.moveSpeed = game->player.baseSpeed;
                } else if (perk->type == 2) {
                    // Boost duration - handled in boost
                } else if (perk->type == 3) {
                    game->showEnemyOnMinimap = true;
                }
                
                game->selectedPerk = -1;
            }
        }
        
        // Continue to next level (double press confirmation)
        if (IsKeyPressed(KEY_SPACE)) {
            if (!game->shopContinuePressed) {
                game->shopContinuePressed = true;
            } else {
                // Second press - continue
                game->shopContinuePressed = false;
                InitLevel(game, game->currentLevel);
            }
        }
        return;
    }
    
    // PLAYING mode
    // Update stab effect
    if (game->stabEffectTimer > 0) {
        game->stabEffectTimer -= deltaTime;
        if (game->stabEffectTimer <= 0) {
            game->isBeingAttacked = false;
        }
    }
    
    // Update enemy
    UpdateEnemy(&game->enemy, game->player.position, deltaTime);
    
    // Play jumpscare sound in loop when enemy is close
    if (game->enemy.isActive) {
        float distToEnemy = Vector2Distance(game->enemy.position, game->player.position);
        float jumpscareDistance = 2.7f; // Start playing when enemy is within 4 units
        
        if (distToEnemy < jumpscareDistance) {
            // Keep sound playing in loop (restart when finished)
            if (IsSoundReady(game->jumpscareSound) && !IsSoundPlaying(game->jumpscareSound)) {
                PlaySound(game->jumpscareSound);
            }
        } else {
            // Stop sound when enemy is far
            if (IsSoundReady(game->jumpscareSound) && IsSoundPlaying(game->jumpscareSound)) {
                StopSound(game->jumpscareSound);
            }
        }
    }
    
    // Check if caught by enemy
    if (IsPlayerCaught(&game->enemy, game->player.position)) {
        if (!game->isBeingAttacked) {
            game->isBeingAttacked = true;
            game->stabEffectTimer = 2.0f;  // Stab effect duration
            if (IsSoundReady(game->stabSound)) {
                PlaySound(game->stabSound);
            }
        }
        
        if (game->stabEffectTimer < 1.5f) {  // Die after 0.5s of effect
            game->mode = GAME_LOST;
            // Stop music when player dies
            if (IsMusicReady(game->horrorMusic)) {
                StopMusicStream(game->horrorMusic);
            }
            // Stop jumpscare sound
            if (IsSoundReady(game->jumpscareSound) && IsSoundPlaying(game->jumpscareSound)) {
                StopSound(game->jumpscareSound);
            }
            return;
        }
    }
    
    // Update speed boost timer
    if (game->player.hasSpeedBoost) {
        game->player.boostTimer -= deltaTime;
        if (game->player.boostTimer <= 0.0f) {
            game->player.hasSpeedBoost = false;
            game->player.boostTimer = 0.0f;
        }
    }
    
    // Rotation
    Vector2 mouseDelta = GetMouseDelta();
    game->player.angle += mouseDelta.x * game->player.mouseSensitivity;
    
    // Direction vectors
    Vector2 dirVec = { cosf(game->player.angle), sinf(game->player.angle) };
    Vector2 rightVec = { cosf(game->player.angle + PI/2), sinf(game->player.angle + PI/2) };
    
    // Movement
    Vector2 moveDir = { 0, 0 };
    if (IsKeyDown(KEY_W)) { moveDir.x += dirVec.x; moveDir.y += dirVec.y; }
    if (IsKeyDown(KEY_S)) { moveDir.x -= dirVec.x; moveDir.y -= dirVec.y; }
    if (IsKeyDown(KEY_A)) { moveDir.x -= rightVec.x; moveDir.y -= rightVec.y; }
    if (IsKeyDown(KEY_D)) { moveDir.x += rightVec.x; moveDir.y += rightVec.y; }
    
    // Apply movement with collision
    if (moveDir.x != 0 || moveDir.y != 0) {
        float len = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        moveDir.x /= len;
        moveDir.y /= len;
        
        // Apply speed boost (2x multiplier)
        float speedMultiplier = game->player.hasSpeedBoost ? 2.0f : 1.0f;
        float moveX = moveDir.x * game->player.moveSpeed * speedMultiplier * deltaTime;
        float moveY = moveDir.y * game->player.moveSpeed * speedMultiplier * deltaTime;
        
        // Try X movement
        float newX = game->player.position.x + moveX;
        int mapX = (int)newX;
        int mapY = (int)game->player.position.y;
        int tile = GetMapTile(mapX, mapY);
        
        // Check if passing through door
        if (tile == 2 && game->totalGold >= game->doorCost) {
            game->mode = LOADING;
            game->loadingTimer = 0.0f;
            game->currentLevel++;
            // Stop music when entering door
            if (IsMusicReady(game->horrorMusic)) {
                StopMusicStream(game->horrorMusic);
            }
            // Stop jumpscare sound
            if (IsSoundReady(game->jumpscareSound) && IsSoundPlaying(game->jumpscareSound)) {
                StopSound(game->jumpscareSound);
            }
            return;
        }
        
        if (tile == 0 || (tile == 2 && game->totalGold >= game->doorCost)) {
            game->player.position.x = newX;
        }
        
        // Try Y movement
        float newY = game->player.position.y + moveY;
        mapX = (int)game->player.position.x;
        mapY = (int)newY;
        tile = GetMapTile(mapX, mapY);
        
        if (tile == 2 && game->totalGold >= game->doorCost) {
            game->mode = LOADING;
            game->loadingTimer = 0.0f;
            game->currentLevel++;
            // Stop music when entering door
            if (IsMusicReady(game->horrorMusic)) {
                StopMusicStream(game->horrorMusic);
            }
            // Stop jumpscare sound
            if (IsSoundReady(game->jumpscareSound) && IsSoundPlaying(game->jumpscareSound)) {
                StopSound(game->jumpscareSound);
            }
            return;
        }
        
        if (tile == 0 || (tile == 2 && game->totalGold >= game->doorCost)) {
            game->player.position.y = newY;
        }
    }
    
    // Update collectibles
    UpdateCollectibles(game->collectibles, game->player.position, game->totalGold, 
                      game->player.hasSpeedBoost, game->player.boostTimer, game->player.goldMultiplier, game->collectSound);
}

void DrawStabEffect(float intensity) {
    // Red bloody knife slash effect
    unsigned char alpha = (unsigned char)(intensity * 200);
    
    // Red vignette
    for (int i = 0; i < 100; i++) {
        unsigned char vignetteAlpha = (unsigned char)(i * 2.5f * intensity);
        DrawRectangleLinesEx(
            Rectangle{(float)i, (float)i, (float)SCREEN_WIDTH - i*2, (float)SCREEN_HEIGHT - i*2},
            2, Color{255, 0, 0, vignetteAlpha}
        );
    }
    
    // Slash marks
    DrawLineEx(Vector2{100, 100}, Vector2{400, 300}, 5, Color{255, 0, 0, alpha});
    DrawLineEx(Vector2{500, 150}, Vector2{700, 400}, 5, Color{255, 0, 0, alpha});
    DrawLineEx(Vector2{200, 400}, Vector2{350, 500}, 5, Color{255, 0, 0, alpha});
    
    // Screen flash
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{255, 0, 0, (unsigned char)(alpha * 0.3f)});
}

void DrawLoadingScreen(float progress) {
    BeginDrawing();
    ClearBackground(BLACK);
    
    // Half-Life style loading
    int barWidth = 400;
    int barHeight = 30;
    int barX = SCREEN_WIDTH / 2 - barWidth / 2;
    int barY = SCREEN_HEIGHT / 2 - barHeight / 2;
    
    DrawRectangle(barX, barY, barWidth, barHeight, DARKGRAY);
    DrawRectangle(barX, barY, (int)(barWidth * progress), barHeight, ORANGE);
    DrawRectangleLines(barX - 2, barY - 2, barWidth + 4, barHeight + 4, WHITE);
    
    const char* text = "LOADING...";
    int textWidth = MeasureText(text, 40);
    DrawText(text, SCREEN_WIDTH / 2 - textWidth / 2, barY - 60, 40, WHITE);
    
    const char* percentText = TextFormat("%.0f%%", progress * 100);
    int percentWidth = MeasureText(percentText, 20);
    DrawText(percentText, SCREEN_WIDTH / 2 - percentWidth / 2, barY + barHeight + 20, 20, WHITE);
    
    EndDrawing();
}

void DrawShop(const GameState* game) {
    BeginDrawing();
    ClearBackground(Color{20, 20, 30, 255});
    
    // Title
    const char* title = "UPGRADE SHOP";
    int titleWidth = MeasureText(title, 50);
    DrawText(title, SCREEN_WIDTH / 2 - titleWidth / 2, 20, 50, GOLD);
    
    // Current gold
    const char* goldText = TextFormat("Current Gold: $%d", game->totalGold);
    int goldWidth = MeasureText(goldText, 30);
    DrawText(goldText, SCREEN_WIDTH / 2 - goldWidth / 2, 80, 30, GREEN);
    
    // Current stats box
    DrawRectangle(20, 130, 260, 120, Color{30, 30, 50, 255});
    DrawRectangleLines(20, 130, 260, 120, GOLD);
    DrawText("CURRENT STATS:", 30, 140, 20, YELLOW);
    DrawText(TextFormat("Speed: %.1f", game->player.baseSpeed), 30, 170, 18, WHITE);
    DrawText(TextFormat("Gold Multiplier: %.1fx", game->player.goldMultiplier), 30, 195, 18, WHITE);
    DrawText(TextFormat("Level: %d/%d", game->currentLevel, MAX_LEVELS), 30, 220, 18, WHITE);
    
    // Draw perks
    int perkWidth = 220;
    int perkHeight = 200;
    int spacing = 20;
    int startX = (SCREEN_WIDTH - (perkWidth * 3 + spacing * 2)) / 2;
    int startY = 270;
    
    for (int i = 0; i < 3; i++) {
        int x = startX + i * (perkWidth + spacing);
        int y = startY;
        
        Perk perk = game->shopPerks[i];
        bool canAfford = game->totalGold >= perk.cost;
        bool selected = game->selectedPerk == i;
        
        Color boxColor = selected ? Color{80, 80, 120, 255} : Color{40, 40, 60, 255};
        DrawRectangle(x, y, perkWidth, perkHeight, boxColor);
        DrawRectangleLines(x, y, perkWidth, perkHeight, selected ? GOLD : GRAY);
        
        DrawText(TextFormat("[%d]", i + 1), x + 10, y + 10, 20, YELLOW);
        DrawText(perk.name, x + 10, y + 40, 20, WHITE);
        DrawText(perk.description, x + 10, y + 70, 16, LIGHTGRAY);
        
        Color costColor = canAfford ? GREEN : RED;
        DrawText(TextFormat("Cost: $%d", perk.cost), x + 10, y + 140, 20, costColor);
        
        if (!canAfford) {
            DrawText("Too expensive!", x + 10, y + 170, 14, RED);
        } else if (selected) {
            DrawText("Press ENTER", x + 10, y + 170, 14, GREEN);
        }
    }
    
    // Continue button with confirmation
    const char* continueText;
    Color buttonColor;
    if (!game->shopContinuePressed) {
        continueText = "Press SPACE to continue to next level";
        buttonColor = Color{0, 100, 0, 200};
    } else {
        continueText = "Are you sure? Press SPACE again";
        buttonColor = Color{150, 100, 0, 200};
    }
    
    int contWidth = MeasureText(continueText, 25);
    DrawRectangle(SCREEN_WIDTH / 2 - contWidth / 2 - 20, SCREEN_HEIGHT - 80, contWidth + 40, 50, buttonColor);
    DrawRectangleLines(SCREEN_WIDTH / 2 - contWidth / 2 - 20, SCREEN_HEIGHT - 80, contWidth + 40, 50, game->shopContinuePressed ? ORANGE : GREEN);
    DrawText(continueText, SCREEN_WIDTH / 2 - contWidth / 2, SCREEN_HEIGHT - 65, 25, WHITE);
    
    EndDrawing();
}

void DrawGameWon(const GameState* game) {
    BeginDrawing();
    ClearBackground(Color{10, 30, 10, 255});
    
    // Victory text
    const char* title = "YOU ESCAPED!";
    int titleWidth = MeasureText(title, 60);
    DrawText(title, SCREEN_WIDTH / 2 - titleWidth / 2, 150, 60, GREEN);
    
    // Description
    const char* desc = "You successfully escaped the maze killer!";
    int descWidth = MeasureText(desc, 25);
    DrawText(desc, SCREEN_WIDTH / 2 - descWidth / 2, 230, 25, WHITE);
    
    // Stats
    DrawText(TextFormat("Levels Completed: %d", MAX_LEVELS), SCREEN_WIDTH / 2 - 120, 300, 22, YELLOW);
    DrawText(TextFormat("Final Gold: $%d", game->totalGold), SCREEN_WIDTH / 2 - 100, 330, 22, GOLD);
    DrawText(TextFormat("Final Speed: %.1f", game->player.baseSpeed), SCREEN_WIDTH / 2 - 100, 360, 22, SKYBLUE);
    
    // Restart
    const char* restart = "Press R to restart";
    int restartWidth = MeasureText(restart, 30);
    DrawText(restart, SCREEN_WIDTH / 2 - restartWidth / 2, 420, 30, LIGHTGRAY);
    
    // Game credits at bottom
    const char* gameTitle = "MazeKiller3D";
    int gameTitleWidth = MeasureText(gameTitle, 30);
    DrawText(gameTitle, SCREEN_WIDTH / 2 - gameTitleWidth / 2, SCREEN_HEIGHT - 80, 30, Color{150, 255, 150, 255});
    
    const char* credits = "A Ludum Dare 58 Game";
    int creditsWidth = MeasureText(credits, 20);
    DrawText(credits, SCREEN_WIDTH / 2 - creditsWidth / 2, SCREEN_HEIGHT - 45, 20, Color{100, 200, 100, 255});
    
    EndDrawing();
}

void DrawGameLost(const GameState* game) {
    BeginDrawing();
    ClearBackground(Color{30, 10, 10, 255});
    
    // Game over text
    const char* title = "YOU DIED";
    int titleWidth = MeasureText(title, 70);
    DrawText(title, SCREEN_WIDTH / 2 - titleWidth / 2, 150, 70, RED);
    
    // Description
    const char* desc = "You were stabbed by the maze killer...";
    int descWidth = MeasureText(desc, 25);
    DrawText(desc, SCREEN_WIDTH / 2 - descWidth / 2, 240, 25, Color{200, 100, 100, 255});
    
    // Stats
    DrawText(TextFormat("Reached Level: %d/%d", game->currentLevel, MAX_LEVELS), SCREEN_WIDTH / 2 - 120, 320, 22, YELLOW);
    DrawText(TextFormat("Gold Collected: $%d", game->totalGold), SCREEN_WIDTH / 2 - 120, 350, 22, GOLD);
    
    // Return to menu
    const char* menuText = "Press SPACE to return to main menu";
    int menuWidth = MeasureText(menuText, 28);
    DrawText(menuText, SCREEN_WIDTH / 2 - menuWidth / 2, 420, 28, LIGHTGRAY);
    
    // Game credits at bottom
    const char* gameTitle = "MazeKiller3D";
    int gameTitleWidth = MeasureText(gameTitle, 30);
    DrawText(gameTitle, SCREEN_WIDTH / 2 - gameTitleWidth / 2, SCREEN_HEIGHT - 80, 30, Color{255, 150, 150, 255});
    
    const char* credits = "A Ludum Dare 58 Game";
    int creditsWidth = MeasureText(credits, 20);
    DrawText(credits, SCREEN_WIDTH / 2 - creditsWidth / 2, SCREEN_HEIGHT - 45, 20, Color{200, 100, 100, 255});
    
    EndDrawing();
}

void DrawMainMenu(const GameState* game) {
    BeginDrawing();
    ClearBackground(Color{10, 5, 5, 255});
    
    // Spooky background effect - dark red vignette
    for (int i = 0; i < 80; i++) {
        unsigned char alpha = (unsigned char)(i * 1.2f);
        DrawRectangleLinesEx(
            Rectangle{(float)i, (float)i, (float)SCREEN_WIDTH - i*2, (float)SCREEN_HEIGHT - i*2},
            1, Color{80, 0, 0, alpha}
        );
    }
    
    // Animated enemy figure in background
    float bobOffset = sinf(game->animTime * 1.5f) * 20.0f;
    int enemyX = SCREEN_WIDTH / 2;
    int enemyY = SCREEN_HEIGHT / 2 + 50 + (int)bobOffset;
    
    // Large shadowy enemy figure
    DrawCircle(enemyX, enemyY - 80, 60, Color{40, 0, 0, 150}); // Head
    DrawRectangle(enemyX - 40, enemyY - 20, 80, 120, Color{40, 0, 0, 150}); // Body
    
    // Glowing red eyes
    float pulseIntensity = (sinf(game->animTime * 3.0f) + 1.0f) / 2.0f;
    unsigned char eyeAlpha = (unsigned char)(150 + pulseIntensity * 105);
    DrawCircle(enemyX - 20, enemyY - 85, 8, Color{255, 0, 0, eyeAlpha});
    DrawCircle(enemyX + 20, enemyY - 85, 8, Color{255, 0, 0, eyeAlpha});
    
    // Knife glint
    DrawLine(enemyX + 50, enemyY + 20, enemyX + 70, enemyY - 10, Color{200, 200, 200, 100});
    
    // Dark overlay for menu readability
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{0, 0, 0, 100});
    
    // Title with blood drip effect
    const char* title = "MazeKiller3D";
    int titleWidth = MeasureText(title, 80);
    DrawText(title, SCREEN_WIDTH / 2 - titleWidth / 2 + 2, 122, 80, Color{80, 0, 0, 255}); // Shadow
    DrawText(title, SCREEN_WIDTH / 2 - titleWidth / 2, 120, 80, Color{200, 0, 0, 255}); // Dark red
    
    // Subtitle
    const char* subtitle = "A Ludum Dare 58 Game";
    int subtitleWidth = MeasureText(subtitle, 25);
    DrawText(subtitle, SCREEN_WIDTH / 2 - subtitleWidth / 2, 210, 25, Color{150, 50, 50, 255});
    
    // Menu options with spooky style
    const char* startText = "Start Game";
    const char* exitText = "Exit";
    
    int startY = 300;
    int spacing = 60;
    
    // Start button
    Color startColor = game->menuSelection == 0 ? Color{255, 200, 0, 255} : Color{200, 150, 150, 255};
    int startWidth = MeasureText(startText, 40);
    if (game->menuSelection == 0) {
        DrawRectangle(SCREEN_WIDTH / 2 - startWidth / 2 - 20, startY - 10, startWidth + 40, 50, Color{80, 0, 0, 150});
        DrawRectangleLines(SCREEN_WIDTH / 2 - startWidth / 2 - 20, startY - 10, startWidth + 40, 50, Color{200, 0, 0, 255});
        DrawText(">", SCREEN_WIDTH / 2 - startWidth / 2 - 50, startY, 40, Color{255, 200, 0, 255});
    }
    DrawText(startText, SCREEN_WIDTH / 2 - startWidth / 2, startY, 40, startColor);
    
    // Exit button
    Color exitColor = game->menuSelection == 1 ? Color{255, 200, 0, 255} : Color{200, 150, 150, 255};
    int exitWidth = MeasureText(exitText, 40);
    if (game->menuSelection == 1) {
        DrawRectangle(SCREEN_WIDTH / 2 - exitWidth / 2 - 20, startY + spacing - 10, exitWidth + 40, 50, Color{80, 0, 0, 150});
        DrawRectangleLines(SCREEN_WIDTH / 2 - exitWidth / 2 - 20, startY + spacing - 10, exitWidth + 40, 50, Color{200, 0, 0, 255});
        DrawText(">", SCREEN_WIDTH / 2 - exitWidth / 2 - 50, startY + spacing, 40, Color{255, 200, 0, 255});
    }
    DrawText(exitText, SCREEN_WIDTH / 2 - exitWidth / 2, startY + spacing, 40, exitColor);
    
    // Controls
    const char* controls = "Use W/S or Arrow Keys to navigate | ENTER or SPACE to select";
    int controlsWidth = MeasureText(controls, 18);
    DrawText(controls, SCREEN_WIDTH / 2 - controlsWidth / 2, SCREEN_HEIGHT - 100, 18, Color{100, 50, 50, 255});
    
    // Game description
    const char* desc = "Collect gold, avoid the killer, escape the maze!";
    int descWidth = MeasureText(desc, 22);
    DrawText(desc, SCREEN_WIDTH / 2 - descWidth / 2, SCREEN_HEIGHT - 60, 22, Color{150, 100, 100, 255});
    
    EndDrawing();
}

void DrawGame(const GameState* game) {
    if (game->mode == MAIN_MENU) {
        DrawMainMenu(game);
        return;
    }
    
    if (game->mode == LOADING) {
        DrawLoadingScreen(game->loadingTimer / 1.5f);
        return;
    }
    
    if (game->mode == SHOP) {
        DrawShop(game);
        return;
    }
    
    if (game->mode == GAME_WON) {
        DrawGameWon(game);
        return;
    }
    
    if (game->mode == GAME_LOST) {
        DrawGameLost(game);
        return;
    }
    
    // PLAYING mode
    BeginDrawing();
    ClearBackground(BLACK);
    
    // Dark ceiling/floor
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT/2, Color{30, 20, 30, 255});
    DrawRectangle(0, SCREEN_HEIGHT/2, SCREEN_WIDTH, SCREEN_HEIGHT/2, Color{40, 30, 30, 255});
    
    // Depth buffer for sprite occlusion
    float depthBuffer[SCREEN_WIDTH];
    
    Vector2 dirVec = { cosf(game->player.angle), sinf(game->player.angle) };
    
    // Raycasting
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        float cameraX = 2.0f * x / (float)SCREEN_WIDTH - 1.0f;
        float rayAngle = game->player.angle + atanf(cameraX * tanf(game->FOV / 2.0f));
        
        Vector2 rayDir = { cosf(rayAngle), sinf(rayAngle) };
        
        int mapX = (int)game->player.position.x;
        int mapY = (int)game->player.position.y;
        
        float deltaDistX = (rayDir.x == 0) ? 1e30f : fabsf(1.0f / rayDir.x);
        float deltaDistY = (rayDir.y == 0) ? 1e30f : fabsf(1.0f / rayDir.y);
        
        int stepX = (rayDir.x < 0) ? -1 : 1;
        int stepY = (rayDir.y < 0) ? -1 : 1;
        
        float sideDistX = (rayDir.x < 0) 
            ? (game->player.position.x - mapX) * deltaDistX 
            : (mapX + 1.0f - game->player.position.x) * deltaDistX;
        float sideDistY = (rayDir.y < 0) 
            ? (game->player.position.y - mapY) * deltaDistY 
            : (mapY + 1.0f - game->player.position.y) * deltaDistY;
        
        bool hit = false;
        bool side = false;
        
        while (!hit) {
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = false;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = true;
            }
            
            int tile = GetMapTile(mapX, mapY);
            if (tile > 0) hit = true;
        }
        
        float perpWallDist = !side 
            ? (mapX - game->player.position.x + (float)(1 - stepX) / 2) / rayDir.x 
            : (mapY - game->player.position.y + (float)(1 - stepY) / 2) / rayDir.y;
        if (perpWallDist < 0.1f) perpWallDist = 0.1f;
        
        int lineHeight = (int)(SCREEN_HEIGHT / perpWallDist);
        
        int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
        if (drawStart < 0) drawStart = 0;
        int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
        if (drawEnd >= SCREEN_HEIGHT) drawEnd = SCREEN_HEIGHT - 1;
        
        Color baseColor = side ? Color{80, 50, 50, 255} : Color{120, 70, 70, 255};
        
        float fogDistance = 10.0f;
        float fogFactor = perpWallDist / fogDistance;
        if (fogFactor > 1.0f) fogFactor = 1.0f;
        
        Color wallColor;
        int tile = GetMapTile(mapX, mapY);
        bool isDoor = tile == 2;
        bool canAffordDoor = game->totalGold >= game->doorCost;
        
        if (tile == 1) {
            wallColor = {
                (unsigned char)(baseColor.r * (1.0f - fogFactor)),
                (unsigned char)(baseColor.g * (1.0f - fogFactor)),
                (unsigned char)(baseColor.b * (1.0f - fogFactor)),
                255
            };
        } else if (isDoor && !canAffordDoor) {
            baseColor = Color{255, 0, 0, 255};
            wallColor = {
                (unsigned char)(baseColor.r * (1.0f - fogFactor)),
                (unsigned char)(baseColor.g * (1.0f - fogFactor)),
                (unsigned char)(baseColor.b * (1.0f - fogFactor)),
                255
            };
        }

        depthBuffer[x] = perpWallDist;
        
        if (!(isDoor && canAffordDoor)) {
            DrawLine(x, drawStart, x, drawEnd, wallColor);
        }
        
        if (isDoor && !canAffordDoor && x == SCREEN_WIDTH / 2) {
            Color textColor = canAffordDoor ? GREEN : RED;
            int textY = drawStart - 20;
            if (textY < 50) textY = 50;
            const char* doorText = TextFormat("$%d", game->doorCost);
            int textWidth = MeasureText(doorText, 14);
            DrawText(doorText, SCREEN_WIDTH / 2 - textWidth / 2, textY, 14, textColor);
        }
    }

    // Draw enemy
    DrawEnemy(&game->enemy, game->player.position, dirVec, depthBuffer, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // Draw collectibles
    DrawCollectibles(game->collectibles, game->player.position, dirVec, game->animTime, 
                    depthBuffer, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // Minimap
    const int miniMapScale = 6;
    const int miniMapOffsetX = 10;
    const int miniMapOffsetY = 10;
    
    for (int y = 0; y < currentMapHeight; y++) {
        for (int x = 0; x < currentMapWidth; x++) {
            int tile = GetMapTile(x, y);
            Color color = tile == 1 ? WHITE : tile == 2 ? RED : BLACK;
            DrawRectangle(
                miniMapOffsetX + x * miniMapScale, 
                miniMapOffsetY + y * miniMapScale,
                miniMapScale, miniMapScale, color
            );
        }
    }
    
    DrawCollectiblesMinimap(game->collectibles, miniMapOffsetX, miniMapOffsetY, miniMapScale);
    
    // Enemy on minimap (only if radar purchased)
    if (game->showEnemyOnMinimap && game->enemy.isActive) {
        DrawCircle(
            miniMapOffsetX + (int)(game->enemy.position.x * miniMapScale),
            miniMapOffsetY + (int)(game->enemy.position.y * miniMapScale),
            3, Color{150, 0, 0, 255}
        );
    }
    
    // Player on minimap
    DrawCircle(
        miniMapOffsetX + (int)(game->player.position.x * miniMapScale),
        miniMapOffsetY + (int)(game->player.position.y * miniMapScale),
        2, GREEN
    );
    
    DrawLine(
        miniMapOffsetX + (int)(game->player.position.x * miniMapScale),
        miniMapOffsetY + (int)(game->player.position.y * miniMapScale),
        miniMapOffsetX + (int)((game->player.position.x + dirVec.x * 0.5f) * miniMapScale),
        miniMapOffsetY + (int)((game->player.position.y + dirVec.y * 0.5f) * miniMapScale),
        GREEN
    );
    
    // Vignette effect
    for (int i = 0; i < 60; i++) {
        unsigned char alpha = (unsigned char)(i * 2);
        DrawRectangleLinesEx(
            Rectangle{(float)i, (float)i, (float)SCREEN_WIDTH - i*2, (float)SCREEN_HEIGHT - i*2}, 
            1, Color{0, 0, 0, alpha}
        );
    }

    // Draw stab effect if being attacked
    if (game->isBeingAttacked && game->stabEffectTimer > 0) {
        float intensity = game->stabEffectTimer / 2.0f;
        DrawStabEffect(intensity);
    }

    // TOP UI: Goal
    const char* goalText = TextFormat("GOAL: Collect $%d to unlock door", game->doorCost);
    int goalWidth = MeasureText(goalText, 24);
    DrawRectangle(SCREEN_WIDTH / 2 - goalWidth / 2 - 15, 10, goalWidth + 30, 40, Color{0, 0, 0, 180});
    Color goalColor = game->totalGold >= game->doorCost ? GREEN : ORANGE;
    DrawText(goalText, SCREEN_WIDTH / 2 - goalWidth / 2, 18, 24, goalColor);
    
    // RIGHT UI: Gold
    DrawRectangle(SCREEN_WIDTH - 150, 60, 140, 50, Color{0, 0, 0, 180});
    DrawRectangleLines(SCREEN_WIDTH - 150, 60, 140, 50, GOLD);
    DrawText(TextFormat("Gold: $%d", game->totalGold), SCREEN_WIDTH - 140, 75, 25, GOLD);
    
    // Speed boost indicator
    if (game->player.hasSpeedBoost) {
        DrawRectangle(SCREEN_WIDTH - 150, 120, 140, 40, Color{0, 0, 255, 180});
        DrawRectangleLines(SCREEN_WIDTH - 150, 120, 140, 40, BLUE);
        DrawText(TextFormat("BOOST: %.1fs", game->player.boostTimer), SCREEN_WIDTH - 140, 130, 20, BLUE);
    }
    
    DrawFPS(10, SCREEN_HEIGHT - 30);
    DrawText(TextFormat("Level %d/%d", game->currentLevel, MAX_LEVELS), 10, SCREEN_HEIGHT - 50, 20, WHITE);
    
    EndDrawing();
}
