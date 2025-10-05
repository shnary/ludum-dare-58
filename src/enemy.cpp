#include "enemy.h"
#include "map.h"
#include <cmath>
#include <raymath.h>
#include <algorithm>
#include <queue>

void InitEnemy(Enemy* enemy, Vector2 playerPos, int mapWidth, int mapHeight, int level) {
    enemy->speed = 2.0f;
    enemy->detectionRange = 15.0f;
    enemy->attackRange = 1.5f;
    enemy->isActive = (level != 1); // Disable enemy in level 1
    enemy->isChasing = false;
    enemy->pathRecalcTimer = 0.0f;
    enemy->currentPathIndex = 0;
    enemy->path.clear();
    
    Vector2 bestPos = {0, 0};
    
    // For level 2, force spawn at bottom-right corner
    if (level == 2) {
        bestPos = {(float)mapWidth - 2.0f, (float)mapHeight - 2.0f};
        // Check if walkable, otherwise try nearby positions
        if (GetMapTile((int)bestPos.x, (int)bestPos.y) != 0) {
            bestPos = {(float)mapWidth - 3.0f, (float)mapHeight - 3.0f};
        }
    } else {
        // Spawn at far corner from player for other levels
        float maxDist = 0;
        
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
    }
    
    enemy->position = bestPos;
}

// A* pathfinding node
struct AStarNode {
    int x, y;
    float g, h, f;
    int parentX, parentY;
    
    bool operator>(const AStarNode& other) const {
        return f > other.f;
    }
};

// Calculate heuristic (Manhattan distance)
float Heuristic(int x1, int y1, int x2, int y2) {
    return fabsf((float)(x1 - x2)) + fabsf((float)(y1 - y2));
}

// A* pathfinding algorithm
std::vector<Vector2> FindPathAStar(Vector2 start, Vector2 goal) {
    std::vector<Vector2> path;
    
    int startX = (int)start.x;
    int startY = (int)start.y;
    int goalX = (int)goal.x;
    int goalY = (int)goal.y;
    
    // Check if start or goal is invalid
    if (GetMapTile(startX, startY) != 0 || GetMapTile(goalX, goalY) != 0) {
        return path;
    }
    
    // Priority queue for open set
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> openSet;
    
    // Closed set
    std::vector<std::vector<bool>> closedSet(currentMapHeight, std::vector<bool>(currentMapWidth, false));
    std::vector<std::vector<AStarNode>> nodeMap(currentMapHeight, std::vector<AStarNode>(currentMapWidth));
    
    // Initialize start node
    AStarNode startNode;
    startNode.x = startX;
    startNode.y = startY;
    startNode.g = 0;
    startNode.h = Heuristic(startX, startY, goalX, goalY);
    startNode.f = startNode.g + startNode.h;
    startNode.parentX = -1;
    startNode.parentY = -1;
    
    openSet.push(startNode);
    nodeMap[startY][startX] = startNode;
    
    // Directions: 4-way movement
    int dx[] = {0, 1, 0, -1};
    int dy[] = {-1, 0, 1, 0};
    
    while (!openSet.empty()) {
        AStarNode current = openSet.top();
        openSet.pop();
        
        // Skip if already processed
        if (closedSet[current.y][current.x]) continue;
        
        closedSet[current.y][current.x] = true;
        
        // Goal reached
        if (current.x == goalX && current.y == goalY) {
            // Reconstruct path
            int x = goalX, y = goalY;
            while (!(x == startX && y == startY)) {
                path.push_back({(float)x + 0.5f, (float)y + 0.5f});
                AStarNode& node = nodeMap[y][x];
                int tmpX = node.parentX;
                int tmpY = node.parentY;
                x = tmpX;
                y = tmpY;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }
        
        // Explore neighbors
        for (int i = 0; i < 4; i++) {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];
            
            // Check bounds and walkability
            if (nx < 0 || nx >= currentMapWidth || ny < 0 || ny >= currentMapHeight) continue;
            if (GetMapTile(nx, ny) != 0) continue;
            if (closedSet[ny][nx]) continue;
            
            float newG = current.g + 1.0f;
            float h = Heuristic(nx, ny, goalX, goalY);
            float newF = newG + h;
            
            // Add to open set if not visited or found better path
            if (nodeMap[ny][nx].f == 0 || newG < nodeMap[ny][nx].g) {
                AStarNode neighbor;
                neighbor.x = nx;
                neighbor.y = ny;
                neighbor.g = newG;
                neighbor.h = h;
                neighbor.f = newF;
                neighbor.parentX = current.x;
                neighbor.parentY = current.y;
                
                nodeMap[ny][nx] = neighbor;
                openSet.push(neighbor);
            }
        }
    }
    
    // No path found
    return path;
}

void UpdateEnemy(Enemy* enemy, Vector2 playerPos, float deltaTime) {
    if (!enemy->isActive) return;
    
    // Always chase player
    enemy->isChasing = true;
    
    // Recalculate path periodically
    enemy->pathRecalcTimer -= deltaTime;
    
    if (enemy->pathRecalcTimer <= 0.0f || enemy->path.empty() || enemy->currentPathIndex >= (int)enemy->path.size()) {
        enemy->path = FindPathAStar(enemy->position, playerPos);
        enemy->currentPathIndex = 0;
        enemy->pathRecalcTimer = 0.5f; // Recalculate every 0.5 seconds
    }
    
    // Follow A* path
    if (!enemy->path.empty() && enemy->currentPathIndex < (int)enemy->path.size()) {
        Vector2 targetWaypoint = enemy->path[enemy->currentPathIndex];
        Vector2 direction = Vector2Subtract(targetWaypoint, enemy->position);
        float distToWaypoint = Vector2Length(direction);
        
        // If close to waypoint, move to next
        if (distToWaypoint < 0.3f) {
            enemy->currentPathIndex++;
            if (enemy->currentPathIndex < (int)enemy->path.size()) {
                targetWaypoint = enemy->path[enemy->currentPathIndex];
                direction = Vector2Subtract(targetWaypoint, enemy->position);
                distToWaypoint = Vector2Length(direction);
            }
        }
        
        // Move towards waypoint
        if (distToWaypoint > 0.1f) {
            direction = Vector2Normalize(direction);
            float moveAmount = enemy->speed * deltaTime;
            if (moveAmount > distToWaypoint) moveAmount = distToWaypoint;
            
            enemy->position = Vector2Add(enemy->position, Vector2Scale(direction, moveAmount));
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
    
    // Sprite size (enemy is 1.8x larger to be more intimidating)
    float enemyScale = 1.8f;
    int spriteHeight = abs((int)(screenHeight / transformY * enemyScale));
    int spriteWidth = abs((int)(screenHeight / transformY * 0.5f * enemyScale));
    
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

