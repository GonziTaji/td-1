#include "./scene_data.h"
#include "../../utils/utils.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SCENE_DATA_FILE_DIR "resources/scenes_data"
#define SCENE_DATA_MAX_FILE_PATH SCENE_DATA_NAME_MAX_LENGTH + sizeof(SCENE_DATA_FILE_DIR)

static SceneData data = {
    .name = "",
    .cols = 0,
    .rows = 0,
    .pathWaypointsCount = 0,
    .pathWaypoints = {},
    .wavesCount = 0,
    .waves = {},
};

const SceneData *const SCENE_DATA = &data;

static void parseSceneFile(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("No se pudo abrir el archivo");
        return;
    }

    char line[256];
    int lineNumber = 0;
    bool nameFound = false;

    int totalMobsCount = 0;

    while (fgets(line, sizeof(line), f)) {
        lineNumber++;

        // Remove line jump from the line
        line[strcspn(line, "\r\n")] = '\0';

        // Ignore empty lines or comments
        if (line[0] == '#' || line[0] == '\0')
            continue;

        if (!nameFound) {
            strncpy(data.name, line, sizeof(data.name) - 1);
            nameFound = true;
            continue;
        }
        // ok
        switch (line[0]) {
        case 'G': { // Grid
            int scanResponse = sscanf(line, "G %d %d", &data.cols, &data.rows);

            assert(scanResponse == 2 && "Error parsing grid line. Missing values?");
        } break;

        case 'P': { // Waypoint
            assert(data.pathWaypointsCount <= SCENE_DATA_MAX_WAYPOINTS && "Scene data with too many waypoints");

            V2i *p = &data.pathWaypoints[data.pathWaypointsCount];
            int scanResponse = sscanf(line, "P %d %d", &p->x, &p->y);

            assert(scanResponse == 2 && "Line failed to be parsed. Missing values?");

            data.pathWaypointsCount++;
        } break;

        case 'W': { // Wave
            assert(data.wavesCount <= SCENE_DATA_MAX_WAVES && "Scene data with too many waves");

            WaveData *w = &data.waves[data.wavesCount];
            int scanResponse = sscanf(line, "W %d %d %d", &w->startDelaySeconds, (int *)(&w->mobType), &w->mobsCount);

            assert(scanResponse == 3 && "Line failed to be parsed. Missing values?");
            assert(w->mobType < MOB_TYPE_COUNT && "Invalid mob type");

            totalMobsCount += w->mobsCount;

            assert(totalMobsCount <= SCENE_DATA_MAX_MOBS && "Scene data with too many mobs");

            data.wavesCount++;
        } break;

        default:
            printf("Unknown line (%d): %s\n", lineNumber, line);
        }
    }

    fclose(f);
}

static int current_scene;

void scene_data_load(int sceneIndex) {
    current_scene = sceneIndex;

    strncpy(data.name, "", sizeof(data.name));

    data.cols = 0;
    data.rows = 0;
    data.pathWaypointsCount = 0;
    data.wavesCount = 0;

    for (int i = 0; i < SCENE_DATA_MAX_WAVES; i++) {
        data.waves[i] = (WaveData){.mobsCount = 0, .mobType = 0};
    }

    for (int i = 0; i < SCENE_DATA_MAX_WAYPOINTS; i++) {
        data.pathWaypoints[i] = (V2i){0, 0};
    }

    char path[SCENE_DATA_MAX_FILE_PATH];

    snprintf(path, sizeof(path), "%s/%d/scene.txt", SCENE_DATA_FILE_DIR, sceneIndex);

    printf("Loading scene data from \"%s\"\n", path);

    parseSceneFile(path);
}

#if ENABLE_EDITOR

// General
void scene_data_ReloadCurrentScene() {
    scene_data_load(current_scene);
}

// Scene path functions
void scene_data_RemoveLastWaypoint() {
    data.pathWaypointsCount--;
}

bool scene_data_WaypointCanBeSet(V2i new_waypoint) {
    if (data.pathWaypointsCount == SCENE_DATA_MAX_WAYPOINTS) {
        // TODO: return something else like an enum to indicate max reached?
        return false;
    }

    V2i *last_path_end = &data.pathWaypoints[data.pathWaypointsCount - 1];

    const bool same_x_end = last_path_end->x == new_waypoint.x;
    const bool same_y_end = last_path_end->y == new_waypoint.y;

    if (!same_x_end && !same_y_end) {
        // It has to be a straight line
        return false;
    }

    if (same_x_end && same_y_end) {
        // It cannot be the same as the last waypoint
        return false;
    }

    V2i *last_path_start = &data.pathWaypoints[data.pathWaypointsCount - 2];

    const bool path_in_x_axis = last_path_start->x == last_path_end->x;
    const bool path_in_y_axis = last_path_start->y == last_path_end->y;

    // It cannot be a straight line towards the last path's start
    if (same_x_end && path_in_x_axis) {
        if (last_path_end->y > last_path_start->y && last_path_end->y > new_waypoint.y) {
            return false;
        }

        if (last_path_end->y < last_path_start->y && last_path_end->y < new_waypoint.y) {
            return false;
        }
    }

    // It cannot be a straight line towards the last path's start
    if (same_y_end && path_in_y_axis) {
        if (last_path_end->x > last_path_start->x && last_path_end->x > new_waypoint.x) {
            return false;
        }

        if (last_path_end->x < last_path_start->x && last_path_end->x < new_waypoint.x) {
            return false;
        }
    }

    return true;
}

bool scene_data_AddWaypoint(V2i new_waypoint) {
    if (!scene_data_WaypointCanBeSet(new_waypoint)) {
        return false;
    }

    data.pathWaypoints[data.pathWaypointsCount] = new_waypoint;
    data.pathWaypointsCount++;

    return true;
}

// Scene grid functions
void scene_data_ChangeGridDimensions(int cols, int rows) {
    data.cols = cols;
    data.rows = rows;
}

// Scene wave functions
void scene_data_AddWave(float start_delay, int mob_count, MobType mob_type) {
    data.waves[data.wavesCount] = (WaveData){
        .mobsCount = mob_count,
        .mobType = mob_type,
        .startDelaySeconds = start_delay,
    };

    data.wavesCount++;
}

void scene_data_RemoveWave(int wave_index) {
    for (int i = wave_index; i < data.wavesCount; i++) {
        data.waves[i] = data.waves[i + 1];
    }

    data.wavesCount--;
}

#endif
