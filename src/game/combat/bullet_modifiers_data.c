#include "bullet_modifiers_data.h"
#include "cJSON.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_PATH "data/bullet_modifiers.json"

typedef struct {
    BulletModifier *data;
    int count;
    int capacity;
} BulletModifierRegistry;

static BulletModifierRegistry bullet_modifiers = {0};

void unloadBulletModifiersData(void) {
    free(bullet_modifiers.data);
    bullet_modifiers.data = NULL;
    bullet_modifiers.count = 0;
}

int bullet_mod_data_getTowerTypeCount() {
    return bullet_modifiers.count;
}

const BulletModifier *const bullet_mod_data_getDataByIndex(int index) {
    assert(index < bullet_modifiers.count && index >= 0 && "Invalid index");

    return &bullet_modifiers.data[index];
}

bool bullet_mod_data_load() {
    unloadBulletModifiersData();

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
    bullet_modifiers.count = count;
    bullet_modifiers.capacity = count;
    bullet_modifiers.data = calloc(count, sizeof(BulletModifier));

    for (int i = 0; i < count; i++) {
        cJSON *obj = cJSON_GetArrayItem(data, i);
        BulletModifier *mod = &bullet_modifiers.data[i];

        cJSON *name = cJSON_GetObjectItem(obj, "name");
        if (cJSON_IsString(name)) {
            strncpy(mod->name, name->valuestring, sizeof(mod->name) - 1);
        }

        cJSON *item;

        item = cJSON_GetObjectItem(obj, "aoe_range");
        mod->attributes.aoe_range = item ? item->valueint : 0;

        item = cJSON_GetObjectItem(obj, "aoe_falloff_multiplier");
        mod->attributes.aoe_falloff_multiplier = item ? item->valuedouble : 1.0f;

        item = cJSON_GetObjectItem(obj, "chain_max_bounces");
        mod->attributes.chain_max_bounces = item ? item->valueint : 0;

        item = cJSON_GetObjectItem(obj, "chain_bounce_range");
        mod->attributes.chain_bounce_range = item ? item->valueint : 0;

        item = cJSON_GetObjectItem(obj, "chain_bounce_multiplier");
        mod->attributes.chain_bounce_multiplier = item ? item->valuedouble : 1.0f;

        item = cJSON_GetObjectItem(obj, "detonate_damage");
        mod->attributes.detonate_damage = item ? item->valueint : 0;

        item = cJSON_GetObjectItem(obj, "detonate_range");
        mod->attributes.detonate_range = item ? item->valueint : 0;
    }

    cJSON_Delete(root);
    free(json);
    return true;
}

#ifdef ENABLE_EDITOR

int bullet_mod_data_GetModCount() {
    return bullet_modifiers.count;
}

BulletModifier *bullet_mod_data_GetMutableModData(int mod_id) {
    return &bullet_modifiers.data[mod_id];
}

int bullet_mod_data_RemoveModData(int mod_id) {
    for (int i = mod_id; i < bullet_modifiers.count; i++) {
        bullet_modifiers.data[i] = bullet_modifiers.data[i + 1];
    }

    bullet_modifiers.count--;

    return bullet_modifiers.count;
}

int bullet_mod_data_CreateNewMod() {
    if (bullet_modifiers.count >= bullet_modifiers.capacity) {
        bullet_modifiers.capacity *= 2;
        bullet_modifiers.data = realloc(bullet_modifiers.data, bullet_modifiers.capacity * sizeof(BulletModifier));
    }

    bullet_modifiers.data[bullet_modifiers.count] = (BulletModifier){.name = "Unnamed"};

    bullet_modifiers.count++;

    return bullet_modifiers.count - 1;
}

#endif
