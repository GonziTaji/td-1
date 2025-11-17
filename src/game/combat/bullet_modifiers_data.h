#pragma once

#include <stdbool.h>
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
    BulletAttributes attributes;
} BulletModifier;

int bullet_mod_data_getTowerTypeCount();
const BulletModifier *const bullet_mod_data_getDataByIndex(int index);
bool bullet_mod_data_load();

#ifdef ENABLE_EDITOR

int bullet_mod_data_GetModCount();
BulletModifier *bullet_mod_data_GetMutableModData(int mod_id);
int bullet_mod_data_RemoveModData(int mod_id);
int bullet_mod_data_CreateNewMod();
bool bullet_mod_data_Save();

#endif
