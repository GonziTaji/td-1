#include "towers_data.h"
#include "cJSON.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_PATH "data/towers.json"

static TowerRegistry tower_registry = {0};

static Color parseColor(cJSON *colorArray) {
    if (!cJSON_IsArray(colorArray) || cJSON_GetArraySize(colorArray) < 4)
        return (Color){255, 255, 255, 255};

    return (Color){
        .r = cJSON_GetArrayItem(colorArray, 0)->valueint,
        .g = cJSON_GetArrayItem(colorArray, 1)->valueint,
        .b = cJSON_GetArrayItem(colorArray, 2)->valueint,
        .a = cJSON_GetArrayItem(colorArray, 3)->valueint,
    };
}

static void unload() {
    if (tower_registry.data) {
        free(tower_registry.data);
        tower_registry.data = NULL;
        tower_registry.count = 0;
    }
}

int tower_data_getTowerTypeCount() {
    return tower_registry.count;
}

const TowerBaseData *const tower_data_getDataByIndex(int index) {
    // assert?
    if (index >= tower_registry.count || index < 0) {
        return NULL;
    }

    return &tower_registry.data[index];
}

bool tower_data_load() {
    unload();

    FILE *file = fopen(FILE_PATH, "r");
    if (!file) {
        fprintf(stderr, "Could not open %s\n", FILE_PATH);
        return false;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    char *data = malloc(length + 1);
    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);

    cJSON *root = cJSON_Parse(data);
    cJSON *json_data = cJSON_GetObjectItem(root, "data");

    if (!json_data || !cJSON_IsArray(json_data)) {
        fprintf(stderr, "ERROR: JSON data must be an array. File: %s\n", FILE_PATH);
        free(data);
        return false;
    }

    int count = cJSON_GetArraySize(json_data);
    float size = sizeof(TowerBaseData) * count;

    tower_registry.count = count;
    tower_registry.capacity = count;
    tower_registry.data = malloc(size);

    for (int i = 0; i < count; i++) {
        cJSON *towerJSON = cJSON_GetArrayItem(json_data, i);
        TowerBaseData *tower = &tower_registry.data[i];

        cJSON *name = cJSON_GetObjectItem(towerJSON, "name");
        strncpy(tower->name, name ? name->valuestring : "Unnamed", sizeof(tower->name));

        cJSON *attrs = cJSON_GetObjectItem(towerJSON, "attributes");
        TowerAttributes *dest = &tower->attributes;

        if (attrs) {
            dest->damage = cJSON_GetObjectItem(attrs, "damage")->valuedouble;
            dest->range = cJSON_GetObjectItem(attrs, "range")->valuedouble;
            dest->rate_of_fire = cJSON_GetObjectItem(attrs, "rate_of_fire")->valuedouble;
            dest->bullet_speed = cJSON_GetObjectItem(attrs, "bullet_speed")->valuedouble;
            dest->multishot = cJSON_GetObjectItem(attrs, "multishot")->valuedouble;
            dest->crit_chance_percent = cJSON_GetObjectItem(attrs, "crit_chance_percent")->valuedouble;
        }

        tower->tower_color = parseColor(cJSON_GetObjectItem(towerJSON, "tower_color"));
        tower->bullet_color = parseColor(cJSON_GetObjectItem(towerJSON, "bullet_color"));
        tower->bullet_width = cJSON_GetObjectItem(towerJSON, "bullet_width")->valueint;
    }

    cJSON_Delete(json_data);
    free(data);
    return true;
}

#ifdef ENABLE_EDITOR

TowerBaseData *tower_data_GetMutableTowerData(int tower_id) {
    return &tower_registry.data[tower_id];
}

void tower_data_RemoveTowerData(int tower_id) {
    for (int i = tower_id; i < tower_registry.count; i++) {
        tower_registry.data[i] = tower_registry.data[i + 1];
    }

    tower_registry.count--;
}

int tower_data_CreateNewTowerType() {
    if (tower_registry.count >= tower_registry.capacity) {
        tower_registry.capacity *= 2;
        tower_registry.data = realloc(tower_registry.data, tower_registry.capacity * sizeof(TowerBaseData));
    }

    TowerBaseData *tower_data = &tower_registry.data[tower_registry.count];

    strncpy(tower_data->name, "Unnamed", sizeof(tower_data->name));

    tower_data->tower_color = WHITE;
    tower_data->bullet_color = WHITE;
    tower_data->bullet_width = 10;

    for (TowerAttributeType attr_type = 0; attr_type < TOWER_ATTR_COUNT; attr_type++) {
        tower_data->attributes.values[attr_type] = 0;
    }

    tower_registry.count++;

    return tower_registry.count - 1;
}

static cJSON *createColorArray(Color color) {
    cJSON *color_array = cJSON_CreateArray();
    if (!color_array) {
        return NULL;
    }

    cJSON_AddItemToArray(color_array, cJSON_CreateNumber(color.r));
    cJSON_AddItemToArray(color_array, cJSON_CreateNumber(color.g));
    cJSON_AddItemToArray(color_array, cJSON_CreateNumber(color.b));
    cJSON_AddItemToArray(color_array, cJSON_CreateNumber(color.a));

    return color_array;
}

bool tower_data_Save() {
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return false;
    }

    cJSON_AddStringToObject(root, "$schema", "./schemas/towers.schema.json");

    cJSON *data_array = cJSON_CreateArray();
    if (!data_array) {
        cJSON_Delete(root);
        return false;
    }
    cJSON_AddItemToObject(root, "data", data_array);

    for (int i = 0; i < tower_registry.count; i++) {
        TowerBaseData *tower = &tower_registry.data[i];
        cJSON *tower_obj = cJSON_CreateObject();
        if (!tower_obj) {
            cJSON_Delete(root);
            return false;
        }

        cJSON_AddStringToObject(tower_obj, "name", tower->name);

        cJSON *attrs_obj = cJSON_CreateObject();
        if (!attrs_obj) {
            cJSON_Delete(tower_obj);
            cJSON_Delete(root);
            return false;
        }

        TowerAttributes *attrs = &tower->attributes;
        cJSON_AddNumberToObject(attrs_obj, "damage", attrs->damage);
        cJSON_AddNumberToObject(attrs_obj, "range", attrs->range);
        cJSON_AddNumberToObject(attrs_obj, "rate_of_fire", attrs->rate_of_fire);
        cJSON_AddNumberToObject(attrs_obj, "bullet_speed", attrs->bullet_speed);
        cJSON_AddNumberToObject(attrs_obj, "multishot", attrs->multishot);
        cJSON_AddNumberToObject(attrs_obj, "crit_chance_percent", attrs->crit_chance_percent);

        cJSON_AddItemToObject(tower_obj, "attributes", attrs_obj);

        cJSON *tower_color_array = createColorArray(tower->tower_color);
        if (!tower_color_array) {
            cJSON_Delete(tower_obj);
            cJSON_Delete(root);
            return false;
        }
        cJSON_AddItemToObject(tower_obj, "tower_color", tower_color_array);

        cJSON *bullet_color_array = createColorArray(tower->bullet_color);
        if (!bullet_color_array) {
            cJSON_Delete(tower_obj);
            cJSON_Delete(root);
            return false;
        }
        cJSON_AddItemToObject(tower_obj, "bullet_color", bullet_color_array);
        cJSON_AddNumberToObject(tower_obj, "bullet_width", tower->bullet_width);

        cJSON_AddItemToArray(data_array, tower_obj);
    }

    char *json_string = cJSON_Print(root);
    if (!json_string) {
        cJSON_Delete(root);
        return false;
    }

    FILE *f = fopen(FILE_PATH, "w");
    if (!f) {
        free(json_string);
        cJSON_Delete(root);
        return false;
    }

    fprintf(f, "%s", json_string);
    fclose(f);

    free(json_string);
    cJSON_Delete(root);

    return true;
}

#endif
