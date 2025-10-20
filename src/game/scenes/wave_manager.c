#include "./wave_manager.h"
#include "../../utils/grid.h"
#include "../../utils/utils.h"
#include "../constants.h"
#include "../gameplay.h"
#include "./scene_data.h"
#include "./view_mamanger.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    MOB_STATUS_INACTIVE,
    MOB_STATUS_WAITING_SPAWN,
    MOB_STATUS_ALIVE,
    MOB_STATUS_DEAD,
} MobStatus;

// typedef enum {
//     MOB_TYPE_RED,
//     MOB_TYPE_BLUE,
//     MOB_TYPE_COUNT,
// } MobType;
//
// struct {
//     int maxHealth;
//     float movementSpeed;
// } mobTypeData[MOB_TYPE_COUNT] = {
//     [MOB_TYPE_RED] = { 100, 1 },
//     [MOB_TYPE_BLUE] = { 30, 2 },
// };

int currentWaveIndex = -1;

int mobsHealth[SCENE_DATA_MAX_MOBS];
int mobsWaveIndex[SCENE_DATA_MAX_MOBS];
Vector2 mobsPosition[SCENE_DATA_MAX_MOBS];
MobStatus mobsStatus[SCENE_DATA_MAX_MOBS];
int mobsTargetWaypointIndex[SCENE_DATA_MAX_MOBS];
float mobsTimeInCurrentPath[SCENE_DATA_MAX_MOBS];
// Tiles that it crosses in one second
float mobsMovementSpeed[SCENE_DATA_MAX_MOBS];

int waveMobsCount = 0;

float spawnCooldownSeconds = 0.4f;

float wavesSpawnTimers[SCENE_DATA_MAX_WAVES];
float waves[SCENE_DATA_MAX_WAVES];

typedef enum {
    WAVE_STATUS_NOT_STARTED,
    WAVE_STATUS_STARTED,
} WaveStatus;

WaveStatus wavesStatus[SCENE_DATA_MAX_WAVES];

void drawMobs() {
    char buffer[16];

    for (int i = 0; i < waveMobsCount; i++) {
        if (mobsStatus[i] != MOB_STATUS_ALIVE) {
            continue;
        }

        int mobWidth = 10;
        int mobHeight = 10;

        Vector2 drawOrigin
            = Vector2Subtract(mobsPosition[i], (Vector2){mobWidth / 2.0f, mobHeight / 2.0f});

        Rectangle mobRect = {
            drawOrigin.x,
            drawOrigin.y,
            mobWidth,
            mobHeight,
        };

        DrawRectanglePro(mobRect, Vector2Zero(), 0, RED);

        if (gameplay_drawInfo) {
            snprintf(buffer, 16, "%d", mobsHealth[i]);
            DrawText(buffer, drawOrigin.x, drawOrigin.y - 30, 16, WHITE);

            snprintf(buffer, 16, "%d", i);
            DrawText(buffer, drawOrigin.x, drawOrigin.y + 30, 16, WHITE);
        }
    }
}

void drawPath() {
    for (int indexEnd = 1; indexEnd < SCENE_DATA->pathWaypointsCount; indexEnd++) {
        V2i waypointStart = SCENE_DATA->pathWaypoints[indexEnd - 1];
        V2i waypointEnd = SCENE_DATA->pathWaypoints[indexEnd];

        Vector2 start = grid_getTileCenter(SCENE_TRANSFORM, waypointStart.x, waypointStart.y);
        Vector2 end = grid_getTileCenter(SCENE_TRANSFORM, waypointEnd.x, waypointEnd.y);

        DrawLineEx(start, end, TILE_WIDTH, (Color){234, 227, 173, 100});
    }
}

float getPathTime(int waypointIndex, float movementSpeed) {
    if (waypointIndex == 0)
        return 0;

    V2i startCoords = SCENE_DATA->pathWaypoints[waypointIndex - 1];
    V2i endCoords = SCENE_DATA->pathWaypoints[waypointIndex];

    int tilesX = abs(startCoords.x - endCoords.x) + 1;
    int tilesY = abs(startCoords.y - endCoords.y) + 1;
    int tilesScalar = tilesX == 1 ? tilesY : tilesX;
    float pathTime = tilesScalar / movementSpeed;

    return pathTime;
}

// Public functions

// Wave utils

int wave_mob_isAlive(int mobIndex) {
    return mobsStatus[mobIndex] == MOB_STATUS_ALIVE;
}

Vector2 wave_mob_getPosition(int mobIndex) {
    return mobsPosition[mobIndex];
}

void wave_mob_takeDamage(int mobIndex, int damage) {
    mobsHealth[mobIndex] -= damage;

    if (mobsHealth[mobIndex] <= 0) {
        mobsStatus[mobIndex] = MOB_STATUS_DEAD;
    }
}

float wave_mob_getPercentajeTraveled(int mobIndex) {
    float totalTime = 0;
    float timeTraveled = 0;

    for (int i = 0; i < SCENE_DATA->pathWaypointsCount; i++) {
        if (i == 0) {
            continue;
        }

        totalTime += getPathTime(i, mobsMovementSpeed[mobIndex]);

        if (mobsTargetWaypointIndex[mobIndex] == i) {
            timeTraveled += mobsTimeInCurrentPath[mobIndex];
            continue;
        }

        if (mobsTargetWaypointIndex[mobIndex] < i) {
            continue;
        }

        timeTraveled = totalTime;
    }

    float percentageTraveled = timeTraveled * 100 / totalTime;

    return percentageTraveled;
}

int wave_getMobCount() {
    return waveMobsCount;
}

bool wave_isPath(int tileX, int tileY) {
    for (int indexEnd = 1; indexEnd < SCENE_DATA->pathWaypointsCount; indexEnd++) {
        V2i waypointStart = SCENE_DATA->pathWaypoints[indexEnd - 1];
        V2i waypointEnd = SCENE_DATA->pathWaypoints[indexEnd];

        if (waypointStart.x <= tileX && tileX <= waypointEnd.x && waypointStart.y <= tileY
            && tileY <= waypointEnd.y) {
            return true;
        }
    }

    return false;
}

void wave_clear() {
    currentWaveIndex = -1;
    waveMobsCount = 0;

    for (int i = 0; i < SCENE_DATA_MAX_MOBS; i++) {
        mobsStatus[i] = MOB_STATUS_INACTIVE;
        mobsPosition[i] = Vector2Zero();
        mobsWaveIndex[i] = -1;
    }

    for (int i = 0; i < SCENE_DATA_MAX_WAVES; i++) {
        wavesSpawnTimers[i] = 0;
        wavesStatus[i] = WAVE_STATUS_NOT_STARTED;
    }
}

void wave_startNext() {
    if (currentWaveIndex >= SCENE_DATA->wavesCount) {
        // LOG? no more waves
        return;
    }

    currentWaveIndex += 1;

    wavesStatus[currentWaveIndex] = WAVE_STATUS_STARTED;
    wavesSpawnTimers[currentWaveIndex] = spawnCooldownSeconds;

    const WaveData *currentWave = &SCENE_DATA->waves[currentWaveIndex];

    int oldMobsCount = waveMobsCount;

    waveMobsCount += currentWave->mobsCount;

    for (int i = oldMobsCount; i < waveMobsCount; i++) {
        mobsStatus[i] = MOB_STATUS_WAITING_SPAWN;
        mobsHealth[i] = currentWave->mobMaxHealth;
        mobsWaveIndex[i] = currentWaveIndex;
        mobsMovementSpeed[i] = currentWave->mobMovementSpeed;
        mobsTimeInCurrentPath[i] = 0;
        mobsTargetWaypointIndex[i] = 1;

        const V2i *coords = &SCENE_DATA->pathWaypoints[0];
        mobsPosition[i] = grid_getTileCenter(SCENE_TRANSFORM, coords->x, coords->y);
    }
}

void wave_update(float deltaTime) {
    for (int i = 0; i < SCENE_DATA->wavesCount; i++) {
        if (wavesStatus[i] == WAVE_STATUS_NOT_STARTED) {
            continue;
        }

        wavesSpawnTimers[i] -= deltaTime;
    }

    for (int i = 0; i < waveMobsCount; i++) {
        int waveIndex = mobsWaveIndex[i];

        if (mobsStatus[i] == MOB_STATUS_WAITING_SPAWN && wavesSpawnTimers[waveIndex] <= 0) {
            wavesSpawnTimers[waveIndex] += spawnCooldownSeconds;
            mobsStatus[i] = MOB_STATUS_ALIVE;
        }

        if (mobsStatus[i] == MOB_STATUS_ALIVE) {
            int waypointIndex = mobsTargetWaypointIndex[i];
            V2i waypointCoords = SCENE_DATA->pathWaypoints[waypointIndex];
            Vector2 waypointPos
                = grid_getTileCenter(SCENE_TRANSFORM, waypointCoords.x, waypointCoords.y);

            V2i prevWaypointCoords = SCENE_DATA->pathWaypoints[waypointIndex - 1];
            Vector2 prevWaypointPos
                = grid_getTileCenter(SCENE_TRANSFORM, prevWaypointCoords.x, prevWaypointCoords.y);

            float pathTime = getPathTime(waypointIndex, mobsMovementSpeed[i]);

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
                if (mobsTargetWaypointIndex[i] == SCENE_DATA->pathWaypointsCount - 1) {
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

void wave_draw() {
    // for debug. Eventually, path will have a different sprite
    drawPath();
    drawMobs();
}
