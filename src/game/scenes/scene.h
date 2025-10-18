#pragma once

#include "../../utils/utils.h"

// Garden dimensions
#define SCENE_COLS 16
#define SCENE_ROWS 16
#define SCENE_TILE_COUNT (SCENE_COLS * SCENE_ROWS)

extern Transform2D SCENE_TRANSFORM;
extern const int *const scene_hoveredTileIndex;

void scene_init();
void scene_handleInput();
void scene_update(float deltaTime);
void scene_draw();
