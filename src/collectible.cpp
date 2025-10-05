#include "collectible.h"
#include "map.h"
#include <raymath.h>
#include <cmath>
#include <stdlib.h>

std::vector<Collectible> InitCollectibles() {
    std::vector<Collectible> collectibles;
    
    // Generate random collectible positions, ensuring they're not in walls
    int numCoins = 15 + (rand() % 10); // 15-25 coins
    int attempts = 0;
    int maxAttempts = numCoins * 10;
    
    while (collectibles.size() < (size_t)numCoins && attempts < maxAttempts) {
        attempts++;
        
        float x = 2.0f + (float)(rand() % (currentMapWidth - 4));
        float y = 2.0f + (float)(rand() % (currentMapHeight - 4));
        
        // Check if position is walkable and not a door
        int mapX = (int)x;
        int mapY = (int)y;
        
        if (GetMapTile(mapX, mapY) != 0) continue;
        
        // Check if too close to other collectibles
        bool tooClose = false;
        for (const auto& c : collectibles) {
            float dx = c.pos.x - x;
            float dy = c.pos.y - y;
            if (dx*dx + dy*dy < 4.0f) { // At least 2 units apart
                tooClose = true;
                break;
            }
        }
        
        if (tooClose) continue;
        
        collectibles.push_back({{x, y}, false, 10, COIN});
    }
    
    // Add 1-2 speed boosts in safe locations
    int numBoosts = 1 + (rand() % 2);
    attempts = 0;
    int boostsAdded = 0;
    
    while (boostsAdded < numBoosts && attempts < 100) {
        attempts++;
        
        float x = 3.0f + (float)(rand() % (currentMapWidth - 6));
        float y = 3.0f + (float)(rand() % (currentMapHeight - 6));
        
        int mapX = (int)x;
        int mapY = (int)y;
        
        if (GetMapTile(mapX, mapY) != 0) continue;
        
        // Check if too close to other collectibles
        bool tooClose = false;
        for (const auto& c : collectibles) {
            float dx = c.pos.x - x;
            float dy = c.pos.y - y;
            if (dx*dx + dy*dy < 9.0f) { // At least 3 units apart
                tooClose = true;
                break;
            }
        }
        
        if (tooClose) continue;
        
        collectibles.push_back({{x, y}, false, 0, BOOST});
        boostsAdded++;
    }
    
    return collectibles;
}

void UpdateCollectibles(std::vector<Collectible>& collectibles, Vector2 playerPos, int& totalGold, 
                       bool& hasSpeedBoost, float& boostTimer, float goldMultiplier) {
    for (auto& collectible : collectibles) {
        if (!collectible.collected) {
            float dx = playerPos.x - collectible.pos.x;
            float dy = playerPos.y - collectible.pos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < 0.7f) {
                collectible.collected = true;
                if (collectible.type == COIN) {
                    totalGold += (int)(collectible.value * goldMultiplier);
                } else if (collectible.type == BOOST) {
                    hasSpeedBoost = true;
                    boostTimer = 10.0f;  // 10 second boost
                }
            }
        }
    }
}

void DrawCollectibles(const std::vector<Collectible>& collectibles, Vector2 playerPos, Vector2 dirVec,
                     float animTime, const float* depthBuffer, int screenWidth, int screenHeight) {
    for (const auto& collectible : collectibles) {
        if (collectible.collected) continue;
        
        // Calculate sprite position relative to player
        float spriteX = collectible.pos.x - playerPos.x;
        float spriteY = collectible.pos.y - playerPos.y;
        
        // Calculate distance to sprite
        float spriteDistance = sqrtf(spriteX * spriteX + spriteY * spriteY);
        
        // Calculate plane perpendicular to direction
        float planeX = -dirVec.y;
        float planeY = dirVec.x;
        
        // Transform sprite with inverse camera matrix
        float invDet = 1.0f / (planeX * dirVec.y - dirVec.x * planeY);
        float transformX = invDet * (dirVec.y * spriteX - dirVec.x * spriteY);
        float transformY = invDet * (-planeY * spriteX + planeX * spriteY);
        
        // Don't render if behind player or too close
        if (transformY <= 0.2f) continue;
        
        // Screen X position
        int spriteScreenX = (int)(((float)screenWidth / 2) * (1.0f + transformX / transformY));
        
        // Sprite size based on distance
        int spriteHeight = abs((int)(screenHeight / transformY * 0.5f));
        int spriteWidth = abs((int)(screenHeight / transformY * 0.5f));
        
        // Clamp size
        /*
        if (spriteHeight < 8) spriteHeight = 8;
        if (spriteHeight > 500) spriteHeight = 500;
        if (spriteWidth < 8) spriteWidth = 8;
        if (spriteWidth > 500) spriteWidth = 500;
        */
        
        int drawStartY = -spriteHeight / 2 + screenHeight / 2;
        int drawEndY = spriteHeight / 2 + screenHeight / 2;
        int drawStartX = -spriteWidth / 2 + spriteScreenX;
        int drawEndX = spriteWidth / 2 + spriteScreenX;
        
        // Animation - bobbing effect
        float bobOffset = sinf(animTime * 3.0f) * 10.0f;
        drawStartY += (int)bobOffset;
        drawEndY += (int)bobOffset;
        
        // Don't render if off screen
        if (drawEndX < 0 || drawStartX >= screenWidth) continue;
        
        // Check depth buffer for occlusion - check multiple points across sprite
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
        
        // Only draw if at least 40% of check points are visible
        if (visiblePoints < checkPoints * 0.4f) continue;
        
        // Draw collectible as yellow circle with glow
        int centerX = (drawStartX + drawEndX) / 2;
        int centerY = (drawStartY + drawEndY) / 2;
        int radius = (drawEndX - drawStartX) / 3;
        
        // Ensure minimum size
        if (radius < 5) radius = 5;
        // if (radius > 30) radius = 30;
        
        // Glow effect
        DrawCircle(centerX, centerY, radius * 2.0f, Color{255, 215, 0, 40});
        DrawCircle(centerX, centerY, radius * 1.5f, Color{255, 215, 0, 80});
        
        // Main collectible
        if (collectible.type == COIN) {
            DrawCircle(centerX, centerY, radius, GOLD);
        } else if (collectible.type == BOOST) {
            DrawCircle(centerX, centerY, radius, BLUE);
        }
        
        // Highlight
        DrawCircle(centerX - radius/3, centerY - radius/3, (float)radius/3, YELLOW);
        
        // Draw $ text above collectible
        int textSize = radius;
        if (textSize < 12) textSize = 12;
        // if (textSize > 25) textSize = 25;
        
        const char* dollarText = collectible.type == COIN ? "$" : "2x Speed";
        int textWidth = MeasureText(dollarText, textSize);
        if (collectible.type == COIN) {
            DrawText(dollarText, centerX - textWidth/2, centerY - radius - textSize - 8, textSize, YELLOW);
        } else if (collectible.type == BOOST) {
            DrawText(dollarText, centerX - textWidth/2, centerY - radius - textSize - 8, textSize, BLUE);
        }
    }
}

void DrawCollectiblesMinimap(const std::vector<Collectible>& collectibles, int miniMapOffsetX,
                            int miniMapOffsetY, int miniMapScale) {
    for (const auto& collectible : collectibles) {
        if (!collectible.collected) {
            DrawCircle(
                miniMapOffsetX + (int)(collectible.pos.x * miniMapScale),
                miniMapOffsetY + (int)(collectible.pos.y * miniMapScale),
                3, collectible.type == COIN ? GOLD : BLUE
            );
        }
    }
}

