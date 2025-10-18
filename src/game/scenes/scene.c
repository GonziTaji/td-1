#include "scene.h"
#include "../../core/asset_manager.h"
#include "../../input/input.h"
#include "../../utils/grid.h"
#include "../constants.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SCENE_SCALE_INITIAL 5.0f
#define SCENE_SCALE_STEP 0.4f
#define SCENE_SCALE_MIN SCENE_SCALE_INITIAL - (2 * SCENE_SCALE_STEP)
#define SCENE_SCALE_MAX SCENE_SCALE_INITIAL + (5 * SCENE_SCALE_STEP)
#define SCENE_MAX_TOWERS 20
#define SCENE_MAX_BULLETS 1024

bool drawInfo = false;

// -------------------
// START WAVE MANAGEMENT

int waypointsCount = 6;

V2i pathWaypoints[] = {
    {.x = 0, .y = 0},
    {.x = 0, .y = 5},
    {.x = 12, .y = 5},
    {.x = 12, .y = 3},
    {.x = 14, .y = 3},
    {.x = 14, .y = 10},
};

#define MAX_MOBS 128

typedef enum {
    MOB_STATUS_INACTIVE,
    MOB_STATUS_WAITING_SPAWN,
    MOB_STATUS_ALIVE,
    MOB_STATUS_DEAD,
} MobStatus;

int mobsHealth[MAX_MOBS];
Vector2 mobsPosition[MAX_MOBS];
MobStatus mobsStatus[MAX_MOBS];
int mobsTargetWaypointIndex[MAX_MOBS];
float mobsTimeInCurrentPath[MAX_MOBS];
int waveMobsCount = 0;
int mobsSpawned = 0;
// Tiles that it crosses in one second
float mobMovementSpeed = 1.0f;

float spawnCooldownSeconds = 0.4f;
float timeSinceLastSpawn = 0;

float waveStartDelaySeconds = 1200.f / 1000.f;
float waveTime = 0;

typedef enum {
    WAVE_STATUS_NOT_STARTED,
    WAVE_STATUS_STARTING,
    WAVE_STATUS_STARTED,
} WaveStatus;

WaveStatus waveStatus = WAVE_STATUS_NOT_STARTED;

void startWave(int mobsCount) {
    waveStatus = WAVE_STATUS_STARTING;
    waveMobsCount = mobsCount;
    mobsSpawned = 0;
    timeSinceLastSpawn = 0;
    waveTime = 0;

    for (int i = 0; i < waveMobsCount; i++) {
        mobsStatus[i] = i < waveMobsCount ? MOB_STATUS_WAITING_SPAWN : MOB_STATUS_INACTIVE;
    }
}

void updateWave(float deltaTime) {
    if (waveStatus == WAVE_STATUS_NOT_STARTED) {
        return;
    }

    waveTime += deltaTime;

    if (waveStatus == WAVE_STATUS_STARTING && waveTime >= waveStartDelaySeconds) {
        waveStatus = WAVE_STATUS_STARTED;
        return;
    }

    if (mobsSpawned < waveMobsCount) {
        timeSinceLastSpawn += deltaTime;

        if (timeSinceLastSpawn >= spawnCooldownSeconds) {
            timeSinceLastSpawn -= spawnCooldownSeconds;

            mobsHealth[mobsSpawned] = 100;
            mobsStatus[mobsSpawned] = MOB_STATUS_ALIVE;
            mobsTargetWaypointIndex[mobsSpawned] = 1;
            mobsTimeInCurrentPath[mobsSpawned] = 0;
            mobsPosition[mobsSpawned]
                = grid_getTileCenter(SCENE_TRANSFORM, pathWaypoints[0].x, pathWaypoints[0].y);

            mobsSpawned++;
        }
    }

    for (int i = 0; i < mobsSpawned; i++) {
        if (mobsStatus[i] == MOB_STATUS_ALIVE) {
            int waypointIndex = mobsTargetWaypointIndex[i];
            V2i waypointCoords = pathWaypoints[waypointIndex];
            Vector2 waypointPos
                = grid_getTileCenter(SCENE_TRANSFORM, waypointCoords.x, waypointCoords.y);

            V2i prevWaypointCoords = pathWaypoints[waypointIndex - 1];
            Vector2 prevWaypointPos
                = grid_getTileCenter(SCENE_TRANSFORM, prevWaypointCoords.x, prevWaypointCoords.y);

            int tilesX = abs(prevWaypointCoords.x - waypointCoords.x) + 1;
            int tilesY = abs(prevWaypointCoords.y - waypointCoords.y) + 1;
            int tilesScalar = tilesX == 1 ? tilesY : tilesX;
            float pathTime = tilesScalar / mobMovementSpeed;

            mobsTimeInCurrentPath[i] += deltaTime;
            mobsTimeInCurrentPath[i] = Clamp(mobsTimeInCurrentPath[i], 0, pathTime);

            float t = mobsTimeInCurrentPath[i] / pathTime;
            mobsPosition[i] = Vector2Lerp(prevWaypointPos, waypointPos, t);

            Vector2 posMin = {
                MIN(prevWaypointPos.x, waypointPos.x),
                MIN(prevWaypointPos.y, waypointPos.y),
            };

            Vector2 posMax = {
                MAX(prevWaypointPos.x, waypointPos.x),
                MAX(prevWaypointPos.y, waypointPos.y),
            };

            mobsPosition[i] = Vector2Clamp(mobsPosition[i], posMin, posMax);

            if (mobsTimeInCurrentPath[i] >= pathTime) {
                if (mobsTargetWaypointIndex[i] == waypointsCount - 1) {
                    mobsStatus[i] = MOB_STATUS_INACTIVE;
                    continue;
                }

                mobsTargetWaypointIndex[i]++;
                mobsTimeInCurrentPath[i] = 0;
                continue;
            }
        }
    }
}

void drawMobs() {
    char buffer[16];

    for (int i = 0; i < mobsSpawned; i++) {
        if (mobsStatus[i] != MOB_STATUS_ALIVE) {
            continue;
        }

        DrawRectangle(mobsPosition[i].x, mobsPosition[i].y, 20, 20, RED);

        if (drawInfo) {
            snprintf(buffer, 16, "%d", i);
            DrawText(buffer, mobsPosition[i].x, mobsPosition[i].y + 30, 16, WHITE);
        }
    }
}

bool isPath(int tileX, int tileY) {
    for (int indexEnd = 1; indexEnd < waypointsCount; indexEnd++) {
        V2i waypointStart = pathWaypoints[indexEnd - 1];
        V2i waypointEnd = pathWaypoints[indexEnd];

        if (waypointStart.x <= tileX && tileX <= waypointEnd.x && waypointStart.y <= tileY
            && tileY <= waypointEnd.y) {
            return true;
        }
    }

    return false;
}

void drawPath() {
    for (int indexEnd = 1; indexEnd < waypointsCount; indexEnd++) {
        V2i waypointStart = pathWaypoints[indexEnd - 1];
        V2i waypointEnd = pathWaypoints[indexEnd];

        Vector2 start = grid_getTileCenter(SCENE_TRANSFORM, waypointStart.x, waypointStart.y);
        Vector2 end = grid_getTileCenter(SCENE_TRANSFORM, waypointEnd.x, waypointEnd.y);

        DrawLineEx(start, end, TILE_WIDTH, (Color){234, 227, 173, 100});
    }
}

// END WAVE MANAGEMENT
// -------------------

// ----------------------
// START TOWER MANAGEMENT

typedef struct {
    bool onScene;
    V2i coords;
    int currentTargetMobIndex;
} Tower;

Tower towersPool[SCENE_MAX_TOWERS];
float towersTimeSinceLastShot[SCENE_MAX_TOWERS];

// Bullets per second
float towerRateOfFire = 3.0f;
float towerRange = 60.0f * SCENE_SCALE_INITIAL;

typedef struct {
    bool alive;
    float travelProgress;
    Vector2 position;
    V2i originTowerCoords;
    int mobTargetIndex;
} TowerBullet;

TowerBullet towerBullets[SCENE_MAX_BULLETS];

void createBullet(int mobTargetIndex, int x, int y) {
    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        if (!towerBullets[i].alive) {
            towerBullets[i].alive = true;
            towerBullets[i].travelProgress = 0;
            towerBullets[i].originTowerCoords.x = x;
            towerBullets[i].originTowerCoords.y = y;
            towerBullets[i].mobTargetIndex = mobTargetIndex;
            towerBullets[i].position = grid_getTileCenter(SCENE_TRANSFORM, x, y);

            printf("Created bullet from [%d, %d] to target enemy %d\n", x, y, mobTargetIndex);

            return;
        }
    }
}

const int BULLET_SPEED = 800; // pixels per second

void updateBullets(float deltaTime) {
    for (int i = 0; i < SCENE_MAX_BULLETS; i++) {
        if (!towerBullets[i].alive) {
            continue;
        }

        Vector2 originPos = grid_getTileCenter(SCENE_TRANSFORM,
            towerBullets[i].originTowerCoords.x,
            towerBullets[i].originTowerCoords.y);

        Vector2 targetPos = mobsPosition[towerBullets[i].mobTargetIndex];

        float scaledBulletSpeed = BULLET_SPEED * SCENE_TRANSFORM.scale / SCENE_SCALE_INITIAL;
        float distance = Vector2Distance(originPos, targetPos);
        float dt = (scaledBulletSpeed * deltaTime) / distance;

        towerBullets[i].travelProgress += dt;
        towerBullets[i].travelProgress = Clamp(towerBullets[i].travelProgress, 0, 1);

        if (towerBullets[i].travelProgress == 1) {
            towerBullets[i].alive = false;
        }

        towerBullets[i].position
            = Vector2Lerp(originPos, targetPos, towerBullets[i].travelProgress);
    }
}

float getScaledTowerRange(float range) {
    float scale = SCENE_TRANSFORM.scale / SCENE_SCALE_INITIAL;

    return range * scale;
}

bool isInRange(int mobIndex, Vector2 towerPos, float towerRange) {
    float scaledRange = getScaledTowerRange(towerRange);
    Vector2 mobPos = mobsPosition[mobIndex];

    return utils_checkCollisionPointEllipse(mobPos, towerPos, scaledRange, scaledRange / 2);
}

/// Returns -1 if no mob found
/// @param `maxDistanceSqrt` - the tower range squared, to be compared to the distance squared (to
/// avoid square roots)
int getTowerTarget(Vector2 towerPosition) {
    int closestIndex = -1;

    for (int i = 0; i < MAX_MOBS; i++) {
        if (mobsStatus[i] != MOB_STATUS_ALIVE) {
            continue;
        }

        if (!isInRange(i, towerPosition, towerRange)) {
            continue;
        }

        if (closestIndex < i) {
            closestIndex = i;
        }
    }

    return closestIndex;
}

void updateTowers(float deltaTime) {
    float towerSecondsPerBullet = 1.0f / towerRateOfFire;

    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        if (!towersPool[i].onScene) {
            continue;
        }

        Vector2 towerPos
            = grid_getTileCenter(SCENE_TRANSFORM, towersPool[i].coords.x, towersPool[i].coords.y);

        if (towersPool[i].currentTargetMobIndex == -1) {
            towersPool[i].currentTargetMobIndex = getTowerTarget(towerPos);
            continue;
        }

        int mobIndex = towersPool[i].currentTargetMobIndex;

        if (mobsStatus[mobIndex] != MOB_STATUS_ALIVE) {
            towersPool[i].currentTargetMobIndex = getTowerTarget(towerPos);
            continue;
        }

        if (!isInRange(mobIndex, towerPos, towerRange)) {
            towersPool[i].currentTargetMobIndex = -1;
            continue;
        }

        towersTimeSinceLastShot[i] += deltaTime;

        if (towersTimeSinceLastShot[i] >= towerSecondsPerBullet) {
            printf("Shooting after %0.2f seconds\n", towersTimeSinceLastShot[i]);
            towersTimeSinceLastShot[i] -= towerSecondsPerBullet;

            createBullet(towersPool[i].currentTargetMobIndex,
                towersPool[i].coords.x,
                towersPool[i].coords.y);
        }
    }
}

void createTower(int x, int y) {
    if (isPath(x, y)) {
        return;
    }

    int firstAvailableIndex = -1;

    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        if (firstAvailableIndex == -1 && !towersPool[i].onScene) {
            firstAvailableIndex = i;
            continue;
        }

        if (towersPool[i].onScene && towersPool[i].coords.x == x && towersPool[i].coords.y == y) {
            // space occupied by tower in position i
            return;
        }
    }

    if (firstAvailableIndex != -1) {
        Tower *tower = &towersPool[firstAvailableIndex];
        tower->onScene = true;
        tower->coords.x = x;
        tower->coords.y = y;
        tower->currentTargetMobIndex = -1;

        // will shoot as soon as it has a target
        towersTimeSinceLastShot[firstAvailableIndex] = 1.0f / towerRateOfFire;
    }

    // nothing happens if the tower is not set if there's no more space
}

void removeTower(int x, int y) {
    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        if (towersPool[i].coords.x == x && towersPool[i].coords.y) {
            towersPool[i].onScene = false;

            return;
        }
    }
}

// END TOWER MANAGEMENT
// --------------------

int hoveredTileIndex = -1;
const int *const scene_hoveredTileIndex = &hoveredTileIndex;

// ---------------------
// START VIEW MANAGEMENT

const Vector2 SCENE_INITIAL_POS = {200, 600};

Transform2D SCENE_TRANSFORM = {
    .scale = SCENE_SCALE_INITIAL,
    .translation = SCENE_INITIAL_POS,
    .previousTranslation = SCENE_INITIAL_POS,
};

static void resetZoomView() {
    SCENE_TRANSFORM.scale = SCENE_SCALE_INITIAL;
}

static void zoomView(float amount) {
    float newScale = utils_clampf(SCENE_SCALE_MIN, SCENE_SCALE_MAX, SCENE_TRANSFORM.scale + amount);
    if (newScale != SCENE_TRANSFORM.scale) {
        SCENE_TRANSFORM.scale = newScale;
    }
}

static void moveView(Vector2 delta) {
    SCENE_TRANSFORM.translation.x -= delta.x;
    SCENE_TRANSFORM.translation.y -= delta.y;
}

void drawIsoRecLines(IsoRec isoRec, Color color) {
    DrawLineEx(isoRec.top, isoRec.left, 2, color);
    DrawLineEx(isoRec.left, isoRec.bottom, 2, color);
    DrawLineEx(isoRec.bottom, isoRec.right, 2, color);
    DrawLineEx(isoRec.right, isoRec.top, 2, color);
}

// Public

void scene_init() {
    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        towersPool[i].coords.x = 0;
        towersPool[i].coords.y = 0;
        towersPool[i].onScene = false;
    }

    for (int i = 0; i < SCENE_MAX_BULLETS; i++) {
        towerBullets[i] = (TowerBullet){
            .alive = false,
            .travelProgress = 0,
            .mobTargetIndex = 0,
            .originTowerCoords = (V2i){0, 0},
        };
    }
}

void scene_handleInput() {
    V2i hoveredCoords
        = grid_worldPointToCoords(SCENE_TRANSFORM, input.worldMousePos.x, input.worldMousePos.y);

    hoveredTileIndex
        = grid_getTileIndexFromCoords(SCENE_COLS, SCENE_ROWS, hoveredCoords.x, hoveredCoords.y);

    if (input.mouseButtonState[MOUSE_BUTTON_RIGHT] == MOUSE_BUTTON_STATE_DRAGGING) {
        if (fabs(input.worldMouseDelta.x + input.worldMouseDelta.y) > 2) {
            return moveView(input.worldMouseDelta);
        }
    }

    if (input.mouseWheelMove > 0) {
        return zoomView(SCENE_SCALE_STEP);

    } else if (input.mouseWheelMove < 0) {
        return zoomView(-SCENE_SCALE_STEP);
    }

    if (input.keyPressed == KEY_ZERO) {
        return resetZoomView();
    }

    if (input.keyPressed == KEY_SPACE) {
        return startWave(40);
    }

    if (input.keyPressed == KEY_LEFT_ALT) {
        drawInfo = !drawInfo;
        return;
    }

    if (input.mouseButtonState[MOUSE_BUTTON_LEFT] == MOUSE_BUTTON_STATE_PRESSED) {
        if (hoveredTileIndex != -1) {
            V2i coords = grid_getCoordsFromTileIndex(SCENE_COLS, hoveredTileIndex);
            createTower(coords.x, coords.y);
        }
    }
}

// END VIEW MANAGEMENT
// -------------------

// ----------------
// PUBLIC FUNCTIONS

void scene_update(float deltaTime) {
    SCENE_TRANSFORM.previousTranslation = SCENE_TRANSFORM.translation;

    updateWave(deltaTime);
    updateTowers(deltaTime);
    updateBullets(deltaTime);
}

void scene_draw() {
    // draw tiles
    for (int i = 0; i < SCENE_TILE_COUNT; i++) {
        V2i coords = grid_getCoordsFromTileIndex(SCENE_COLS, i);
        IsoRec tile = grid_toIsoRec(SCENE_TRANSFORM, coords, (V2i){1, 1});

        DrawTexturePro(slab1Texture,
            (Rectangle){0, 0, slab1Texture.width, slab1Texture.height},
            (Rectangle){
                tile.left.x,
                tile.top.y,
                TILE_WIDTH * SCENE_TRANSFORM.scale,
                TILE_HEIGHT * SCENE_TRANSFORM.scale,
            },
            (Vector2){0, 0},
            0,
            WHITE);
    }

    drawPath();

    if (hoveredTileIndex != -1) {
        V2i coords = grid_getCoordsFromTileIndex(SCENE_COLS, hoveredTileIndex);
        IsoRec tile = grid_toIsoRec(SCENE_TRANSFORM, coords, (V2i){1, 1});
        drawIsoRecLines(tile, BROWN);

        // Draw range indicator
        if (drawInfo) {
            float scaledTowerRange = getScaledTowerRange(towerRange);
            V2i hoveredCoords = grid_getCoordsFromTileIndex(SCENE_COLS, hoveredTileIndex);
            Vector2 rangeIndicatorCenter
                = grid_getTileCenter(SCENE_TRANSFORM, hoveredCoords.x, hoveredCoords.y);

            DrawEllipse(rangeIndicatorCenter.x,
                rangeIndicatorCenter.y,
                scaledTowerRange,
                scaledTowerRange / 2,
                (Color){100, 255, 100, 10});

            for (int j = 0; j < 3; j++) {
                DrawEllipseLines(rangeIndicatorCenter.x - j,
                    rangeIndicatorCenter.y - j,
                    scaledTowerRange,
                    (scaledTowerRange) / 2,
                    (Color){40, 90, 40, 60});
            }
        }
    }

    char buffer[16];

    // draw towers
    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        if (!towersPool[i].onScene) {
            continue;
        }

        Vector2 tileCenter
            = grid_getTileCenter(SCENE_TRANSFORM, towersPool[i].coords.x, towersPool[i].coords.y);

        DrawEllipse(tileCenter.x, tileCenter.y, 16, 8, WHITE);

        if (drawInfo) {
            snprintf(buffer, 16, "%d", towersPool[i].currentTargetMobIndex);
            DrawText(buffer, tileCenter.x - 8, tileCenter.y - 30, 16, BLACK);

            int mobIndex = towersPool[i].currentTargetMobIndex;

            if (mobsStatus[mobIndex] == MOB_STATUS_ALIVE) {
                // Draw ray between tower and its target
                DrawLine(tileCenter.x,
                    tileCenter.y,
                    mobsPosition[mobIndex].x,
                    mobsPosition[mobIndex].y,
                    YELLOW);
            }
        }
    }

    drawMobs();

    for (int i = 0; i < SCENE_MAX_BULLETS; i++) {
        if (!towerBullets[i].alive) {
            continue;
        }

        DrawCircle(towerBullets[i].position.x, towerBullets[i].position.y, 4, YELLOW);
    }
}
