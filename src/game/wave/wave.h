#pragma once
#include "../combat/status_effects_data.h"
#include <raylib.h>
#include <stdbool.h>

// utils
int wave_getMobCount();

// utils - mob
void wave_mob_removeStatusEffect(int mobIndex, int modifierId);
void wave_mob_addStatusEffect(int mob_index, StatusEffect modifier_data);
int wave_mob_isAlive(int mobIndex);
Vector2 wave_mob_getPosition(int mobIndex);
void wave_mob_takeDamage(int mobIndex, int damage);
float wave_mob_getPercentajeTraveled(int mobIndex);

// utils - path
bool wave_isPath(int tileX, int tileY);

// lifecycle
void wave_initData();
void wave_startNext();
void wave_update(float deltaTime);

// draw
void wave_draw();
void wave_drawInfo();

#ifdef ENABLE_EDITOR

void wave_StopWaves();

#endif
