#pragma once

#include "../../utils/utils.h"

typedef enum {
    STATUS_EFFECT_TYPE_SLOW,
    STATUS_EFFECT_TYPE_DOT,
    STATUS_EFFECT_TYPE_STUN,
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
