#include "../../input/input.h"
#include "../../utils/grid.h"
#include "../../utils/utils.h"
#include "../constants.h"
#include "../gameplay.h"
#include "./scene_data.h"
#include "./view_mamanger.h"
#include "wave_manager.h"
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>

#define SCENE_MAX_TOWERS 20
#define SCENE_MAX_BULLETS 1024

static char buffer[16];

GameplayMode gameplayMode = GAMEPLAY_MODE_NORMAL;

typedef struct {
    bool onScene;
    V2i coords;
    int currentTargetMobIndex;
} Tower;

Tower towersPool[SCENE_MAX_TOWERS];
float towersTimeSinceLastShot[SCENE_MAX_TOWERS];

// Bullets per second
float TOWER_RATE_OF_FIRE = 3.0f;
float TOWER_RANGE = 60.0f * SCENE_SCALE_INITIAL;

const int BULLET_SPEED = 800; // pixels per second
const int BULLET_DAMAGE = 14;

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
int getTowerTarget(Vector2 towerPosition) {
    int mobCount = wave_getMobCount();

    int mostTraveled = 0;
    int targetIndex = -1;

    for (int i = 0; i < mobCount; i++) {
        if (!wave_mob_isAlive(i)) {
            continue;
        }

        if (!isInRange(i, towerPosition, TOWER_RANGE)) {
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
        tower->onScene = true;
        tower->coords.x = x;
        tower->coords.y = y;
        tower->currentTargetMobIndex = -1;

        // will shoot as soon as it has a target
        towersTimeSinceLastShot[firstAvailableIndex] = 1.0f / TOWER_RATE_OF_FIRE;

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
        towersTimeSinceLastShot[i] = 0;
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
}

void updateTowers(float deltaTime) {
    float towerSecondsPerBullet = 1.0f / TOWER_RATE_OF_FIRE;

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

        if (!wave_mob_isAlive(mobIndex)) {
            towersPool[i].currentTargetMobIndex = getTowerTarget(towerPos);
            continue;
        }

        if (!isInRange(mobIndex, towerPos, TOWER_RANGE)) {
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

// --------
// UPDATE -

void updateBullets(float deltaTime) {
    for (int i = 0; i < SCENE_MAX_BULLETS; i++) {
        if (!towerBullets[i].alive) {
            continue;
        }

        Vector2 originPos = grid_getTileCenter(SCENE_TRANSFORM,
            towerBullets[i].originTowerCoords.x,
            towerBullets[i].originTowerCoords.y);

        Vector2 targetPos = wave_mob_getPosition(towerBullets[i].mobTargetIndex);

        float scaledBulletSpeed = BULLET_SPEED * SCENE_TRANSFORM->scale / SCENE_SCALE_INITIAL;
        float distance = Vector2Distance(originPos, targetPos);
        float dt = (scaledBulletSpeed * deltaTime) / distance;

        towerBullets[i].travelProgress += dt;
        towerBullets[i].travelProgress = Clamp(towerBullets[i].travelProgress, 0, 1);

        if (towerBullets[i].travelProgress == 1) {
            wave_mob_takeDamage(towerBullets[i].mobTargetIndex, BULLET_DAMAGE);
            towerBullets[i].alive = false;
        }

        towerBullets[i].position
            = Vector2Lerp(originPos, targetPos, towerBullets[i].travelProgress);
    }
}

void towers_update(float deltaTime) {
    updateTowers(deltaTime);
    updateBullets(deltaTime);
}

// ------
// DRAW -

void drawTower(Vector2 towerCenter) {
    Color towerColor
        = gameplayMode == GAMEPLAY_MODE_TOWER_REMOVE ? (Color){186, 54, 45, 255} : WHITE;

    DrawEllipse(towerCenter.x, towerCenter.y, 16, 8, towerColor);
}

void drawRangeIndicator(int towerX, int towerY) {
    float scaledTowerRange = getScaledTowerRange(TOWER_RANGE);
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
    Vector2 m = input.worldMousePos;
    V2i coords = grid_worldPointToCoords(SCENE_TRANSFORM, m.x, m.y);
    if (grid_isValidCoords(SCENE_DATA->cols, SCENE_DATA->rows, coords.x, coords.y)) {
        Vector2 tileCenter = grid_getTileCenter(SCENE_TRANSFORM, coords.x, coords.y);
        drawTower(tileCenter);
        drawRangeIndicator(coords.x, coords.y);
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

void drawBullet(Vector2 bulletPos) {
    DrawCircle(bulletPos.x, bulletPos.y, 4, YELLOW);
}

void towers_draw() {
    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        if (!towersPool[i].onScene) {
            continue;
        }

        V2i towerCoords = towersPool[i].coords;
        Vector2 tileCenter = grid_getTileCenter(SCENE_TRANSFORM, towerCoords.x, towerCoords.y);
        drawTower(tileCenter);

        if (gameplay_drawInfo) {
            int mobIndex = towersPool[i].currentTargetMobIndex;
            drawRangeIndicator(towerCoords.x, towerCoords.y);
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

        drawBullet(towerBullets[i].position);
    }
}
