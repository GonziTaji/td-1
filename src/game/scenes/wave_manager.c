#include "./wave_manager.h"
#include "../../core/asset_manager.h"
#include "../../utils/grid.h"
#include "../../utils/utils.h"
#include "../constants.h"
#include "../gameplay.h"
#include "./view_mamanger.h"
#include "scene_data.h"
#include <assert.h>
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

typedef struct {
    const StatModifier *modifier;
    float timeRemaining;
    bool isActive;
} ModifierTimer;

struct {
    int maxHealth;
    float movementSpeed;
    Color color;
} mobTypeData[MOB_TYPE_COUNT] = {
    [MOB_TYPE_RED] = {100, 1, RED},
    [MOB_TYPE_BLUE] = {30, 2, BLUE},
};

int currentWaveIndex = -1;

int mobsHealth[SCENE_DATA_MAX_MOBS];
MobType mobsTypes[SCENE_DATA_MAX_MOBS];
int mobsWaveIndex[SCENE_DATA_MAX_MOBS];
Vector2 mobsPosition[SCENE_DATA_MAX_MOBS];
MobStatus mobsStatus[SCENE_DATA_MAX_MOBS];
int mobsTargetWaypointIndex[SCENE_DATA_MAX_MOBS];
float mobsTimeInCurrentPath[SCENE_DATA_MAX_MOBS];
// Tiles that it crosses in one second
float mobsMovementSpeed[SCENE_DATA_MAX_MOBS];
ModifierTimer mobsModifiersTimers[SCENE_DATA_MAX_MOBS][SCENE_DATA_MAX_MOB_STAT_MODS];

int totalMobsCount = 0;

float spawnCooldownSeconds = 0.4f;

typedef enum {
    WAVE_STATUS_NOT_STARTED,
    WAVE_STATUS_STARTED,
    WAVE_STATUS_ENDED,
} WaveStatus;

WaveStatus wavesStatus[SCENE_DATA_MAX_WAVES];
float wavesSpawnTimers[SCENE_DATA_MAX_WAVES];
float wavesStartTimer[SCENE_DATA_MAX_WAVES];
float wavesMobsRemainingToSpawn[SCENE_DATA_MAX_WAVES];

void drawMobs() {
    char buffer[16];

    for (int i = 0; i < totalMobsCount; i++) {
        if (mobsStatus[i] != MOB_STATUS_ALIVE) {
            continue;
        }

        int mobWidth = 10;
        int mobHeight = 10;

        Vector2 drawOrigin
            = Vector2Subtract(mobsPosition[i], (Vector2){mobWidth / 2.0f, mobHeight / 2.0f});

        Rectangle mobRec = {
            drawOrigin.x,
            drawOrigin.y,
            mobWidth,
            mobHeight,
        };

        Color mobColor = WHITE;

        switch (mobsTypes[i]) {
        case MOB_TYPE_RED:
            mobColor = RED;
            break;
        case MOB_TYPE_BLUE:
            break;
        case MOB_TYPE_COUNT:
            assert(false && "Unexpected MOB_TYPE_COUNT enum value");
            break;
        }

        DrawRectangleRec(mobRec, mobColor);

        if (gameplay_drawInfo) {
            drawOrigin.y -= 30;
            snprintf(buffer, 16, "%d", mobsHealth[i]);
            DrawTextEx(uiFont, buffer, drawOrigin, 16, 1, WHITE);

            drawOrigin.y += 30 + 30;
            snprintf(buffer, 16, "%d", i);
            DrawTextEx(uiFont, buffer, drawOrigin, 16, 1, WHITE);
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

float applyModifiers(int mobIndex, float value, ModifierEffectType effectType) {
    float newValue = value;

    ModifierTimer *timers = mobsModifiersTimers[mobIndex];

    for (int i = 0; i < SCENE_DATA_MAX_MOB_STAT_MODS; i++) {
        const StatModifier *modifier = timers[i].modifier;

        if (modifier == NULL || !timers[i].isActive) {
            continue;
        }

        if (modifier->type != effectType) {
            continue;
        }

        switch (modifier->valueType) {
        case MODIFIER_VALUE_TYPE_FLAT:
            newValue += modifier->value;
            break;
        case MODIFIER_VALUE_TYPE_PERCENT:
            newValue += newValue * (modifier->value / 100);
            break;
        case MODIFIER_VALUE_TYPE_MULTIPLIER:
            newValue *= modifier->value;
            break;
        }
    }

    return newValue;
}

// float getModifiedMovementSpeed(int mobIndex) {
//     float movementSpeed = mobsMovementSpeed[mobIndex];
//
//     ModifierTimer *timers = mobsModifiersTimers[mobIndex];
//
//     for (int i = 0; i < SCENE_DATA_MAX_MOB_STAT_MODS; i++) {
//         const StatModifier *modifier = timers[i].modifier;
//
//         if (modifier == NULL || !timers[i].isActive) {
//             continue;
//         }
//
//         if (modifier->type != MODIFIER_EFFECT_TYPE_SLOW) {
//             continue;
//         }
//
//         switch (modifier->valueType) {
//         case MODIFIER_VALUE_TYPE_FLAT:
//             movementSpeed += modifier->value;
//             break;
//         case MODIFIER_VALUE_TYPE_PERCENT:
//             movementSpeed += movementSpeed * (modifier->value / 100);
//             break;
//         case MODIFIER_VALUE_TYPE_MULTIPLIER:
//             movementSpeed *= modifier->value;
//             break;
//         }
//     }
//
//     return movementSpeed;
// }

// Public functions

// Mob functions

void wave_mob_removeModifier(int mobIndex, int modifierId) {
    ModifierTimer *timers = mobsModifiersTimers[mobIndex];

    for (int i = 0; i < SCENE_DATA_MAX_MOB_STAT_MODS; i++) {
        if (timers[i].modifier == NULL) {
            continue;
        }

        if (timers[i].modifier->id == modifierId) {
            timers[i].modifier = NULL;
            timers[i].isActive = false;
            return;
        }
    }
}

void wave_mob_addModifier(int mobIndex, const StatModifier *modifierData) {
    int availableSlotIndex = -1;

    ModifierTimer *timers = mobsModifiersTimers[mobIndex];

    for (int i = 0; i < SCENE_DATA_MAX_MOB_STAT_MODS; i++) {
        if (timers[i].isActive && timers[i].modifier->id == modifierData->id) {
            // refresh timer and exit
            timers[i].timeRemaining = modifierData->duration;
            return;
        }

        if (timers[i].isActive) {
            continue;
        }

        if (availableSlotIndex == -1) {
            availableSlotIndex = i;
            continue;
        }
    }

    if (availableSlotIndex != -1) {
        timers[availableSlotIndex].isActive = true;
        timers[availableSlotIndex].modifier = modifierData;
        timers[availableSlotIndex].timeRemaining = modifierData->duration;
    }
}

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
    return totalMobsCount;
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

void wave_initData() {
    currentWaveIndex = -1;
    totalMobsCount = 0;

    for (int i = 0; i < SCENE_DATA_MAX_MOBS; i++) {
        mobsStatus[i] = MOB_STATUS_INACTIVE;
        mobsPosition[i] = Vector2Zero();
        mobsWaveIndex[i] = -1;
    }

    for (int i = 0; i < SCENE_DATA_MAX_WAVES; i++) {
        const WaveData *currentWave = &SCENE_DATA->waves[i];

        wavesSpawnTimers[i] = spawnCooldownSeconds;
        wavesStatus[i] = WAVE_STATUS_NOT_STARTED;
        wavesStartTimer[i] = currentWave->startDelaySeconds;
        wavesMobsRemainingToSpawn[i] = currentWave->mobsCount;

        int oldMobsCount = totalMobsCount;
        totalMobsCount += currentWave->mobsCount;

        for (int i = oldMobsCount; i < totalMobsCount; i++) {
            mobsStatus[i] = MOB_STATUS_WAITING_SPAWN;
            mobsTypes[i] = currentWave->mobType;
            mobsHealth[i] = mobTypeData[currentWave->mobType].maxHealth;
            mobsWaveIndex[i] = currentWaveIndex;
            mobsMovementSpeed[i] = mobTypeData[currentWave->mobType].movementSpeed;
            mobsTimeInCurrentPath[i] = 0;
            mobsTargetWaypointIndex[i] = 1;

            // this will change when multiple spawn points is implemented
            const V2i *coords = &SCENE_DATA->pathWaypoints[0];
            mobsPosition[i] = grid_getTileCenter(SCENE_TRANSFORM, coords->x, coords->y);

            // modifiers
            for (int j = 0; j < SCENE_DATA_MAX_MOB_STAT_MODS; j++) {
                mobsModifiersTimers[i][j].modifier = NULL;
                mobsModifiersTimers[i][j].timeRemaining = 0;
                mobsModifiersTimers[i][j].isActive = false;
            }
        }
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
    wavesStartTimer[currentWaveIndex] = SCENE_DATA->waves[currentWaveIndex].startDelaySeconds;

    const WaveData *currentWave = &SCENE_DATA->waves[currentWaveIndex];

    int oldMobsCount = totalMobsCount;

    totalMobsCount += currentWave->mobsCount;

    for (int i = oldMobsCount; i < totalMobsCount; i++) {
        mobsStatus[i] = MOB_STATUS_WAITING_SPAWN;
        mobsTypes[i] = currentWave->mobType;
        mobsHealth[i] = mobTypeData[currentWave->mobType].maxHealth;
        mobsWaveIndex[i] = currentWaveIndex;
        mobsMovementSpeed[i] = mobTypeData[currentWave->mobType].movementSpeed;
        mobsTimeInCurrentPath[i] = 0;
        mobsTargetWaypointIndex[i] = 1;

        const V2i *coords = &SCENE_DATA->pathWaypoints[0];
        mobsPosition[i] = grid_getTileCenter(SCENE_TRANSFORM, coords->x, coords->y);
    }
}

void wave_update(float deltaTime) {
    for (int i = 0; i < SCENE_DATA->wavesCount; i++) {
        // TODO: switch?
        if (wavesStatus[i] == WAVE_STATUS_ENDED) {
            continue;
        }

        if (wavesStatus[i] == WAVE_STATUS_NOT_STARTED) {
            wavesStartTimer[i] -= deltaTime;

            if (wavesStartTimer[i] <= 0) {
                wave_startNext();
            }

            continue;
        }

        if (wavesStatus[i] == WAVE_STATUS_STARTED) {
            if (wavesMobsRemainingToSpawn[i] == 0) {
                wavesStatus[i] = WAVE_STATUS_ENDED;
            }

            wavesSpawnTimers[i] -= deltaTime;
        }
    }

    for (int i = 0; i < totalMobsCount; i++) {
        int waveIndex = mobsWaveIndex[i];

        if (mobsStatus[i] == MOB_STATUS_WAITING_SPAWN && wavesSpawnTimers[waveIndex] <= 0) {
            mobsStatus[i] = MOB_STATUS_ALIVE;
            wavesSpawnTimers[waveIndex] += spawnCooldownSeconds;
            wavesMobsRemainingToSpawn[waveIndex]--;
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

            mobsTimeInCurrentPath[i] += applyModifiers(i, deltaTime, MODIFIER_EFFECT_TYPE_SLOW);
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
            }

            for (int timerIndex = 0; timerIndex < SCENE_DATA_MAX_MOB_STAT_MODS; timerIndex++) {
                ModifierTimer *timer = &mobsModifiersTimers[i][timerIndex];

                if (timer == NULL || !timer->isActive) {
                    continue;
                }

                if (timer->modifier->durationType == DURATION_TYPE_PERMANENT) {
                    continue;
                }

                timer->timeRemaining -= deltaTime;

                if (timer->timeRemaining <= 0) {
                    timer->isActive = false;
                    timer->timeRemaining = 0;
                }
            }
        }
    }
}

void wave_draw() {
    // for debug. Eventually, path will have a different sprite
    drawPath();
    drawMobs();
}
