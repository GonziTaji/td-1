#include "../../utils/utils.h"

#define SCENE_DATA_MAX_MOBS 1024
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
