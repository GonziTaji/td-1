#pragma once
#include <raylib.h>
#include <stdbool.h>

// utils
int wave_getMobCount();
// utils - mob
int wave_mob_isAlive(int mobIndex);
Vector2 wave_mob_getPosition(int mobIndex);
void wave_mob_takeDamage(int mobIndex, int damage);
// utils - path
bool wave_isPath(int tileX, int tileY);

// lifecycle
void wave_start(int mobsCount);
void wave_update(float deltaTime);
// draw
void wave_draw();
void wave_drawInfo();
