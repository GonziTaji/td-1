#pragma once

extern const int *const scene_hoveredTileIndex;

void scene_init();
void scene_handleInput();
void scene_update(float deltaTime);
void scene_draw();
