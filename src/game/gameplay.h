#pragma once

#include <stdbool.h>
typedef enum {
    GAMEPLAY_SPEED_NORMAL,
    GAMEPLAY_SPEED_FAST,
    GAMEPLAY_SPEED_FASTEST,
    GAMEPLAY_SPEED_COUNT,
} GameplaySpeed;

typedef enum {
    GAMEPLAY_MODE_NORMAL,
    GAMEPLAY_MODE_TOWER_REMOVE,
    GAMEPLAY_MODE_TOWER_PLACE,
} GameplayMode;

extern bool gameplay_drawInfo;
