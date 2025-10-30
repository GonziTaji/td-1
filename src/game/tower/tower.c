#include "../../input/input.h"
#include "../../utils/grid.h"
#include "../../utils/utils.h"
#include "../constants.h"
#include "../gameplay.h"
#include "../scene/scene_data.h"
#include "../scene/view_mamanger.h"
#include "../wave/wave.h"
#include <float.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCENE_MAX_TOWERS 20
#define SCENE_MAX_BULLETS 1024
#define TOWER_MAX_STATUS_EFFECTS 8
#define TOWER_MAX_ENHANCEMENTS 1024

static char buffer[16];

GameplayMode gameplayMode = GAMEPLAY_MODE_NORMAL;

typedef struct {
    int mob_index;
    float damage;
} BulletHit;

typedef enum {
    TOWER_TYPE_WHITE,
    TOWER_TYPE_RED,
    TOWER_TYPE_BLUE,
    TOWER_TYPE_COUNT,
} TowerType;

typedef struct {
    TowerType type;

    // base stats
    float rate_of_fire;
    float shooting_range;
    int damage;
    int bullet_speed;

    // render
    Color tower_color;
    Color bullet_color;
    int bullet_width;
} TowerBaseData;

typedef struct {
    TowerType type;

    V2i coords;
    bool on_scene;
    int current_target_idx;
    float time_since_last_shot;

    int status_effect_count;
    StatusEffect status_effect[TOWER_MAX_STATUS_EFFECTS];

    int enhancement_count;
    StatusEffect enhancements[TOWER_MAX_ENHANCEMENTS];
} Tower;

// typedef enum {
//     BULLET_STATE_INACTIVE,
//     BULLET_STATE_ALIVE,
//     BULLET_STATE_IMACTED,
// } BulletState;

typedef struct {
    bool alive;
    float travel_progress;
    Vector2 position;
    V2i source_coords;
    int mob_target_index;
    int modifier_count;
    StatusEffect effects[TOWER_MAX_STATUS_EFFECTS];
    Color color;
    int render_width;
    int damage;
    int speed;
    /// damage * aoe_falloff_multiplier = damage at the edge of the aoe range
    float aoe_falloff_multiplier;
    int aoe_range;
    /// damage from last bounce * bounce_multiplier = damage for the next bounce
    float bounce_multiplier;
    int max_bounces;
    int bounce_prev_mob_index;
    int bounce_range;
    int bounce_count;
} TowerBullet;

const TowerBaseData towerBaseData[TOWER_TYPE_COUNT] = {
    [TOWER_TYPE_WHITE] =
        {
            .rate_of_fire = 4,
            .shooting_range = 280,
            .damage = 5,
            .bullet_speed = 700,
            .tower_color = WHITE,
            .bullet_color = YELLOW,
            .bullet_width = 10,
        },
    [TOWER_TYPE_BLUE] =
        {
            .rate_of_fire = 1.2f,
            .shooting_range = 240,
            .damage = 8,
            .bullet_speed = 500,
            .tower_color = BLUE,
            .bullet_color = SKYBLUE,
            .bullet_width = 12,
        },
    [TOWER_TYPE_RED] =
        {
            .rate_of_fire = 0.6f,
            .shooting_range = 380,
            .damage = 12,
            .bullet_speed = 800,
            .tower_color = RED,
            .bullet_color = DARKPURPLE,
            .bullet_width = 8,
        },
};

const StatusEffect slow_effect_1 = {
    .id = 1,
    .type = STATUS_EFFECT_TYPE_SLOW,
    .durationType = DURATION_TYPE_TEMPORARY,
    .duration = 1.0f,
    .value = -50,
    .valueType = STATUS_EFFECT_VALUE_TYPE_PERCENT,
};

const StatusEffect dot_effect_1 = {
    .id = 2,
    .type = STATUS_EFFECT_TYPE_DOT,
    .durationType = DURATION_TYPE_TEMPORARY,
    .duration = 7.0f,
    .value = -1.0f,
    .valueType = STATUS_EFFECT_VALUE_TYPE_FLAT,
};

Tower towersPool[SCENE_MAX_TOWERS];
TowerType towerToPlaceType = TOWER_TYPE_WHITE;

TowerBullet towerBullets[SCENE_MAX_BULLETS];

/// If a should be placed before b, compare function should return positive
/// value. If it should be placed after b, it should return negative value.
/// Returns 0 otherwise.
int compareFloats(const void *a, const void *b) {
    return (*(float *)a - *(float *)b);
}

void createBullet(const Tower *tower, int mobTargetIndex) {
    for (int i = 0; i < SCENE_MAX_BULLETS; i++) {
        if (!towerBullets[i].alive) {
            towerBullets[i].alive = true;
            towerBullets[i].source_coords = tower->coords;
            towerBullets[i].travel_progress = 0;
            towerBullets[i].mob_target_index = mobTargetIndex;
            towerBullets[i].color = towerBaseData[tower->type].bullet_color;
            towerBullets[i].render_width = towerBaseData[tower->type].bullet_width;
            towerBullets[i].bounce_prev_mob_index = -1;
            towerBullets[i].bounce_range = 0;
            towerBullets[i].bounce_count = 0;
            towerBullets[i].position
                = grid_getTileCenter(SCENE_TRANSFORM, tower->coords.x, tower->coords.y);

            towerBullets[i].speed = towerBaseData[tower->type].bullet_speed;

            // TODO: apply enhancements
            towerBullets[i].damage = towerBaseData[tower->type].damage;
            towerBullets[i].aoe_range = 0;
            towerBullets[i].aoe_falloff_multiplier = 0;
            towerBullets[i].bounce_multiplier = 0;
            towerBullets[i].max_bounces = 0;

            // Add projectile modifiers
            towerBullets[i].modifier_count = tower->status_effect_count;

            memcpy(towerBullets[i].effects,
                tower->status_effect,
                tower->status_effect_count * sizeof(StatusEffect));

            printf("Created bullet from [%d, %d] to target enemy %d\n",
                tower->coords.x,
                tower->coords.y,
                mobTargetIndex);

            return;
        }
    }
}

// void createBullet(TowerType towerType, int mobTargetIndex, int x, int y) {
//     for (int i = 0; i < SCENE_MAX_BULLETS; i++) {
//         if (!towerBullets[i].alive) {
//             towerBullets[i].alive = true;
//             towerBullets[i].tower
//             towerBullets[i].travelProgress = 0;
//             towerBullets[i].mobTargetIndex = mobTargetIndex;
//             towerBullets[i].position = grid_getTileCenter(SCENE_TRANSFORM, x,
//             y); towerBullets[i].damage =
//             towerTypeData.bulletDamage[towerType]; towerBullets[i].targetType
//             = towerTypeData.bulletTargetType[towerType]; towerBullets[i].AOE
//             = towerTypeData.bulletAOE[towerType]; towerBullets[i].modifier =
//             towerTypeData.bulletModifier[towerType];
//
//             printf("Created bullet from [%d, %d] to target enemy %d\n", x, y,
//             mobTargetIndex);
//
//             return;
//         }
//     }
// }

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
/// @param `maxDistanceSqrt` - the tower range squared, to be compared to the
/// distance squared (to avoid square roots)
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
        if (firstAvailableIndex == -1 && !towersPool[i].on_scene) {
            firstAvailableIndex = i;
            continue;
        }

        if (towersPool[i].on_scene && towersPool[i].coords.x == x && towersPool[i].coords.y == y) {
            // space occupied by tower in position i
            return;
        }
    }

    if (firstAvailableIndex != -1) {
        Tower *tower = &towersPool[firstAvailableIndex];
        tower->type = towerToPlaceType;
        tower->on_scene = true;
        tower->coords.x = x;
        tower->coords.y = y;
        tower->current_target_idx = -1;
        // will shoot as soon as it has a target
        tower->time_since_last_shot = 1.0f / towerBaseData[towerToPlaceType].rate_of_fire;

        gameplayMode = GAMEPLAY_MODE_NORMAL;
    }

    // nothing happens if the tower is not set because there's no more space
}

void removeTower(int x, int y) {
    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        if (towersPool[i].coords.x == x && towersPool[i].coords.y == y) {
            towersPool[i].on_scene = false;

            return;
        }
    }
}

void towers_clear() {
    gameplayMode = GAMEPLAY_MODE_NORMAL;

    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        towersPool[i].on_scene = false;
        towersPool[i].current_target_idx = -1;
        towersPool[i].time_since_last_shot = 0;
    }

    for (int i = 0; i < SCENE_MAX_BULLETS; i++) {
        towerBullets[i].alive = false;
    }
}

void towers_handleInput() {
    if (input.mouseButtonState[MOUSE_BUTTON_LEFT] == MOUSE_BUTTON_STATE_PRESSED) {
        V2i coords = grid_worldPointToCoords(SCENE_TRANSFORM,
            input.worldMousePos.x,
            input.worldMousePos.y);

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
        if (!towersPool[i].on_scene) {
            continue;
        }

        const TowerBaseData *data = &towerBaseData[towersPool[i].type];

        float towerSecondsPerBullet = 1.0f / data->rate_of_fire;

        Vector2 towerPos
            = grid_getTileCenter(SCENE_TRANSFORM, towersPool[i].coords.x, towersPool[i].coords.y);

        if (towersPool[i].current_target_idx == -1) {
            towersPool[i].current_target_idx = getTowerTarget(towerPos, data->shooting_range);
            continue;
        }

        int mobIndex = towersPool[i].current_target_idx;

        if (!wave_mob_isAlive(mobIndex)) {
            towersPool[i].current_target_idx = getTowerTarget(towerPos, data->shooting_range);
            continue;
        }

        if (!isInRange(mobIndex, towerPos, data->shooting_range)) {
            towersPool[i].current_target_idx = -1;
            continue;
        }

        towersPool[i].time_since_last_shot += deltaTime;

        if (towersPool[i].time_since_last_shot >= towerSecondsPerBullet) {
            printf("Shooting after %0.2f seconds\n", towersPool[i].time_since_last_shot);
            towersPool[i].time_since_last_shot -= towerSecondsPerBullet;

            createBullet(&towersPool[i], towersPool[i].current_target_idx);
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

        TowerBullet *bullet = &towerBullets[bulletIndex];

        Vector2 originPos
            = grid_getTileCenter(SCENE_TRANSFORM, bullet->source_coords.x, bullet->source_coords.y);

        Vector2 targetPos = wave_mob_getPosition(bullet->mob_target_index);

        float scaledBulletSpeed = bullet->speed * SCENE_TRANSFORM->scale / SCENE_SCALE_INITIAL;
        float distance = Vector2Distance(originPos, targetPos);
        float dt = (scaledBulletSpeed * deltaTime) / distance;

        bullet->travel_progress = Clamp(dt + bullet->travel_progress, 0, 1);

        bullet->position = Vector2Lerp(originPos, targetPos, bullet->travel_progress);

        if (bullet->travel_progress != 1) {
            continue;
        }

        int hits_count = 0;

        BulletHit hits[SCENE_DATA_MAX_MOBS] = {0};

        int damagedMobIndex = bullet->mob_target_index;
        hits[hits_count] = (BulletHit){.mob_index = damagedMobIndex, .damage = bullet->damage};
        hits_count++;

        float mob_distance_sqrt[SCENE_DATA_MAX_MOBS] = {0};

        // check aoe
        if (bullet->aoe_range > 0) {
            float aoeSqrt = pow(bullet->aoe_range, 2);
            int mobCount = wave_getMobCount();

            // @performance: optimization oportunity
            for (int otherMobIndex = 0; otherMobIndex < mobCount; otherMobIndex++) {
                mob_distance_sqrt[otherMobIndex] = FLT_MAX;

                if (otherMobIndex == damagedMobIndex) {
                    continue;
                }

                if (!wave_mob_isAlive(otherMobIndex)) {
                    continue;
                }

                float distanceSqrt = Vector2DistanceSqr(wave_mob_getPosition(damagedMobIndex),
                    wave_mob_getPosition(otherMobIndex));

                mob_distance_sqrt[otherMobIndex] = distanceSqrt;

                if (aoeSqrt >= distanceSqrt) {
                    int distanceRatio = distanceSqrt / pow(bullet->aoe_range, 2);
                    int aoeModifier = Lerp(1, bullet->aoe_falloff_multiplier, distanceRatio);

                    hits[hits_count] = (BulletHit){
                        .mob_index = damagedMobIndex,
                        .damage = bullet->damage * aoeModifier,
                    };
                    hits_count++;
                }
            }
        }

        // check bounce
        // TODO: chance? or always
        if (bullet->max_bounces > 0 && bullet->max_bounces < bullet->bounce_count) {
            long elemSize = sizeof(mob_distance_sqrt[0]);
            long elemCount = sizeof(mob_distance_sqrt) / elemSize;

            qsort(mob_distance_sqrt, elemCount, elemSize, compareFloats);

            int in_bounce_range_count = 0;
            for (in_bounce_range_count = 0; in_bounce_range_count < elemCount;
                in_bounce_range_count++) {

                if (mob_distance_sqrt[in_bounce_range_count] > bullet->bounce_range) {
                    break;
                }
            }

            if (in_bounce_range_count > 0) {
                bullet->bounce_prev_mob_index = damagedMobIndex;
                bullet->mob_target_index = rand() % in_bounce_range_count;
                bullet->bounce_count++;
            }
        }

        if (hits_count > 0) {
            bullet->alive = false;

            for (int i = 0; i < hits_count; i++) {
                wave_mob_takeDamage(hits[i].mob_index, hits[i].damage);

                // apply modifiers
                for (int modIdx = 0; modIdx < bullet->modifier_count; modIdx++) {
                    wave_mob_addModifier(damagedMobIndex, bullet->effects[modIdx]);
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

    Color c = towerBaseData[type].tower_color;

    DrawEllipse(towerCenter.x, towerCenter.y, towerWidth, towerHeight, c);

    // red overlay
    if (gameplayMode == GAMEPLAY_MODE_TOWER_REMOVE) {
        c = (Color){244, 77, 67, 120};
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
        drawRangeIndicator(towerBaseData[towerToPlaceType].shooting_range, coords.x, coords.y);
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
    DrawCircle(bullet->position.x, bullet->position.y, bullet->render_width, bullet->color);
}

void towers_draw() {
    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        if (!towersPool[i].on_scene) {
            continue;
        }

        V2i towerCoords = towersPool[i].coords;
        Vector2 tileCenter = grid_getTileCenter(SCENE_TRANSFORM, towerCoords.x, towerCoords.y);
        TowerType type = towersPool[i].type;
        drawTower(type, tileCenter);

        if (gameplay_drawInfo) {
            int mobIndex = towersPool[i].current_target_idx;
            drawRangeIndicator(towerBaseData[type].shooting_range, towerCoords.x, towerCoords.y);
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
