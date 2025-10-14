#include "utils.h"
#include "../game/constants.h"
#include <assert.h>
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
