#ifndef GAME_H
#define GAME_H

#include <raylib.h>
#include <vector>
#include "collectible.h"
#include "enemy.h"

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int MAX_LEVELS = 5;

enum GameMode {
    MAIN_MENU,
    PLAYING,
    LOADING,
    SHOP,
    GAME_WON,
    GAME_LOST
};

struct Perk {
    const char* name;
    const char* description;
    int cost;
    int type; // 0=gold multiplier, 1=speed increase, 2=boost duration, 3=enemy radar
    float value;
};

struct Player {
    Vector2 position;
    float angle;
    float moveSpeed;
    float mouseSensitivity;
    bool hasSpeedBoost;
    float boostTimer;
    float goldMultiplier;
    float baseSpeed;
};

struct GameState {
    Player player;
    std::vector<Collectible> collectibles;
    Enemy enemy;
    int totalGold;
    float animTime;
    float FOV;
    int doorCost;
    int currentLevel;
    GameMode mode;
    float loadingTimer;
    Perk shopPerks[3];
    int selectedPerk;
    float stabEffectTimer;
    bool isBeingAttacked;
    bool shopContinuePressed;
    int menuSelection;
    bool showEnemyOnMinimap;
    Sound collectSound;
    Sound stabSound;
    Sound purchaseSound;
    Sound jumpscareSound;
    Music horrorMusic;
};

// Initialize game state
void InitGame(GameState* game);

// Initialize level
void InitLevel(GameState* game, int level);

// Generate shop perks
void GenerateShopPerks(GameState* game);

// Update game logic
void UpdateGame(GameState* game, float deltaTime);

// Render game
void DrawGame(const GameState* game);

// Draw loading screen
void DrawLoadingScreen(float progress);

// Draw shop UI
void DrawShop(const GameState* game);

// Draw game won screen
void DrawGameWon(const GameState* game);

// Draw game lost screen
void DrawGameLost(const GameState* game);

// Draw knife stab effect
void DrawStabEffect(float intensity);

// Draw main menu
void DrawMainMenu(const GameState* game);

#endif

