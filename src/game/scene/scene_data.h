#pragma once

#include "../../utils/utils.h"

#define SCENE_DATA_MAX_MOBS 1024
#define SCENE_DATA_MAX_MOB_STAT_MODS 8
#define SCENE_DATA_MAX_WAYPOINTS 10
#define SCENE_DATA_MAX_WAVES 10

#define SCENE_DATA_NAME_MAX_LENGTH 64

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
    int id;
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

#ifdef ENABLE_EDITOR

void scene_data_ReloadCurrentScene();
void scene_data_RemoveLastWaypoint();
bool scene_data_WaypointCanBeSet(V2i new_waypoint);
bool scene_data_AddWaypoint(V2i new_waypoint);
void scene_data_ChangeGridDimensions(int cols, int rows);
WaveData *scene_data_GetMutableWave(int wave_index);
void scene_data_AddWave(float start_delay, int mob_count, MobType mob_type);
void scene_data_RemoveWave(int wave_index);
void scene_data_ReplaceWaypoints(V2i *waypoints, int waypoints_count);

#endif
