#include "./scene_data.h"
#include "../../utils/utils.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SCENE_DATA_FILE_DIR "resources/scenes_data"
#define SCENE_DATA_MAX_FILE_PATH SCENE_DATA_NAME_MAX_LENGTH + sizeof(SCENE_DATA_FILE_DIR)

SceneData data = {
    .name = "",
    .cols = 0,
    .rows = 0,
    .pathWaypointsCount = 0,
    .pathWaypoints = {},
    .wavesCount = 0,
    .waves = {},
};

const SceneData *const SCENE_DATA = &data;

void parseSceneFile(const char *path) {
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
            assert(data.pathWaypointsCount <= SCENE_DATA_MAX_WAYPOINTS
                   && "Scene data with too many waypoints");

            V2i *p = &data.pathWaypoints[data.pathWaypointsCount];
            int scanResponse = sscanf(line, "P %d %d", &p->x, &p->y);

            assert(scanResponse == 2 && "Line failed to be parsed. Missing values?");

            data.pathWaypointsCount++;
        } break;

        case 'W': { // Wave
            assert(data.wavesCount <= SCENE_DATA_MAX_WAVES && "Scene data with too many waves");

            WaveData *w = &data.waves[data.wavesCount];
            int scanResponse = sscanf(
                line, "W %d %d %d", &w->startDelaySeconds, (int *)(&w->mobType), &w->mobsCount);

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

void scene_data_load(int sceneIndex) {
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

    snprintf(path, sizeof(path), "%s/scene_%d.txt", SCENE_DATA_FILE_DIR, sceneIndex);

    printf("Loading scene data from \"%s\"\n", path);

    parseSceneFile(path);
}
