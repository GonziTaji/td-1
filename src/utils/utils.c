#include "utils.h"
#include "../game/constants.h"
#include <assert.h>
#include <math.h>
#include <raylib.h>
#include <string.h>

#ifdef ENABLE_EDITOR

static const EnumJsonMapping duration_type_mapping[] = {
    {DURATION_TYPE_TEMPORARY, "TEMPORAL"},
    {DURATION_TYPE_PERMANENT, "PERMANENT"},
};
static int duration_type_mapping_count = MAPPING_COUNT(duration_type_mapping);

static const EnumJsonMapping value_type_mapping[] = {
    {MOD_VALUE_TYPE_FLAT, "FLAT"},
    {MOD_VALUE_TYPE_FLAT, "MULTIPLIER"},
};
static int value_type_mapping_count = MAPPING_COUNT(duration_type_mapping);

#endif

float utils_clampf(float min, float max, float value) {
    if (min > value) {
        return min;
    }

    if (max < value) {
        return max;
    }

    return value;
}

bool utils_checkCollisionPointEllipse(Vector2 point,
    Vector2 ellipseCenter,
    float ellipseRadiusX,
    float ellipseRadiusY) {

    float term1 = powf(point.x - ellipseCenter.x, 2) / powf(ellipseRadiusX, 2);
    float term2 = powf(point.y - ellipseCenter.y, 2) / powf(ellipseRadiusY, 2);

    return (term1 + term2 <= 1.0f);
}

Rectangle Vector2ToRec(Vector2 pos, Vector2 size) {
    return (Rectangle){pos.x, pos.y, size.x, size.y};
}

Vector2 RectangleGetPosition(Rectangle rec) {
    return (Vector2){rec.x, rec.y};
}

Vector2 RectangleGetSize(Rectangle rec) {
    return (Vector2){rec.width, rec.height};
}

#ifdef ENABLE_EDITOR

char *utils_GetDurationTypeLabel(DurationType type) {

    assert(false && "Invalid tower attribute type");
}

char **utils_GetAllDurationTypeLabels() {
    static char *duration_labels[DURATION_TYPE_COUNT];

    for (DurationType type = 0; type < DURATION_TYPE_COUNT; type++) {
        switch (type) {
        case DURATION_TYPE_TEMPORARY:
            duration_labels[type] = "Temporal";
            break;

        case DURATION_TYPE_PERMANENT:
            duration_labels[type] = "Permanent";
            break;

        case DURATION_TYPE_COUNT:
            assert(false && "Invalid tower attribute type");
        }
    }

    return duration_labels;
}

char **utils_GetModValueTypeLabels() {
    static char *mod_labels[MOD_VALUE_TYPE_COUNT];

    for (DurationType type = 0; type < DURATION_TYPE_COUNT; type++) {
        switch (type) {
        case MOD_VALUE_TYPE_FLAT:
            mod_labels[type] = "Flat";
            break;

        case MOD_VALUE_TYPE_MULTIPLIER:
            mod_labels[type] = "Multiplier";
            break;

        case DURATION_TYPE_COUNT:
            assert(false && "Invalid tower attribute type");
        }
    }

    return mod_labels;
}

int utils_data_ParseEnum(const char *str_value, const EnumJsonMapping *map, int map_count) {
    for (int i = 0; i < map_count; i++) {
        if (strcmp(map[i].json_string, str_value) == 0) {
            return map[i].enum_value;
        }
    }

    assert(false && "Invalid string value for enum");
}

const char *utils_data_EnumToStr(int enum_value, const EnumJsonMapping *map, int map_count) {
    if (enum_value >= 0 && enum_value < map_count) {
        return map[enum_value].json_string;
    }

    assert(false && "Enum value out of bounds");
}

DurationType utils_ParseDurationType(const char *str) {
    return utils_data_ParseEnum(str, duration_type_mapping, duration_type_mapping_count);
}

const char *utils_DurationTypeToStr(DurationType type) {
    return utils_data_EnumToStr(type, duration_type_mapping, duration_type_mapping_count);
}

ModValueType utils_ParseModValueType(const char *str) {
    return utils_data_ParseEnum(str, value_type_mapping, value_type_mapping_count);
}

const char *utils_ModValueTypeToStr(ModValueType type) {
    return utils_data_EnumToStr(type, value_type_mapping, value_type_mapping_count);
}

#endif
