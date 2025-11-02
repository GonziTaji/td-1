#include "../../input/input.h"
#include "../../utils/grid.h"
#include "../../utils/utils.h"
#include "../constants.h"
#include "../gameplay.h"
#include "../scene/scene_data.h"
#include "../scene/view_mamanger.h"
#include "../wave/wave.h"
#include "towers_data.h"
#include <float.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char buffer[16];

GameplayMode gameplayMode = GAMEPLAY_MODE_NORMAL;

typedef struct {
    int mob_index;
    float damage;
} BulletHit;

const StatusEffect slow_effect_1 = {
    .id = 1,
    .type = STATUS_EFFECT_TYPE_SLOW,
    .duration_type = DURATION_TYPE_TEMPORARY,
    .duration = 1.0f,
    .value = -20,
    .value_type = MOD_VALUE_TYPE_MULTIPLIER,
};

const StatusEffect dot_effect_1 = {
    .id = 2,
    .type = STATUS_EFFECT_TYPE_DOT,
    .duration_type = DURATION_TYPE_TEMPORARY,
    .duration = 7.0f,
    .value = -1.0f,
    .value_type = MOD_VALUE_TYPE_FLAT,
};

Tower towersPool[SCENE_MAX_TOWERS];
int tower_to_place_idx = 0;

TowerBullet towerBullets[SCENE_MAX_BULLETS];

/// If `a` should be placed before `b`, compare function should return positive
/// value. If it should be placed after `b`, it should return negative value.
/// Returns 0 otherwise.
int compareFloats(const void *a, const void *b) {
    return (*(float *)a - *(float *)b);
}

void spawnBullet(const Tower *tower, int mobTargetIndex) {
    for (int i = 0; i < SCENE_MAX_BULLETS; i++) {
        if (towerBullets[i].alive) {
            continue;
        }

        towerBullets[i] = tower->bullet;

        towerBullets[i].alive = true;
        towerBullets[i].source_coords = tower->coords;
        towerBullets[i].mob_target_index = mobTargetIndex;
        towerBullets[i].position = grid_getTileCenter(SCENE_TRANSFORM, tower->coords.x, tower->coords.y);

        return;
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

void calculateTowerAttributes(Tower *tower) {
    tower->attributes = tower_data_getDataByIndex(tower->type_idx)->attributes;

    // Apply flat modifiers
    for (int i = 0; i < tower->tower_modifier_count; i++) {
        TowerModifier *mod = &tower->tower_modifiers[i];

        if (mod->value_type == MOD_VALUE_TYPE_FLAT) {
            for (int j = 0; j < TOWER_ATTR_COUNT; j++) {
                tower->attributes.values[j] += mod->attributes.values[j];
            }
        }
    }

    // Apply multiplier modifiers
    for (int i = 0; i < tower->tower_modifier_count; i++) {
        TowerModifier *mod = &tower->tower_modifiers[i];

        if (mod->value_type == MOD_VALUE_TYPE_MULTIPLIER) {
            for (int j = 0; j < TOWER_ATTR_COUNT; j++) {
                tower->attributes.values[j] *= mod->attributes.values[j];
            }
        }
    }
}

void calculateTowerBullet(Tower *tower) {
    TowerBaseData *base = tower_data_getDataByIndex(tower->type_idx);
    TowerAttributes *attr = &tower->attributes;

    // Init
    tower->bullet = (TowerBullet){
        .alive = false,
        .damage = attr->values[TOWER_ATTR_DAMAGE],
        .speed = attr->values[TOWER_ATTR_BULLET_SPEED],
        .color = base->bullet_color,
        .render_width = base->bullet_width,
        .attributes = {
            .aoe_range = 0,
            .aoe_falloff_multiplier = 0,
            .chain_max_bounces = 0,
            .chain_prev_mob_index = -1,
            .chain_bounce_range = 0,
            .chain_bounce_count = 0,
            .chain_bounce_multiplier = 0,
            .detonate_damage = 0,
            .detonate_range = 0,
        },
        // is set when the bullet is shot, in case the tower changes coords (not implemented)
        .source_coords = {0.0},
        .mob_target_index = -1,
        .position = {0,0},
    };

    // Add bullet modifiers
    BulletAttributes *bullet_attrs = &tower->bullet.attributes;

    for (int i = 0; i < tower->bullet_modifier_count; i++) {
        BulletModifier *mod = &tower->bullet_modifiers[i];

        bullet_attrs->aoe_range += mod->attributes.aoe_range;
        bullet_attrs->aoe_falloff_multiplier = mod->attributes.aoe_falloff_multiplier;

        bullet_attrs->chain_max_bounces += mod->attributes.chain_max_bounces;
        bullet_attrs->chain_bounce_range += mod->attributes.chain_bounce_range;
        bullet_attrs->chain_bounce_multiplier = mod->attributes.chain_bounce_multiplier;

        bullet_attrs->detonate_damage += mod->attributes.detonate_damage;
        bullet_attrs->detonate_range = mod->attributes.detonate_range;
    }

    // Add status effects
    tower->bullet.effect_count = tower->status_effect_count;
    memcpy(tower->bullet.effects, tower->status_effect, tower->status_effect_count * sizeof(StatusEffect));
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
        TowerBaseData *base = tower_data_getDataByIndex(tower_to_place_idx);

        tower->type_idx = tower_to_place_idx;
        tower->on_scene = true;
        tower->coords.x = x;
        tower->coords.y = y;
        tower->current_target_idx = -1;
        // will shoot as soon as it has a target
        tower->time_since_last_shot = 1.0f / base->attributes.rate_of_fire;

        calculateTowerAttributes(tower);
        calculateTowerBullet(tower);

        gameplayMode = GAMEPLAY_MODE_NORMAL;
    };

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
        V2i coords = grid_worldPointToCoords(SCENE_TRANSFORM, input.worldMousePos.x, input.worldMousePos.y);

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
        gameplayMode = gameplayMode != GAMEPLAY_MODE_TOWER_PLACE ? GAMEPLAY_MODE_TOWER_PLACE : GAMEPLAY_MODE_NORMAL;
    }

    if (input.keyPressed == KEY_R) {
        gameplayMode = gameplayMode != GAMEPLAY_MODE_TOWER_REMOVE ? GAMEPLAY_MODE_TOWER_REMOVE : GAMEPLAY_MODE_NORMAL;
    }

    if (gameplayMode == GAMEPLAY_MODE_TOWER_PLACE && input.keyPressed == KEY_TAB) {
        tower_to_place_idx = (tower_to_place_idx + 1) % tower_data_getTowerTypeCount();
    }
}

void updateTowers(float deltaTime) {
    for (int i = 0; i < SCENE_MAX_TOWERS; i++) {
        if (!towersPool[i].on_scene) {
            continue;
        }

        const TowerAttributes *attr = &towersPool[i].attributes;

        float towerSecondsPerBullet = 1.0f / attr->rate_of_fire;

        Vector2 towerPos = grid_getTileCenter(SCENE_TRANSFORM, towersPool[i].coords.x, towersPool[i].coords.y);

        if (towersPool[i].current_target_idx == -1) {
            towersPool[i].current_target_idx = getTowerTarget(towerPos, attr->range);
            continue;
        }

        int mobIndex = towersPool[i].current_target_idx;

        if (!wave_mob_isAlive(mobIndex)) {
            towersPool[i].current_target_idx = getTowerTarget(towerPos, attr->range);
            continue;
        }

        if (!isInRange(mobIndex, towerPos, attr->range)) {
            towersPool[i].current_target_idx = -1;
            continue;
        }

        towersPool[i].time_since_last_shot += deltaTime;

        if (towersPool[i].time_since_last_shot >= towerSecondsPerBullet) {
            // printf("Shooting after %0.2f seconds\n", towersPool[i].time_since_last_shot);
            towersPool[i].time_since_last_shot -= towerSecondsPerBullet;

            spawnBullet(&towersPool[i], towersPool[i].current_target_idx);
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

        Vector2 originPos = grid_getTileCenter(SCENE_TRANSFORM, bullet->source_coords.x, bullet->source_coords.y);

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

        BulletAttributes *attrs = &bullet->attributes;

        // check aoe
        if (attrs->aoe_range > 0) {
            float aoeSqrt = pow(attrs->aoe_range, 2);
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

                float distanceSqrt
                    = Vector2DistanceSqr(wave_mob_getPosition(damagedMobIndex), wave_mob_getPosition(otherMobIndex));

                mob_distance_sqrt[otherMobIndex] = distanceSqrt;

                if (aoeSqrt >= distanceSqrt) {
                    int distanceRatio = distanceSqrt / pow(attrs->aoe_range, 2);
                    int aoeModifier = Lerp(1, attrs->aoe_falloff_multiplier, distanceRatio);

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
        if (attrs->chain_max_bounces > 0 && attrs->chain_max_bounces < attrs->chain_bounce_count) {
            long elemSize = sizeof(mob_distance_sqrt[0]);
            long elemCount = sizeof(mob_distance_sqrt) / elemSize;

            qsort(mob_distance_sqrt, elemCount, elemSize, compareFloats);

            int in_bounce_range_count = 0;
            for (in_bounce_range_count = 0; in_bounce_range_count < elemCount; in_bounce_range_count++) {

                if (mob_distance_sqrt[in_bounce_range_count] > attrs->chain_bounce_range) {
                    break;
                }
            }

            if (in_bounce_range_count > 0) {
                bullet->mob_target_index = rand() % in_bounce_range_count;
                attrs->chain_prev_mob_index = damagedMobIndex;
                attrs->chain_bounce_count++;
            }
        }

        if (hits_count > 0) {
            bullet->alive = false;

            for (int i = 0; i < hits_count; i++) {
                wave_mob_takeDamage(hits[i].mob_index, hits[i].damage);

                // apply modifiers
                for (int modIdx = 0; modIdx < bullet->effect_count; modIdx++) {
                    wave_mob_addStatusEffect(damagedMobIndex, bullet->effects[modIdx]);
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

void drawTower(int type_id, Vector2 towerCenter) {
    int towerWidth = 16;
    int towerHeight = 8;

    Color c = tower_data_getDataByIndex(type_id)->tower_color;

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
    Vector2 m = input.worldMousePos;
    V2i coords = grid_worldPointToCoords(SCENE_TRANSFORM, m.x, m.y);

    if (grid_isValidCoords(SCENE_DATA->cols, SCENE_DATA->rows, coords.x, coords.y)) {
        Vector2 tileCenter = grid_getTileCenter(SCENE_TRANSFORM, coords.x, coords.y);
        drawTower(tower_to_place_idx, tileCenter);

        float range = tower_data_getDataByIndex(tower_to_place_idx)->attributes.range;
        drawRangeIndicator(range, coords.x, coords.y);
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

        drawTower(towersPool[i].type_idx, tileCenter);

        if (gameplay_drawInfo) {
            int mobIndex = towersPool[i].current_target_idx;
            drawTowerTarget(tileCenter, mobIndex);

            float range = tower_data_getDataByIndex(tower_to_place_idx)->attributes.range;
            drawRangeIndicator(range, towerCoords.x, towerCoords.y);

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
