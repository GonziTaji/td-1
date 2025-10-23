#pragma once

#include "../../utils/utils.h"

#define SCENE_DATA_MAX_MOBS 1024
#define SCENE_DATA_MAX_MOB_STAT_MODS 8
#define SCENE_DATA_MAX_WAYPOINTS 10
#define SCENE_DATA_MAX_WAVES 10

#define SCENE_DATA_NAME_MAX_LENGTH 64

typedef enum {
    DURATION_TYPE_PERMANENT,
    DURATION_TYPE_TEMPORARY,
} DurationType;

typedef enum {
    MODIFIER_EFFECT_TYPE_SLOW,
    MODIFIER_EFFECT_TYPE_DOT,
} ModifierEffectType;

typedef enum {
    MODIFIER_VALUE_TYPE_FLAT,
    MODIFIER_VALUE_TYPE_PERCENT,
    MODIFIER_VALUE_TYPE_MULTIPLIER,
} ModifierValueType;

typedef struct {
    int id;
    ModifierEffectType type;
    DurationType durationType;
    float duration;
    float value;
    ModifierValueType valueType;
} StatModifier;

typedef enum {
    MOB_TYPE_RED,
    MOB_TYPE_BLUE,
    MOB_TYPE_COUNT,
} MobType;

typedef struct {
    int startDelaySeconds;
    int mobsCount;
    MobType mobType;
} WaveData;

typedef struct {
    char name[SCENE_DATA_NAME_MAX_LENGTH];
    int cols;
    int rows;
    int pathWaypointsCount;
    V2i pathWaypoints[SCENE_DATA_MAX_WAYPOINTS];
    int wavesCount;
    WaveData waves[SCENE_DATA_MAX_WAVES];
} SceneData;

extern const SceneData *const SCENE_DATA;

void scene_data_load(int sceneIndex);
