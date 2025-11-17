#pragma once

#include "../../utils/utils.h"

typedef enum {
    STATUS_EFFECT_TYPE_SLOW,
    STATUS_EFFECT_TYPE_DOT,
    STATUS_EFFECT_TYPE_STUN,
    STATUS_EFFECT_TYPE_DETONATE_ON_DEATH,
    STATUS_EFFECT_TYPE_COUNT,
} StatusEffectType;

typedef struct {
    char name[32];
    int id;
    StatusEffectType type;
    DurationType duration_type;
    ModValueType value_type;

    float value;
    float duration;
    float dot_interval;
} StatusEffect;

const StatusEffect *const status_effect_data_getDataById(int index);
bool status_effect_data_load();

#ifdef ENABLE_EDITOR

int status_effect_data_GetEffectsCount();
StatusEffect *status_effect_data_GetMutableStatusData(int effect_id);
char **status_effect_data_GetEffectTypeLabels();
int status_effect_data_RemoveStatusData(int mod_id);
int status_effect_data_CreateNewStatus();
bool status_effect_data_Save();

#endif
