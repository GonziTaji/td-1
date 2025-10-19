#include "./scene_data.h"
#include "../../utils/utils.h"
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

        switch (line[0]) {
        case 'G': { // Grid
            int scanResponse = sscanf(line, "G %d %d", &data.cols, &data.rows);

            if (scanResponse != 2) {
                printf("Error parsing grid on line %d: %s\n", lineNumber, line);
            }
        } break;

        case 'P': { // Waypoint
            if (data.pathWaypointsCount >= SCENE_DATA_MAX_WAYPOINTS) {
                printf("Skipping waypoint on line %d because max waypoints reached", lineNumber);
                break;
            }

            V2i *p = &data.pathWaypoints[data.pathWaypointsCount];
            int scanResponse = sscanf(line, "P %d %d", &p->x, &p->y);

            if (scanResponse == 2) {
                data.pathWaypointsCount++;
            } else {
                printf("Error parsing waypoint on line %d: %s\n", lineNumber, line);
            }
        } break;

        case 'W': { // Wave
            if (data.wavesCount >= SCENE_DATA_MAX_WAVES) {
                printf("Skipping wave on line %d because max waves reached", lineNumber);
                break;
            }

            WaveData *w = &data.waves[data.wavesCount];
            int scanResponse
                = sscanf(line, "W %d %d %f", &w->mobsCount, &w->mobMaxHealth, &w->mobMovementSpeed);

            if (scanResponse == 4) {
                data.wavesCount++;
            } else {
                printf("Error parsing wave on line %d: %s\n", lineNumber, line);
            }
        } break;

        default:
            printf("Línea desconocida (%d): %s\n", lineNumber, line);
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
        data.waves[i] = (WaveData){0, 0, 0};
    }

    for (int i = 0; i < SCENE_DATA_MAX_WAYPOINTS; i++) {
        data.pathWaypoints[i] = (V2i){0, 0};
    }

    char path[SCENE_DATA_MAX_FILE_PATH];

    snprintf(path, sizeof(path), "%s/scene_%d.txt", SCENE_DATA_FILE_DIR, sceneIndex);

    printf("Loading scene data from \"%s\"\n", path);

    parseSceneFile(path);
}
