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

typedef enum {
    TOWER_MOD_ROF,
    TOWER_MOD_MULTUSHOT,
    TOWER_MOD_CRIT_CHANCE,
    TOWER_MOD_DAMAGE,
    TOWER_MOD_RANGE,
    TOWER_MOD_BULLET_SPEED,
} TowerModifierType;

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
    char name[32];
    TowerAttributes attributes;
    /// used to define the formula to calculate damage, range or ROF of the modifier
    ModValueType value_type;
} TowerModifier;

int tower_mod_data_getTowerTypeCount();
const TowerModifier *const tower_mod_data_getDataByIndex(int index);
bool tower_mod_data_load();

#ifdef ENABLE_EDITOR

char *tower_modifiers_data_GetAttrLabel(TowerAttributeType attribute);

#endif
