#include "../../input/input.h"
#include "../../utils/grid.h"
#include "../../utils/utils.h"
#include "../constants.h"
#include "../gameplay.h"
#include "./scene_data.h"
#include "./view_mamanger.h"
#include "wave_manager.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>

#define SCENE_MAX_TOWERS 20
#define SCENE_MAX_BULLETS 1024

static char buffer[16];

GameplayMode gameplayMode = GAMEPLAY_MODE_NORMAL;

typedef enum {
    TOWER_TYPE_WHITE,
    TOWER_TYPE_BLUE,
    TOWER_TYPE_RED,
    TOWER_TYPE_COUNT,
} TowerType;

typedef struct {
    TowerType type;
    bool onScene;
    V2i coords;
    int currentTargetMobIndex;
    float timeSinceLastShot;
} Tower;

typedef enum {
    BULLET_TYPE_SINGLE_TARGET,
    BULLET_TYPE_AOE,
} BulletTargetType;

const StatModifier slowModifier_1 = {
    .id = 1,
    .type = MODIFIER_EFFECT_TYPE_SLOW,
    .durationType = DURATION_TYPE_TEMPORARY,
    .duration = 1.0f,
    .value = -50,
    .valueType = MODIFIER_VALUE_TYPE_PERCENT,
};

const StatModifier dotModifier_1 = {
    .id = 2,
    .type = MODIFIER_EFFECT_TYPE_DOT,
    .durationType = DURATION_TYPE_TEMPORARY,
    .duration = 7.0f,
    .value = -1.0f,
    .valueType = MODIFIER_VALUE_TYPE_FLAT,
};

const struct {
    // bullets per second
    float rateOfFile[TOWER_TYPE_COUNT];
    float range[TOWER_TYPE_COUNT];
    int bulletDamage[TOWER_TYPE_COUNT];
    int bulletAOE[TOWER_TYPE_COUNT];
    Color bulletColor[TOWER_TYPE_COUNT];
    int bulletWidth[TOWER_TYPE_COUNT];
    int bulletSpeed[TOWER_TYPE_COUNT];
    BulletTargetType bulletTargetType[TOWER_TYPE_COUNT];
    const StatModifier *bulletModifier[TOWER_TYPE_COUNT];
    Color color[TOWER_TYPE_COUNT];
} towerTypeData = {
    .rateOfFile = {8, 1.2f, 0.8f},
    .range = {320, 240, 280},
    .bulletDamage = {6, 8, 18},
    .bulletAOE = {0, 40, 60},
    .bulletColor = {YELLOW, SKYBLUE, DARKBLUE},
    .bulletWidth = {4, 10, 16},
    .bulletSpeed = {800, 500, 300},
    .bulletTargetType = {BULLET_TYPE_SINGLE_TARGET, BULLET_TYPE_AOE, BULLET_TYPE_AOE},
    .bulletModifier = {&dotModifier_1, &slowModifier_1, NULL},
    .color = {WHITE, BLUE, DARKPURPLE},
};

Tower towersPool[SCENE_MAX_TOWERS];
TowerType towerToPlaceType = TOWER_TYPE_WHITE;

typedef struct {
    bool alive;
    float travelProgress;
    Vector2 position;
    TowerType originTowerType;
    V2i originTowerCoords;
    int damage;
    int AOE;
    BulletTargetType targetType;
    int mobTargetIndex;
    const StatModifier *modifier;
} TowerBullet;

TowerBullet towerBullets[SCENE_MAX_BULLETS];

void createBullet(TowerType towerType, int mobTargetIndex, int x, int y) {
    for (int i = 0; i < SCENE_MAX_BULLETS; i++) {
        if (!towerBullets[i].alive) {
            towerBullets[i].alive = true;
            towerBullets[i].travelProgress = 0;
            towerBullets[i].originTowerType = towerType;
            towerBullets[i].originTowerCoords.x = x;
            towerBullets[i].originTowerCoords.y = y;
            towerBullets[i].mobTargetIndex = mobTargetIndex;
            towerBullets[i].position = grid_getTileCenter(SCENE_TRANSFORM, x, y);
            towerBullets[i].damage = towerTypeData.bulletDamage[towerType];
            towerBullets[i].targetType = towerTypeData.bulletTargetType[towerType];
            towerBullets[i].AOE = towerTypeData.bulletAOE[towerType];
            towerBullets[i].modifier = towerTypeData.bulletModifier[towerType];

            printf("Created bullet from [%d, %d] to target enemy %d\n", x, y, mobTargetIndex);

            return;
        }
    }
}

float getScaledTowerRange(float range) {
    float scale = SCENE_TRANSFORM->scale / SCENE_SCALE_INITIAL;

    return range * scale;
}

bool isInRange(int mobIndex, Vector2 towerPos, float towerRange) {
    float scaledRange = getScaledTowerRange(towerRange);

    Vector2 mobPos = wave_mob_getPosition(mobIndex);
    return utils_checkCollisionPointEllipse(mobPos, towerPos, scaledRange, scaledRange / 2);
}

/// Returns -1 if no mob found
/// @param `maxDistanceSqrt` - the tower range squared, to be compared to the distance squared (to
/// avoid square roots)
int getTowerTarget(Vector2 towerPosition, float towerRange) {
    int mobCount = wave_getMobCount();

    int mostTraveled = 0;
    int targetIndex = -1;

    for (int i = 0; i < mobCount; i++) {
        if (!wave_mob_isAlive(i)) {
            continue;
        }

        if (!isInRange(i, towerPosition, towerRange)) {
            continue;
        }

        float traveled = wave_mob_getPercentajeTraveled(i);

        if (traveled > mostTraveled) {
            mostTraveled = traveled;
            targetIndex = i;
        }
    }

    return targetIndex;
}

void placeTower(int x, int y) {
    if (wave_isPath(x, y)) {
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
        tower->type = towerToPlaceType;
        tower->onScene = true;
        tower->coords.x = x;
        tower->coords.y = y;
        tower->currentTargetMobIndex = -1;
        // will shoot as soon as it has a target
        tower->timeSinceLastShot = 1.0f / towerTypeData.rateOfFile[towerToPlaceType];

        gameplayMode = GAMEPLAY_MODE_NORMAL;
    }

    // nothing happens if the tower is not set because there's no more space
}

void removeTower(int x, int y) {
    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        if (towersPool[i].coords.x == x && towersPool[i].coords.y == y) {
            towersPool[i].onScene = false;

            return;
        }
    }
}

void towers_clear() {
    gameplayMode = GAMEPLAY_MODE_NORMAL;

    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        towersPool[i].onScene = false;
        towersPool[i].currentTargetMobIndex = -1;
        towersPool[i].timeSinceLastShot = 0;
    }

    for (int i = 0; i < SCENE_MAX_BULLETS; i++) {
        towerBullets[i].alive = false;
    }
}

void towers_handleInput() {
    if (input.mouseButtonState[MOUSE_BUTTON_LEFT] == MOUSE_BUTTON_STATE_PRESSED) {
        V2i coords = grid_worldPointToCoords(
            SCENE_TRANSFORM, input.worldMousePos.x, input.worldMousePos.y);

        if (grid_isValidCoords(SCENE_DATA->cols, SCENE_DATA->rows, coords.x, coords.y)) {
            switch (gameplayMode) {
            case GAMEPLAY_MODE_NORMAL:
                // Select tower?
                break;
            case GAMEPLAY_MODE_TOWER_REMOVE:
                removeTower(coords.x, coords.y);
                break;
            case GAMEPLAY_MODE_TOWER_PLACE:
                placeTower(coords.x, coords.y);
                break;
            }
        }
    }

    if (input.keyPressed == KEY_T) {
        gameplayMode = gameplayMode != GAMEPLAY_MODE_TOWER_PLACE ? GAMEPLAY_MODE_TOWER_PLACE
                                                                 : GAMEPLAY_MODE_NORMAL;
    }

    if (input.keyPressed == KEY_R) {
        gameplayMode = gameplayMode != GAMEPLAY_MODE_TOWER_REMOVE ? GAMEPLAY_MODE_TOWER_REMOVE
                                                                  : GAMEPLAY_MODE_NORMAL;
    }

    if (gameplayMode == GAMEPLAY_MODE_TOWER_PLACE && input.keyPressed == KEY_TAB) {
        towerToPlaceType++;

        if (towerToPlaceType == TOWER_TYPE_COUNT) {
            towerToPlaceType = 0;
        }
    }
}

void updateTowers(float deltaTime) {
    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        if (!towersPool[i].onScene) {
            continue;
        }

        TowerType type = towersPool[i].type;

        float towerSecondsPerBullet = 1.0f / towerTypeData.rateOfFile[type];

        Vector2 towerPos
            = grid_getTileCenter(SCENE_TRANSFORM, towersPool[i].coords.x, towersPool[i].coords.y);

        if (towersPool[i].currentTargetMobIndex == -1) {
            towersPool[i].currentTargetMobIndex
                = getTowerTarget(towerPos, towerTypeData.range[type]);
            continue;
        }

        int mobIndex = towersPool[i].currentTargetMobIndex;

        if (!wave_mob_isAlive(mobIndex)) {
            towersPool[i].currentTargetMobIndex
                = getTowerTarget(towerPos, towerTypeData.range[type]);
            continue;
        }

        if (!isInRange(mobIndex, towerPos, towerTypeData.range[type])) {
            towersPool[i].currentTargetMobIndex = -1;
            continue;
        }

        towersPool[i].timeSinceLastShot += deltaTime;

        if (towersPool[i].timeSinceLastShot >= towerSecondsPerBullet) {
            printf("Shooting after %0.2f seconds\n", towersPool[i].timeSinceLastShot);
            towersPool[i].timeSinceLastShot -= towerSecondsPerBullet;

            createBullet(type,
                towersPool[i].currentTargetMobIndex,
                towersPool[i].coords.x,
                towersPool[i].coords.y);
        }
    }
}

// --------
// UPDATE -

void updateBullets(float deltaTime) {
    for (int bulletIndex = 0; bulletIndex < SCENE_MAX_BULLETS; bulletIndex++) {
        if (!towerBullets[bulletIndex].alive) {
            continue;
        }

        Vector2 originPos = grid_getTileCenter(SCENE_TRANSFORM,
            towerBullets[bulletIndex].originTowerCoords.x,
            towerBullets[bulletIndex].originTowerCoords.y);

        Vector2 targetPos = wave_mob_getPosition(towerBullets[bulletIndex].mobTargetIndex);
        // towerBullets[i].originTowerCoords.y);

        TowerType towerType = towerBullets[bulletIndex].originTowerType;
        float bulletSpeed = towerTypeData.bulletSpeed[towerType];
        float bulletDamage = towerTypeData.bulletDamage[towerType];

        float scaledBulletSpeed = bulletSpeed * SCENE_TRANSFORM->scale / SCENE_SCALE_INITIAL;
        float distance = Vector2Distance(originPos, targetPos);
        float dt = (scaledBulletSpeed * deltaTime) / distance;

        towerBullets[bulletIndex].travelProgress += dt;
        towerBullets[bulletIndex].travelProgress
            = Clamp(towerBullets[bulletIndex].travelProgress, 0, 1);

        towerBullets[bulletIndex].position
            = Vector2Lerp(originPos, targetPos, towerBullets[bulletIndex].travelProgress);

        if (towerBullets[bulletIndex].travelProgress != 1) {
            continue;
        }

        int damagedMobIndex = towerBullets[bulletIndex].mobTargetIndex;
        wave_mob_takeDamage(damagedMobIndex, bulletDamage);
        towerBullets[bulletIndex].alive = false;

        if (towerBullets[bulletIndex].modifier != NULL) {
            wave_mob_addModifier(damagedMobIndex, towerBullets[bulletIndex].modifier);
        }

        // check aoe
        if (towerTypeData.bulletAOE[towerType] > 0) {
            float aoeSqrt = pow(towerTypeData.bulletAOE[towerType], 2);
            int mobCount = wave_getMobCount();

            // @performance: optimization oportunity
            for (int otherMobIndex = 0; otherMobIndex < mobCount; otherMobIndex++) {
                if (otherMobIndex == damagedMobIndex) {
                    continue;
                }

                if (!wave_mob_isAlive(otherMobIndex)) {
                    continue;
                }

                float distanceSqrt = Vector2DistanceSqr(
                    wave_mob_getPosition(damagedMobIndex), wave_mob_getPosition(otherMobIndex));

                if (aoeSqrt >= distanceSqrt) {
                    wave_mob_takeDamage(otherMobIndex, bulletDamage);

                    if (towerBullets[bulletIndex].modifier != NULL) {
                        wave_mob_addModifier(otherMobIndex, towerBullets[bulletIndex].modifier);
                    }
                }
            }
        }
    }
}

void towers_update(float deltaTime) {
    updateTowers(deltaTime);
    updateBullets(deltaTime);
}

// ------
// DRAW -

void drawTower(TowerType type, Vector2 towerCenter) {
    int towerWidth = 16;
    int towerHeight = 8;

    Color c = towerTypeData.color[type];

    DrawEllipse(towerCenter.x, towerCenter.y, towerWidth, towerHeight, c);

    if (gameplayMode == GAMEPLAY_MODE_TOWER_REMOVE) {
        c = (Color){204, 67, 57, 100};
        DrawEllipse(towerCenter.x, towerCenter.y, towerWidth, towerHeight, c);
    }
}

void drawRangeIndicator(float range, int towerX, int towerY) {
    float scaledTowerRange = getScaledTowerRange(range);
    Vector2 rangeIndicatorCenter = grid_getTileCenter(SCENE_TRANSFORM, towerX, towerY);

    DrawEllipse(rangeIndicatorCenter.x,
        rangeIndicatorCenter.y,
        scaledTowerRange,
        scaledTowerRange / 2,
        (Color){100, 255, 100, 10});

    for (int j = 0; j < 3; j++) {
        DrawEllipseLines(rangeIndicatorCenter.x,
            rangeIndicatorCenter.y,
            scaledTowerRange - j,
            (scaledTowerRange - j) / 2,
            (Color){40, 90, 40, 60});
    }
}

void drawTowerToPlace() {
    // TODO: change
    Vector2 m = input.worldMousePos;
    V2i coords = grid_worldPointToCoords(SCENE_TRANSFORM, m.x, m.y);
    if (grid_isValidCoords(SCENE_DATA->cols, SCENE_DATA->rows, coords.x, coords.y)) {
        Vector2 tileCenter = grid_getTileCenter(SCENE_TRANSFORM, coords.x, coords.y);
        drawTower(towerToPlaceType, tileCenter);
        drawRangeIndicator(towerTypeData.range[towerToPlaceType], coords.x, coords.y);
    }
}

void drawTowerRayToTarget(Vector2 towerPos, int mobIndex) {
    Vector2 mobPos = wave_mob_getPosition(mobIndex);
    DrawLine(towerPos.x, towerPos.y, mobPos.x, mobPos.y, YELLOW);
}

void drawTowerTarget(Vector2 tileCenter, int mobIndex) {
    snprintf(buffer, sizeof(buffer), "%d", mobIndex);
    DrawText(buffer, tileCenter.x - 8, tileCenter.y - 30, 16, BLACK);
}

void drawBullet(const TowerBullet *bullet) {
    DrawCircle(bullet->position.x,
        bullet->position.y,
        towerTypeData.bulletWidth[bullet->originTowerType],
        towerTypeData.bulletColor[bullet->originTowerType]);
}

void towers_draw() {
    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        if (!towersPool[i].onScene) {
            continue;
        }

        V2i towerCoords = towersPool[i].coords;
        Vector2 tileCenter = grid_getTileCenter(SCENE_TRANSFORM, towerCoords.x, towerCoords.y);
        TowerType type = towersPool[i].type;
        drawTower(type, tileCenter);

        if (gameplay_drawInfo) {
            int mobIndex = towersPool[i].currentTargetMobIndex;
            drawRangeIndicator(towerTypeData.range[type], towerCoords.x, towerCoords.y);
            drawTowerTarget(tileCenter, mobIndex);

            if (wave_mob_isAlive(mobIndex)) {
                drawTowerRayToTarget(tileCenter, mobIndex);
            }
        }
    }

    if (gameplayMode == GAMEPLAY_MODE_TOWER_PLACE) {
        drawTowerToPlace();
    }

    for (int i = 0; i < SCENE_MAX_BULLETS; i++) {
        if (!towerBullets[i].alive) {
            continue;
        }

        drawBullet(&towerBullets[i]);
    }
}
