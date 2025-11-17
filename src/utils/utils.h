#pragma once

#include <raylib.h>
#include <stddef.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) >= (b) ? (a) : (b))

#ifdef ENABLE_EDITOR

typedef struct {
    int enum_value;
    const char *json_string;
} EnumJsonMapping;

#define MAPPING_COUNT(map) (sizeof(map) / sizeof(EnumJsonMapping));

#endif

typedef enum {
    DURATION_TYPE_PERMANENT,
    DURATION_TYPE_TEMPORARY,
    DURATION_TYPE_COUNT,
} DurationType;

typedef enum {
    MOD_VALUE_TYPE_FLAT,
    MOD_VALUE_TYPE_MULTIPLIER,
    MOD_VALUE_TYPE_COUNT,
} ModValueType;

typedef struct {
    float scale;
    Vector2 translation;
    Vector2 previousTranslation;
} Transform2D;

typedef struct {
    Vector2 left;
    Vector2 top;
    Vector2 right;
    Vector2 bottom;
} IsoRec;

typedef struct {
    int x, y;
} V2i;

// math
float utils_clampf(float min, float max, float value);

bool utils_checkCollisionPointEllipse(Vector2 point, Vector2 ellipseCenter, float ellipseRadiusX, float ellipseRadiusY);

// vector utils
Rectangle Vector2ToRec(Vector2 pos, Vector2 size);

// rectangle utils
Vector2 RectangleGetPosition(Rectangle rec);
Vector2 RectangleGetSize(Rectangle rec);

#ifdef ENABLE_EDITOR

char *utils_GetDurationTypeLabel(DurationType type);
char **utils_GetAllDurationTypeLabels();
char **utils_GetModValueTypeLabels();

const char *utils_data_EnumToStr(int enum_value, const EnumJsonMapping *map, int map_count);
int utils_data_ParseEnum(const char *str_value, const EnumJsonMapping *map, int map_count);

DurationType utils_ParseDurationType(const char *str);
const char *utils_DurationTypeToStr(DurationType type);
ModValueType utils_ParseModValueType(const char *str);
const char *utils_ModValueTypeToStr(ModValueType type);

#endif
