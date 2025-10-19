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

#define MAX_MOBS 128

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

void drawMobs() {
    char buffer[16];

    for (int i = 0; i < mobsSpawned; i++) {
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

// This could change if there are N waves in the screen,
// and is cleaner than exposing `waveMobsCount` with a pointer
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

// Wave lifecycle

void wave_start(int mobsCount) {
    waveStatus = WAVE_STATUS_STARTING;
    waveMobsCount = mobsCount;
    mobsSpawned = 0;
    timeSinceLastSpawn = 0;
    waveTime = 0;

    for (int i = 0; i < MAX_MOBS; i++) {
        mobsStatus[i] = i < waveMobsCount ? MOB_STATUS_WAITING_SPAWN : MOB_STATUS_INACTIVE;
    }
}

void wave_update(float deltaTime) {
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
            mobsPosition[mobsSpawned] = grid_getTileCenter(
                SCENE_TRANSFORM, SCENE_DATA->pathWaypoints[0].x, SCENE_DATA->pathWaypoints[0].y);

            mobsSpawned++;
        }
    }

    for (int i = 0; i < mobsSpawned; i++) {
        if (mobsStatus[i] == MOB_STATUS_ALIVE) {
            int waypointIndex = mobsTargetWaypointIndex[i];
            V2i waypointCoords = SCENE_DATA->pathWaypoints[waypointIndex];
            Vector2 waypointPos
                = grid_getTileCenter(SCENE_TRANSFORM, waypointCoords.x, waypointCoords.y);

            V2i prevWaypointCoords = SCENE_DATA->pathWaypoints[waypointIndex - 1];
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
