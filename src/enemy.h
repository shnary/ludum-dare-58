#ifndef ENEMY_H
#define ENEMY_H

#include <raylib.h>

struct Enemy {
    Vector2 position;
    float speed;
    float detectionRange;
    float attackRange;
    bool isActive;
    bool isChasing;
    Vector2 pathTarget; // Current pathfinding target
    float pathRecalcTimer; // Timer to recalculate path
};

// Initialize enemy at far position from player
void InitEnemy(Enemy* enemy, Vector2 playerPos, int mapWidth, int mapHeight);

// Update enemy AI
void UpdateEnemy(Enemy* enemy, Vector2 playerPos, float deltaTime);

// Draw enemy as 3D sprite
void DrawEnemy(const Enemy* enemy, Vector2 playerPos, Vector2 dirVec, 
              const float* depthBuffer, int screenWidth, int screenHeight);

// Check if enemy caught player
bool IsPlayerCaught(const Enemy* enemy, Vector2 playerPos);

#endif

