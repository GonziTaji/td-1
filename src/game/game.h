#pragma once

#include "gameplay.h"
#include <raylib.h>

typedef struct Game {
    float scale;
    GameplaySpeed gameplaySpeed;
} Game;

void game_init(Game *game);
void game_processInput(Game *game);
void game_update(Game *game, float deltaTime);
void game_draw(Game *game);
