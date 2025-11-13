#include "utils.h"
#include "../game/constants.h"
#include <assert.h>
#include <math.h>
#include <raylib.h>

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

#endif
