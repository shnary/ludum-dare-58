#include "enemy.h"
#include "map.h"
#include <cmath>
#include <raymath.h>

void InitEnemy(Enemy* enemy, Vector2 playerPos, int mapWidth, int mapHeight) {
    enemy->speed = 2.0f;
    enemy->detectionRange = 15.0f;
    enemy->attackRange = 1.5f;
    enemy->isActive = true;
    enemy->isChasing = false;
    enemy->pathRecalcTimer = 0.0f;
    
    // Spawn at far corner from player
    float maxDist = 0;
    Vector2 bestPos = {0, 0};
    
    // Try corners and edges
    Vector2 spawnPoints[] = {
        {(float)mapWidth - 2.0f, (float)mapHeight - 2.0f},
        {2.0f, (float)mapHeight - 2.0f},
        {(float)mapWidth - 2.0f, 2.0f},
        {2.0f, 2.0f},
        {(float)mapWidth / 2.0f, 2.0f},
        {(float)mapWidth / 2.0f, (float)mapHeight - 2.0f}
    };
    
    for (int i = 0; i < 6; i++) {
        Vector2 pos = spawnPoints[i];
        float dist = Vector2Distance(pos, playerPos);
        if (dist > maxDist && GetMapTile((int)pos.x, (int)pos.y) == 0) {
            maxDist = dist;
            bestPos = pos;
        }
    }
    
    enemy->position = bestPos;
    enemy->pathTarget = playerPos;
}

// Simple pathfinding: find next step towards player avoiding walls
Vector2 FindNextPathStep(Vector2 from, Vector2 to) {
    // Direct direction to player
    Vector2 direct = Vector2Subtract(to, from);
    float dist = Vector2Length(direct);
    
    if (dist < 0.1f) return from;
    
    Vector2 directNorm = Vector2Normalize(direct);
    
    // Check if direct path is clear
    Vector2 testPos = Vector2Add(from, Vector2Scale(directNorm, 0.5f));
    if (GetMapTile((int)testPos.x, (int)testPos.y) == 0) {
        return testPos;
    }
    
    // Try perpendicular directions if blocked
    Vector2 perpRight = {-directNorm.y, directNorm.x};
    Vector2 perpLeft = {directNorm.y, -directNorm.x};
    
    // Test right perpendicular
    Vector2 testRight = Vector2Add(from, Vector2Scale(perpRight, 0.5f));
    if (GetMapTile((int)testRight.x, (int)testRight.y) == 0) {
        // Check if this gets us closer
        float distRight = Vector2Distance(testRight, to);
        if (distRight < dist + 2.0f) { // Allow some detour
            return testRight;
        }
    }
    
    // Test left perpendicular
    Vector2 testLeft = Vector2Add(from, Vector2Scale(perpLeft, 0.5f));
    if (GetMapTile((int)testLeft.x, (int)testLeft.y) == 0) {
        float distLeft = Vector2Distance(testLeft, to);
        if (distLeft < dist + 2.0f) {
            return testLeft;
        }
    }
    
    // Try diagonal directions
    Vector2 diagonals[] = {
        {directNorm.x + perpRight.x, directNorm.y + perpRight.y},
        {directNorm.x + perpLeft.x, directNorm.y + perpLeft.y},
        {-directNorm.x + perpRight.x, -directNorm.y + perpRight.y},
        {-directNorm.x + perpLeft.x, -directNorm.y + perpLeft.y}
    };
    
    for (int i = 0; i < 4; i++) {
        Vector2 diagNorm = Vector2Normalize(diagonals[i]);
        Vector2 testDiag = Vector2Add(from, Vector2Scale(diagNorm, 0.5f));
        if (GetMapTile((int)testDiag.x, (int)testDiag.y) == 0) {
            return testDiag;
        }
    }
    
    return from; // Stuck
}

void UpdateEnemy(Enemy* enemy, Vector2 playerPos, float deltaTime) {
    if (!enemy->isActive) return;
    
    float distToPlayer = Vector2Distance(enemy->position, playerPos);
    
    // Always chase player
    enemy->isChasing = true;
    
    // Recalculate path periodically or if far from target
    enemy->pathRecalcTimer -= deltaTime;
    float distToTarget = Vector2Distance(enemy->position, enemy->pathTarget);
    
    if (enemy->pathRecalcTimer <= 0.0f || distToTarget < 0.5f) {
        enemy->pathTarget = FindNextPathStep(enemy->position, playerPos);
        enemy->pathRecalcTimer = 0.3f; // Recalculate every 0.3 seconds
    }
    
    if (enemy->isChasing) {
        Vector2 direction = Vector2Subtract(enemy->pathTarget, enemy->position);
        float targetDist = Vector2Length(direction);
        
        if (targetDist > 0.1f) {
            direction = Vector2Normalize(direction);
            
            // Move towards path target
            float moveAmount = enemy->speed * deltaTime;
            if (moveAmount > targetDist) moveAmount = targetDist;
            
            Vector2 newPos = Vector2Add(enemy->position, Vector2Scale(direction, moveAmount));
            
            // Collision check
            int mapX = (int)newPos.x;
            int mapY = (int)newPos.y;
            
            if (GetMapTile(mapX, mapY) == 0) {
                enemy->position = newPos;
            } else {
                // Try sliding along walls
                Vector2 tryX = {newPos.x, enemy->position.y};
                if (GetMapTile((int)tryX.x, (int)tryX.y) == 0) {
                    enemy->position = tryX;
                } else {
                    Vector2 tryY = {enemy->position.x, newPos.y};
                    if (GetMapTile((int)tryY.x, (int)tryY.y) == 0) {
                        enemy->position = tryY;
                    } else {
                        // Stuck, force recalc
                        enemy->pathRecalcTimer = 0.0f;
                    }
                }
            }
        }
    }
}

void DrawEnemy(const Enemy* enemy, Vector2 playerPos, Vector2 dirVec,
              const float* depthBuffer, int screenWidth, int screenHeight) {
    if (!enemy->isActive) return;
    
    // Calculate sprite position relative to player
    float spriteX = enemy->position.x - playerPos.x;
    float spriteY = enemy->position.y - playerPos.y;
    
    // Calculate plane perpendicular to direction
    float planeX = -dirVec.y;
    float planeY = dirVec.x;
    
    // Transform sprite with inverse camera matrix
    float invDet = 1.0f / (planeX * dirVec.y - dirVec.x * planeY);
    float transformX = invDet * (dirVec.y * spriteX - dirVec.x * spriteY);
    float transformY = invDet * (-planeY * spriteX + planeX * spriteY);
    
    // Don't render if behind player
    if (transformY <= 0.2f) return;
    
    // Screen X position
    int spriteScreenX = (int)(((float)screenWidth / 2) * (1.0f + transformX / transformY));
    
    // Sprite size
    int spriteHeight = abs((int)(screenHeight / transformY));
    int spriteWidth = abs((int)(screenHeight / transformY * 0.5f));
    
    int drawStartY = -spriteHeight / 2 + screenHeight / 2;
    int drawEndY = spriteHeight / 2 + screenHeight / 2;
    int drawStartX = -spriteWidth / 2 + spriteScreenX;
    int drawEndX = spriteWidth / 2 + spriteScreenX;
    
    // Don't render if off screen
    if (drawEndX < 0 || drawStartX >= screenWidth) return;
    
    // Check depth buffer
    int checkPoints = 5;
    int visiblePoints = 0;
    
    for (int i = 0; i < checkPoints; i++) {
        int checkX = drawStartX + (drawEndX - drawStartX) * i / (checkPoints - 1);
        if (checkX >= 0 && checkX < screenWidth) {
            if (transformY < depthBuffer[checkX]) {
                visiblePoints++;
            }
        }
    }
    
    if (visiblePoints < checkPoints * 0.4f) return;
    
    // Draw enemy as dark red humanoid figure
    int centerX = (drawStartX + drawEndX) / 2;
    int bodyHeight = spriteHeight;
    int bodyWidth = spriteWidth;
    
    // Body (dark red rectangle)
    DrawRectangle(centerX - bodyWidth / 4, drawStartY + bodyHeight / 4, 
                 bodyWidth / 2, bodyHeight / 2, Color{100, 0, 0, 255});
    
    // Head (dark circle)
    int headRadius = bodyWidth / 4;
    DrawCircle(centerX, drawStartY + bodyHeight / 6, headRadius, Color{80, 0, 0, 255});
    
    // Eyes (glowing red)
    DrawCircle(centerX - headRadius / 3, drawStartY + bodyHeight / 6, 2, RED);
    DrawCircle(centerX + headRadius / 3, drawStartY + bodyHeight / 6, 2, RED);
    
    // Weapon hint (knife)
    DrawLine(centerX + bodyWidth / 3, drawStartY + bodyHeight / 2,
            centerX + bodyWidth / 2, drawStartY + bodyHeight / 3, LIGHTGRAY);
}

bool IsPlayerCaught(const Enemy* enemy, Vector2 playerPos) {
    if (!enemy->isActive) return false;
    float dist = Vector2Distance(enemy->position, playerPos);
    
    if (dist >= enemy->attackRange) return false;
    
    // Check line of sight - enemy can't attack through walls
    Vector2 direction = Vector2Subtract(playerPos, enemy->position);
    float length = Vector2Length(direction);
    if (length < 0.1f) return true;
    
    direction = Vector2Normalize(direction);
    
    // Cast ray from enemy to player
    float step = 0.1f;
    for (float t = 0; t < length; t += step) {
        Vector2 checkPos = Vector2Add(enemy->position, Vector2Scale(direction, t));
        int mapX = (int)checkPos.x;
        int mapY = (int)checkPos.y;
        
        if (GetMapTile(mapX, mapY) > 0) {
            return false; // Wall blocking
        }
    }
    
    return true; // Clear line of sight and in range
}

