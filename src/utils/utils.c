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

bool utils_checkCollisionPointEllipse(
    Vector2 point, Vector2 ellipseCenter, float ellipseRadiusX, float ellipseRadiusY) {

    float term1 = powf(point.x - ellipseCenter.x, 2) / powf(ellipseRadiusX, 2);
    float term2 = powf(point.y - ellipseCenter.y, 2) / powf(ellipseRadiusY, 2);

    return (term1 + term2 <= 1.0f);
}
