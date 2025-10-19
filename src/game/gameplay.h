#pragma once

#include <stdbool.h>
typedef enum {
    GAMEPLAY_SPEED_NORMAL,
    GAMEPLAY_SPEED_FAST,
    GAMEPLAY_SPEED_FASTEST,
    GAMEPLAY_SPEED_COUNT,
} GameplaySpeed;

extern bool gameplay_drawInfo;
