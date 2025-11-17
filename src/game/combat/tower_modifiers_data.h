#pragma once

#include "../../utils/utils.h"

typedef enum {
    TOWER_ATTR_DAMAGE,
    TOWER_ATTR_RANGE,
    TOWER_ATTR_RATE_OF_FIRE,
    TOWER_ATTR_BULLET_SPEED,
    TOWER_ATTR_MULTISHOT,
    TOWER_ATTR_CRIT_CHANCE_PERCENT,
    TOWER_ATTR_COUNT
} TowerAttributeType;

typedef struct {
    union {
        struct {
            float damage;
            float range;
            float rate_of_fire;
            float bullet_speed;
            float multishot;
            float crit_chance_percent;
        };
        float values[TOWER_ATTR_COUNT];
    };
} TowerAttributes;

typedef struct {
    TowerAttributeType target;
    float value;
    ModValueType value_type;
} TowerModifierEntry;

typedef struct {
    char name[31];
    int entries_count;
    TowerModifierEntry entries[TOWER_ATTR_COUNT];
} TowerModifier;

int tower_mod_data_GetModCount();
const TowerModifier *const tower_mod_data_getDataByIndex(int index);
bool tower_mod_data_load();

#ifdef ENABLE_EDITOR

char *tower_mod_data_GetAttrLabel(TowerAttributeType attribute);
TowerModifier *tower_mod_data_GetMutableModData(int mod_id);
int tower_mod_data_RemoveModData(int mod_id);
int tower_mod_data_CreateNewMod();

#endif
