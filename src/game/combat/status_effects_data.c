#include "status_effects_data.h"
#include "cJSON.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_PATH "data/status_effects.json"

typedef struct {
    StatusEffect *data;
    int count;
} StatusEffectRegistry;

static StatusEffectRegistry status_effects = {0};

static StatusEffectType parseStatusEffectType(const char *str) {
    if (!str)
        return STATUS_EFFECT_TYPE_SLOW;
    if (strcmp(str, "SLOW") == 0)
        return STATUS_EFFECT_TYPE_SLOW;
    if (strcmp(str, "DOT") == 0)
        return STATUS_EFFECT_TYPE_DOT;
    if (strcmp(str, "STUN") == 0)
        return STATUS_EFFECT_TYPE_STUN;
    if (strcmp(str, "DETONATE_ON_DEATH") == 0)
        return STATUS_EFFECT_TYPE_DETONATE_ON_DEATH;

    return STATUS_EFFECT_TYPE_SLOW;
}

static DurationType parseDurationType(const char *str) {
    if (!str)
        return DURATION_TYPE_TEMPORARY;
    if (strcmp(str, "TEMPORAL") == 0)
        return DURATION_TYPE_TEMPORARY;
    if (strcmp(str, "PERMANENT") == 0)
        return DURATION_TYPE_PERMANENT;

    return DURATION_TYPE_TEMPORARY;
}

static ModValueType parseModValueType(const char *str) {
    if (!str)
        return MOD_VALUE_TYPE_FLAT;
    if (strcmp(str, "FLAT") == 0)
        return MOD_VALUE_TYPE_FLAT;
    if (strcmp(str, "MULTIPLIER") == 0)
        return MOD_VALUE_TYPE_MULTIPLIER;

    return MOD_VALUE_TYPE_FLAT;
}

static void unloadStatusEffectsData(void) {
    free(status_effects.data);
    status_effects.data = NULL;
    status_effects.count = 0;
}

int status_effect_data_getCount() {
    return status_effects.count;
}

const StatusEffect *const status_effect_data_getDataById(int id) {
    assert(id < status_effects.count && id >= 0 && "Invalid id");

    return &status_effects.data[id];
}

bool status_effect_data_load() {
    unloadStatusEffectsData();

    FILE *f = fopen(FILE_PATH, "r");
    if (!f) {
        return false;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    char *json = malloc(len + 1);
    fread(json, 1, len, f);
    json[len] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(json);
    if (!root) {
        free(json);
        return false;
    }

    cJSON *data = cJSON_GetObjectItem(root, "data");
    if (!cJSON_IsArray(data)) {
        cJSON_Delete(root);
        free(json);
        return false;
    }

    int count = cJSON_GetArraySize(data);
    status_effects.count = count;
    status_effects.data = calloc(count, sizeof(StatusEffect));

    for (int i = 0; i < count; i++) {
        cJSON *obj = cJSON_GetArrayItem(data, i);
        StatusEffect *effect = &status_effects.data[i];

        cJSON *name = cJSON_GetObjectItem(obj, "name");
        if (cJSON_IsString(name)) {
            strncpy(effect->name, name->valuestring, sizeof(effect->name) - 1);
        }

        cJSON *type = cJSON_GetObjectItem(obj, "type");
        effect->type = parseStatusEffectType(type ? type->valuestring : NULL);

        cJSON *duration_type = cJSON_GetObjectItem(obj, "duration_type");
        effect->duration_type = parseDurationType(duration_type ? duration_type->valuestring : NULL);

        cJSON *value_type = cJSON_GetObjectItem(obj, "value_type");
        effect->value_type = parseModValueType(value_type ? value_type->valuestring : NULL);

        cJSON *item;

        item = cJSON_GetObjectItem(obj, "value");
        effect->value = item ? item->valuedouble : 0.0f;

        item = cJSON_GetObjectItem(obj, "duration");
        effect->duration = item ? item->valuedouble : 0.0f;

        item = cJSON_GetObjectItem(obj, "interval");
        effect->dot_interval = item ? item->valuedouble : 0.0f;

        effect->id = i;
    }

    cJSON_Delete(root);
    free(json);
    return true;
}
