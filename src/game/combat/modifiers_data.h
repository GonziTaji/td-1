#pragma once

#include "../../utils/utils.h"

typedef enum {
    BULLET_MOD_AOE,
    BULLET_MOD_CHAIN,
    BULLET_MOD_DETONATE_ON_DEATH,
    BULLET_MOD_COUNT
} BulletModifierType;

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
    // AOE
    int aoe_range;
    /// damage * aoe_falloff_multiplier = damage at the edge of the aoe range
    float aoe_falloff_multiplier;

    // Chain
    int chain_max_bounces;
    int chain_prev_mob_index;
    int chain_bounce_range;
    int chain_bounce_count;
    /// damage from last bounce * bounce_multiplier = damage for the next bounce
    float chain_bounce_multiplier;

    // Detonate
    int detonate_damage;
    int detonate_range;
} BulletAttributes;

typedef struct {
    int id;
    char name[32];
    BulletModifierType type;
    BulletAttributes attributes;
} BulletModifier;

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
