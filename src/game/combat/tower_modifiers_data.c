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
    int capacity;
} TowerModifierRegistry;

static TowerModifierRegistry tower_modifiers = {0};

static ModValueType parseModValueType(const char *str) {
    if (strcmp(str, "FLAT") == 0) {
        return MOD_VALUE_TYPE_FLAT;
    }

    if (strcmp(str, "MULTIPLIER") == 0) {
        return MOD_VALUE_TYPE_MULTIPLIER;
    }

    assert(false && "Invalid value type in tower modifiers data JSON");
}

static TowerAttributeType parseAttribute(const char *str) {
    if (strcmp(str, "DAMAGE") == 0) {
        return TOWER_ATTR_DAMAGE;
    }

    if (strcmp(str, "RANGE") == 0) {
        return TOWER_ATTR_RANGE;
    }

    if (strcmp(str, "RATE_OF_FIRE") == 0) {
        return TOWER_ATTR_RATE_OF_FIRE;
    }

    if (strcmp(str, "BULLET_SPEED") == 0) {
        return TOWER_ATTR_BULLET_SPEED;
    }

    if (strcmp(str, "MULTISHOT") == 0) {
        return TOWER_ATTR_MULTISHOT;
    }

    if (strcmp(str, "CRIT_CHANCE_PERCENT") == 0) {
        return TOWER_ATTR_CRIT_CHANCE_PERCENT;
    }

    assert(false && "Invalid attribute in tower modifiers data JSON");
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
    tower_modifiers.capacity = count;
    tower_modifiers.data = calloc(count, sizeof(TowerModifier));

    for (int i = 0; i < count; i++) {
        cJSON *obj = cJSON_GetArrayItem(data, i);
        TowerModifier *mod = &tower_modifiers.data[i];

        cJSON *name = cJSON_GetObjectItem(obj, "name");
        if (cJSON_IsString(name)) {
            strncpy(mod->name, name->valuestring, sizeof(mod->name) - 1);
        }

        cJSON *attrs_json = cJSON_GetObjectItem(obj, "attributes");

        mod->entries_count = cJSON_GetArraySize(attrs_json);

        for (int mod_entry_idx = 0; mod_entry_idx < mod->entries_count; mod_entry_idx++) {
            cJSON *mod_attr = cJSON_GetArrayItem(attrs_json, mod_entry_idx);
            cJSON *mod_attr_target = cJSON_GetObjectItem(mod_attr, "attribute");
            cJSON *mod_attr_value = cJSON_GetObjectItem(mod_attr, "value");
            cJSON *mod_attr_value_type = cJSON_GetObjectItem(mod_attr, "value_type");
            TowerAttributeType attr_type = parseAttribute(mod_attr_target->valuestring);

            mod->entries[mod_entry_idx].target = attr_type;
            mod->entries[mod_entry_idx].value = mod_attr_value->valuedouble;
            mod->entries[mod_entry_idx].value_type = parseModValueType(mod_attr_value_type->valuestring);
        }
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

int tower_mod_data_RemoveModData(int mod_id) {
    for (int i = mod_id; i < tower_modifiers.count; i++) {
        tower_modifiers.data[i] = tower_modifiers.data[i + 1];
    }

    tower_modifiers.count--;

    return tower_modifiers.count;
}

int tower_mod_data_CreateNewMod() {
    if (tower_modifiers.count >= tower_modifiers.capacity) {
        tower_modifiers.capacity *= 2;
        tower_modifiers.data = realloc(tower_modifiers.data, tower_modifiers.capacity * sizeof(TowerModifier));
    }

    tower_modifiers.data[tower_modifiers.count] = (TowerModifier){.name = "Unnamed"};

    tower_modifiers.count++;

    return tower_modifiers.count - 1;
}

#endif
