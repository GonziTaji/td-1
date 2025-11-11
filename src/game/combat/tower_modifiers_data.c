#include "tower_modifiers_data.h"
#include "cJSON.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_PATH "data/tower_modifiers.json"

typedef struct {
    TowerModifier *data;
    int count;
} TowerModifierRegistry;

static TowerModifierRegistry tower_modifiers = {0};

static ModValueType parseModValueType(const char *str) {
    if (!str)
        return MOD_VALUE_TYPE_FLAT;
    if (strcmp(str, "FLAT") == 0)
        return MOD_VALUE_TYPE_FLAT;
    if (strcmp(str, "MULTIPLIER") == 0)
        return MOD_VALUE_TYPE_MULTIPLIER;
    return MOD_VALUE_TYPE_FLAT;
}

static void unloadTowerModifiersData(void) {
    free(tower_modifiers.data);
    tower_modifiers.data = NULL;
    tower_modifiers.count = 0;
}

int tower_mod_data_GetModCount() {
    return tower_modifiers.count;
}

const TowerModifier *const tower_mod_data_getDataByIndex(int index) {
    assert(index < tower_modifiers.count && index >= 0 && "Invalid index");

    return &tower_modifiers.data[index];
}

bool tower_mod_data_load() {
    unloadTowerModifiersData();

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
    tower_modifiers.count = count;
    tower_modifiers.data = calloc(count, sizeof(TowerModifier));

    for (int i = 0; i < count; i++) {
        cJSON *obj = cJSON_GetArrayItem(data, i);
        TowerModifier *mod = &tower_modifiers.data[i];

        cJSON *name = cJSON_GetObjectItem(obj, "name");
        if (cJSON_IsString(name)) {
            strncpy(mod->name, name->valuestring, sizeof(mod->name) - 1);
        }

        cJSON *value_type = cJSON_GetObjectItem(obj, "value_type");
        mod->value_type = parseModValueType(value_type ? value_type->valuestring : NULL);

        cJSON *item;

        item = cJSON_GetObjectItem(obj, "damage");
        mod->attributes.damage = item ? item->valuedouble : 0.0f;

        item = cJSON_GetObjectItem(obj, "range");
        mod->attributes.range = item ? item->valuedouble : 0.0f;

        item = cJSON_GetObjectItem(obj, "rate_of_fire");
        mod->attributes.rate_of_fire = item ? item->valuedouble : 0.0f;

        item = cJSON_GetObjectItem(obj, "bullet_speed");
        mod->attributes.bullet_speed = item ? item->valuedouble : 0.0f;

        item = cJSON_GetObjectItem(obj, "multishot");
        mod->attributes.multishot = item ? item->valuedouble : 0.0f;

        item = cJSON_GetObjectItem(obj, "crit_chance_percent");
        mod->attributes.crit_chance_percent = item ? item->valuedouble : 0.0f;
    }

    cJSON_Delete(root);
    free(json);
    return true;
}

#ifdef ENABLE_EDITOR

char *tower_mod_data_GetAttrLabel(TowerAttributeType attribute) {
    switch (attribute) {
    case TOWER_ATTR_DAMAGE:
        return "Damage";
    case TOWER_ATTR_RANGE:
        return "Range";
    case TOWER_ATTR_RATE_OF_FIRE:
        return "ROF";
    case TOWER_ATTR_BULLET_SPEED:
        return "Bullet speed";
    case TOWER_ATTR_MULTISHOT:
        return "Multishot";
    case TOWER_ATTR_CRIT_CHANCE_PERCENT:
        return "Crit %";
    case TOWER_ATTR_COUNT:
        assert(false && "Invalid tower attribute type");
    }

    assert(false && "Invalid tower attribute type");
}

TowerModifier *tower_mod_data_GetMutableModData(int mod_id) {
    return &tower_modifiers.data[mod_id];
}

#endif
