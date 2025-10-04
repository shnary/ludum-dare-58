#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <vector>
#include "map.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

struct Collectible {
    Vector2 pos;
    bool collected;
    float value;
};

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "LD58 Maze Killer");
    SetTargetFPS(60);
    DisableCursor();
    
    Vector2 playerPos = { 8.0f, 8.0f };
    float playerAngle = 0.0f;
    const float moveSpeed = 3.0f;
    const float mouseSensitivity = 0.003f;
    const float FOV = 60.0f * DEG2RAD;
    
    // Collectibles (placed in open walkable areas)
    std::vector<Collectible> collectibles = {
        {{4.5f, 2.5f}, false, 10},
        {{7.5f, 2.5f}, false, 10},
        {{12.5f, 2.5f}, false, 10},
        {{13.5f, 6.5f}, false, 10},
        {{9.5f, 9.5f}, false, 10},
        {{5.5f, 13.5f}, false, 10},
        {{2.5f, 6.5f}, false, 10},
        {{4.5f, 10.5f}, false, 10},
    };
    
    int totalGold = 0;
    float animTime = 0.0f;
    
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        animTime += dt;
        
        // Mouse look
        Vector2 mouseDelta = GetMouseDelta();
        playerAngle += mouseDelta.x * mouseSensitivity;
        
        // Direction vectors
        Vector2 dirVec = { cosf(playerAngle), sinf(playerAngle) };
        Vector2 rightVec = { cosf(playerAngle + PI/2), sinf(playerAngle + PI/2) };
        
        // WASD movement
        Vector2 moveDir = { 0, 0 };
        if (IsKeyDown(KEY_W)) { moveDir.x += dirVec.x; moveDir.y += dirVec.y; }
        if (IsKeyDown(KEY_S)) { moveDir.x -= dirVec.x; moveDir.y -= dirVec.y; }
        if (IsKeyDown(KEY_A)) { moveDir.x -= rightVec.x; moveDir.y -= rightVec.y; }
        if (IsKeyDown(KEY_D)) { moveDir.x += rightVec.x; moveDir.y += rightVec.y; }
        
        // Apply movement with collision (check X and Y separately for wall sliding)
        if (moveDir.x != 0 || moveDir.y != 0) {
            float len = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
            moveDir.x /= len;
            moveDir.y /= len;
            
            float moveX = moveDir.x * moveSpeed * dt;
            float moveY = moveDir.y * moveSpeed * dt;
            
            // Try X movement
            float newX = playerPos.x + moveX;
            int mapX = (int)newX;
            int mapY = (int)playerPos.y;
            if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT) {
                if (worldMap[mapY][mapX] == 0) {
                    playerPos.x = newX;
                }
            }
            
            // Try Y movement
            float newY = playerPos.y + moveY;
            mapX = (int)playerPos.x;
            mapY = (int)newY;
            if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT) {
                if (worldMap[mapY][mapX] == 0) {
                    playerPos.y = newY;
                }
            }
        }
        
            // Check collectible pickup (larger pickup radius)
        for (auto& coin : collectibles) {
            if (!coin.collected) {
                float dx = playerPos.x - coin.pos.x;
                float dy = playerPos.y - coin.pos.y;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist < 0.7f) {  // Increased pickup radius
                    coin.collected = true;
                    totalGold += (int)coin.value;
                }
            }
        }
        
        // Raycasting
        BeginDrawing();
        ClearBackground(BLACK);
        
        // Horror ambiance - dark ceiling/floor
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT/2, Color{30, 20, 30, 255});  // Dark ceiling
        DrawRectangle(0, SCREEN_HEIGHT/2, SCREEN_WIDTH, SCREEN_HEIGHT/2, Color{40, 30, 30, 255});  // Dark floor
        
        // Depth buffer for sprite occlusion
        float depthBuffer[SCREEN_WIDTH];
        
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            float cameraX = 2.0f * x / (float)SCREEN_WIDTH - 1.0f;
            float rayAngle = playerAngle + atanf(cameraX * tanf(FOV / 2.0f));
            
            Vector2 rayDir = { cosf(rayAngle), sinf(rayAngle) };
            
            int mapX = (int)playerPos.x;
            int mapY = (int)playerPos.y;
            
            float deltaDistX = (rayDir.x == 0) ? 1e30f : fabsf(1.0f / rayDir.x);
            float deltaDistY = (rayDir.y == 0) ? 1e30f : fabsf(1.0f / rayDir.y);
            
            int stepX = (rayDir.x < 0) ? -1 : 1;
            int stepY = (rayDir.y < 0) ? -1 : 1;
            
            float sideDistX = (rayDir.x < 0) 
                ? (playerPos.x - mapX) * deltaDistX 
                : (mapX + 1.0f - playerPos.x) * deltaDistX;
            float sideDistY = (rayDir.y < 0) 
                ? (playerPos.y - mapY) * deltaDistY 
                : (mapY + 1.0f - playerPos.y) * deltaDistY;
            
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
                
                if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) {
                    hit = true;
                } else if (worldMap[mapY][mapX] > 0) {
                    hit = true;
                }
            }
            
            // Calculate perpendicular distance (prevents fisheye)
            float perpWallDist;
            if (!side) {
                perpWallDist = (mapX - playerPos.x + (1 - stepX) / 2) / rayDir.x;
            } else {
                perpWallDist = (mapY - playerPos.y + (1 - stepY) / 2) / rayDir.y;
            }
            
            // Prevent division by zero
            if (perpWallDist < 0.1f) perpWallDist = 0.1f;
            
            int lineHeight = (int)(SCREEN_HEIGHT / perpWallDist);
            
            int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
            if (drawStart < 0) drawStart = 0;
            int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
            if (drawEnd >= SCREEN_HEIGHT) drawEnd = SCREEN_HEIGHT - 1;
            
            // Horror color scheme with fog
            Color baseColor;
            if (side) {
                baseColor = Color{80, 50, 50, 255};  // Dark red (horizontal walls)
            } else {
                baseColor = Color{120, 70, 70, 255};  // Lighter red (vertical walls)
            }
            
            // Apply distance fog (max fog distance = 10 units)
            float fogDistance = 10.0f;
            float fogFactor = perpWallDist / fogDistance;
            if (fogFactor > 1.0f) fogFactor = 1.0f;
            
            // Interpolate between wall color and fog color (black)
            Color fogColor = BLACK;
            Color wallColor = {
                (unsigned char)(baseColor.r * (1.0f - fogFactor) + fogColor.r * fogFactor),
                (unsigned char)(baseColor.g * (1.0f - fogFactor) + fogColor.g * fogFactor),
                (unsigned char)(baseColor.b * (1.0f - fogFactor) + fogColor.b * fogFactor),
                255
            };
            
            // Store depth for this column
            depthBuffer[x] = perpWallDist;
            
            DrawLine(x, drawStart, x, drawEnd, wallColor);
        }
        
        // Render collectibles as sprites
        for (const auto& coin : collectibles) {
            if (coin.collected) continue;
            
            // Calculate sprite position relative to player
            float spriteX = coin.pos.x - playerPos.x;
            float spriteY = coin.pos.y - playerPos.y;
            
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
            int spriteScreenX = (int)((SCREEN_WIDTH / 2) * (1.0f + transformX / transformY));
            
            // Sprite size based on distance - normal perspective (bigger when close, smaller when far)
            // Use transformY (perpendicular distance) for proper scaling
            int spriteHeight = abs((int)(SCREEN_HEIGHT / transformY * 0.5f));
            int spriteWidth = abs((int)(SCREEN_HEIGHT / transformY * 0.5f));
            
            // Clamp size - allow much bigger when very close
            if (spriteHeight < 8) spriteHeight = 8;
            if (spriteHeight > 300) spriteHeight = 300;
            if (spriteWidth < 8) spriteWidth = 8;
            if (spriteWidth > 300) spriteWidth = 300;
            
            int drawStartY = -spriteHeight / 2 + SCREEN_HEIGHT / 2;
            int drawEndY = spriteHeight / 2 + SCREEN_HEIGHT / 2;
            int drawStartX = -spriteWidth / 2 + spriteScreenX;
            int drawEndX = spriteWidth / 2 + spriteScreenX;
            
            // Animation - bobbing effect
            float bobOffset = sinf(animTime * 3.0f) * 10.0f;
            drawStartY += (int)bobOffset;
            drawEndY += (int)bobOffset;
            
            // Don't render if off screen
            if (drawEndX < 0 || drawStartX >= SCREEN_WIDTH) continue;
            
            // Check depth buffer for occlusion (don't draw behind walls)
            bool visible = false;
            for (int x = drawStartX; x <= drawEndX; x++) {
                if (x >= 0 && x < SCREEN_WIDTH) {
                    // If sprite is closer than wall at this column, it's visible
                    if (transformY < depthBuffer[x]) {
                        visible = true;
                        break;
                    }
                }
            }
            
            if (!visible) continue;
            
            // Draw coin as yellow circle with glow
            int centerX = (drawStartX + drawEndX) / 2;
            int centerY = (drawStartY + drawEndY) / 2;
            int radius = (drawEndX - drawStartX) / 3;
            
            // Ensure minimum size
            if (radius < 5) radius = 5;
            if (radius > 30) radius = 30;
            
            // Glow effect (large semi-transparent circle)
            DrawCircle(centerX, centerY, radius * 2.0f, Color{255, 215, 0, 40});
            DrawCircle(centerX, centerY, radius * 1.5f, Color{255, 215, 0, 80});
            
            // Main coin
            DrawCircle(centerX, centerY, radius, GOLD);
            
            // Highlight
            DrawCircle(centerX - radius/3, centerY - radius/3, radius/3, YELLOW);
            
            // Draw $ text above coin
            int textSize = radius;
            if (textSize < 12) textSize = 12;
            if (textSize > 25) textSize = 25;
            const char* dollarText = "$";
            int textWidth = MeasureText(dollarText, textSize);
            DrawText(dollarText, centerX - textWidth/2, centerY - radius - textSize - 8, textSize, YELLOW);
        }
        
        // Mini-map
        const int miniMapScale = 10;
        const int miniMapOffsetX = 10;
        const int miniMapOffsetY = 10;
        
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                Color color = worldMap[y][x] == 1 ? WHITE : BLACK;
                DrawRectangle(
                    miniMapOffsetX + x * miniMapScale, 
                    miniMapOffsetY + y * miniMapScale,
                    miniMapScale, miniMapScale, color
                );
            }
        }
        
        // Draw collectibles on minimap
        for (const auto& coin : collectibles) {
            if (!coin.collected) {
                DrawCircle(
                    miniMapOffsetX + (int)(coin.pos.x * miniMapScale),
                    miniMapOffsetY + (int)(coin.pos.y * miniMapScale),
                    3, GOLD
                );
            }
        }
        
        DrawCircle(
            miniMapOffsetX + (int)(playerPos.x * miniMapScale),
            miniMapOffsetY + (int)(playerPos.y * miniMapScale),
            2, RED
        );
        
        DrawLine(
            miniMapOffsetX + (int)(playerPos.x * miniMapScale),
            miniMapOffsetY + (int)(playerPos.y * miniMapScale),
            miniMapOffsetX + (int)((playerPos.x + dirVec.x * 0.5f) * miniMapScale),
            miniMapOffsetY + (int)((playerPos.y + dirVec.y * 0.5f) * miniMapScale),
            RED
        );
        
        // Vignette effect for horror atmosphere
        for (int i = 0; i < 60; i++) {
            unsigned char alpha = (unsigned char)(i * 2);
            DrawRectangleLinesEx(Rectangle{(float)i, (float)i, (float)SCREEN_WIDTH - i*2, (float)SCREEN_HEIGHT - i*2}, 1, Color{0, 0, 0, alpha});
        }
        
        // Draw gold UI
        DrawRectangle(SCREEN_WIDTH - 150, 10, 140, 50, Color{0, 0, 0, 180});
        DrawRectangleLines(SCREEN_WIDTH - 150, 10, 140, 50, GOLD);
        DrawText(TextFormat("Gold: $%d", totalGold), SCREEN_WIDTH - 140, 25, 25, GOLD);
        
        DrawFPS(10, SCREEN_HEIGHT - 30);
        DrawText("WASD: Move | Mouse: Look | ESC: Exit", 10, SCREEN_HEIGHT - 50, 20, Color{150, 150, 150, 255});
        
        EndDrawing();
    }
    
    CloseWindow();
}