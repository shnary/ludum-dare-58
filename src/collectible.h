#ifndef COLLECTIBLE_H
#define COLLECTIBLE_H

#include <raylib.h>
#include <vector>

#define COIN 0
#define BOOST 1

struct Collectible {
    Vector2 pos;
    bool collected;
    float value;
    int type;
};

// Initialize collectibles in the world
std::vector<Collectible> InitCollectibles();

// Check and handle player pickup
void UpdateCollectibles(std::vector<Collectible>& collectibles, Vector2 playerPos, int& totalGold, 
                       bool& hasSpeedBoost, float& boostTimer, float goldMultiplier);

// Draw collectibles as 3D sprites with depth
void DrawCollectibles(const std::vector<Collectible>& collectibles, Vector2 playerPos, Vector2 dirVec, 
                     float animTime, const float* depthBuffer, int screenWidth, int screenHeight);

// Draw collectibles on minimap
void DrawCollectiblesMinimap(const std::vector<Collectible>& collectibles, int miniMapOffsetX, 
                            int miniMapOffsetY, int miniMapScale);

#endif

