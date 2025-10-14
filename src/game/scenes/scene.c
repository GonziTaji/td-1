#include "scene.h"
#include "../../core/asset_manager.h"
#include "../../input/input.h"
#include "../../utils/grid.h"
#include "../constants.h"
#include <assert.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SCENE_SCALE_INITIAL 4.0f
#define SCENE_SCALE_STEP 0.4f
#define SCENE_SCALE_MIN SCENE_SCALE_INITIAL - (2 * SCENE_SCALE_STEP)
#define SCENE_SCALE_MAX SCENE_SCALE_INITIAL + (5 * SCENE_SCALE_STEP)
#define SCENE_MAX_TOWERS 20

int hoveredTileIndex = -1;

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
float mobMovementSpeed = 4.0f;

float spawnCooldownSeconds = 0.2f;
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

Vector2 mobMovement;

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

            float t = mobsTimeInCurrentPath[i] / pathTime;
            Vector2 prevMobPos = Vector2Lerp(prevWaypointPos, waypointPos, t);

            mobsTimeInCurrentPath[i] += deltaTime;
            mobsTimeInCurrentPath[i] = Clamp(mobsTimeInCurrentPath[i], 0, pathTime);

            t = mobsTimeInCurrentPath[i] / pathTime;
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

            mobMovement = Vector2Subtract(mobsPosition[i], prevMobPos);
            mobMovement.x = fabs(mobMovement.x);
            mobMovement.y = fabs(mobMovement.y);

            if (mobMovement.x == 0 && mobMovement.y == 0) {
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
    for (int i = 0; i < mobsSpawned; i++) {
        if (mobsStatus[i] != MOB_STATUS_ALIVE) {
            continue;
        }

        DrawRectangle(mobsPosition[i].x, mobsPosition[i].y, 20, 20, RED);

        // Print mob pos
        char buffer[16];
        snprintf(buffer, 16, "%.0f, %.0f", mobsPosition[i].x, mobsPosition[i].y);
        DrawText(buffer, mobsPosition[i].x, mobsPosition[i].y + 64, 16, WHITE);
    }
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
} Tower;

Tower towersPool[SCENE_MAX_TOWERS];

void createTower(int x, int y) {
    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        if (!towersPool[i].onScene) {
            Tower *tower = &towersPool[i];
            tower->onScene = true;
            tower->coords.x = x;
            tower->coords.y = y;

            return;
        }
    }

    assert(false);
}

void removeTower(int x, int y) {
    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        if (towersPool[i].coords.x == x && towersPool[i].coords.y) {
            towersPool[i].coords.x = 0;
            towersPool[i].coords.y = 0;
            towersPool[i].onScene = false;

            return;
        }
    }
}

// END TOWER MANAGEMENT
// --------------------

// ---------------------
// START VIEW MANAGEMENT

Transform2D SCENE_TRANSFORM = {
    .scale = SCENE_SCALE_INITIAL,
    .translation = {200, 600},
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

    createTower(1, 1);
    createTower(3, 3);
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
        return startWave(10);
    }
}

// END VIEW MANAGEMENT
// -------------------

void scene_update(float deltaTime) {
    updateWave(deltaTime);
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
    }

    // draw towers
    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        if (!towersPool[i].onScene) {
            continue;
        }

        Vector2 tileCenter
            = grid_getTileCenter(SCENE_TRANSFORM, towersPool[i].coords.x, towersPool[i].coords.y);

        DrawEllipse(tileCenter.x, tileCenter.y, 16, 8, WHITE);
    }

    drawMobs();
}
