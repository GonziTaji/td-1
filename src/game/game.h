#pragma once

#include "gameplay.h"
#include <raylib.h>

#define GAME_VIEW_WIDTH 1920

typedef struct Game {
    float scale;
    GameplaySpeed gameplaySpeed;
} Game;

void game_init(Game *game);
void game_processInput(Game *game);
void game_update(Game *game, float deltaTime);
const Texture *const game_Render(Game *game);

#ifdef ENABLE_EDITOR
Vector2 game_ToScreen(const Game *game, Vector2 gamePos);
Vector2 game_ToGame(const Game *game, Vector2 screenPos);
#endif
