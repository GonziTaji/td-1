#pragma once

#include "../../utils/utils.h"
#include "../combat/bullet_modifiers_data.h"
#include "../combat/status_effects_data.h"
#include "../combat/tower_modifiers_data.h"
#include <raylib.h>
#include <stdbool.h>

#define SCENE_MAX_TOWERS 20
#define SCENE_MAX_BULLETS 1024
#define TOWER_MAX_STATUS_EFFECTS 8
#define TOWER_MAX_ENHANCEMENTS 1024

typedef struct {
    // Lyfecycle
    bool alive;
    float travel_progress;
    Vector2 position;
    V2i source_coords;
    int mob_target_index;

    float damage;
    float speed;
    bool is_crit;

    BulletAttributes attributes;

    // Status effects
    int effect_count;
    StatusEffect effects[TOWER_MAX_STATUS_EFFECTS];

    // Render
    Color color;
    int render_width;
} TowerBullet;

typedef struct {
    char name[32];
    TowerAttributes attributes;

    // render
    Color tower_color;
    Color bullet_color;
    int bullet_width;
} TowerBaseData;

typedef struct {
    int type_idx;

    // Lyfecycle
    V2i coords;
    bool on_scene;
    int current_target_idx;
    float time_since_last_shot;

    // Modifiers
    int status_effect_count;
    StatusEffect status_effect[TOWER_MAX_STATUS_EFFECTS];

    int tower_modifier_count;
    TowerModifier tower_modifiers[TOWER_MAX_ENHANCEMENTS];

    int bullet_modifier_count;
    BulletModifier bullet_modifiers[TOWER_MAX_ENHANCEMENTS];

    float crit_pitty_bonus;

    /// Computed base values with modifiers applied
    TowerBullet bullet;
    TowerAttributes attributes;
} Tower;

typedef struct {
    TowerBaseData *data;
    int count;
    int capacity;
} TowerRegistry;

const TowerBaseData *const tower_data_getDataByIndex(int tower_id);
int tower_data_getTowerTypeCount();
bool tower_data_load();

#ifdef ENABLE_EDITOR

TowerBaseData *tower_data_GetMutableTowerData(int tower_id);
void tower_data_RemoveTowerData(int tower_id);
int tower_data_CreateNewTowerType();
bool tower_data_Save();

#endif
