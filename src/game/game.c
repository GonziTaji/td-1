#include "game.h"
#include "../debug/debug_panel.h"
#include "../input/input.h"
#include "../input/key_map.h"
#include "./constants.h"
#include "./scenes/scene.h"
#include "gameplay.h"
#include <raylib.h>

#define GAME_VIEW_WIDTH 1920

RenderTexture2D target;
int previousScreenWidth;

void calculateGameView(Game *game) {
    game->scale = (float)GetScreenWidth() / GAME_VIEW_WIDTH;

    UnloadRenderTexture(target);
    target = LoadRenderTexture(GetScreenWidth() / game->scale, GetScreenHeight() / game->scale);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
}

void game_init(Game *game) {
    game->gameplaySpeed = GAMEPLAY_SPEED_NORMAL;

    scene_init(1);

    calculateGameView(game);
}

void game_processInput(Game *game) {
    input_update(game->scale);

    scene_handleInput();

    Message cmd = keyMap_processInput();
    if (messages_dispatchMessage(cmd, game)) {
        return;
    }
}

void game_update(Game *game, float deltaTime) {
    int i = GetScreenWidth() / game->scale;
    if (i != target.texture.width) {
        calculateGameView(game);
    }

    scene_update(deltaTime);
}

void game_draw(Game *game) {
    // Draw scene in target render texture
    BeginTextureMode(target);

    ClearBackground((Color){100, 100, 100, 100});

    scene_draw();

    debugPanel_draw();

    EndTextureMode();

    // Draw render texture in game texture
    BeginDrawing();

    ClearBackground(BLACK);

    Rectangle source = {
        0.0f,
        0.0f,
        target.texture.width,
        -target.texture.height,
    };

    Rectangle dest = {0, 0, GetScreenWidth(), GetScreenHeight()};

    Vector2 origin = {0, 0};

    DrawTexturePro(target.texture, source, dest, origin, 0.0f, WHITE);

    EndDrawing();
}
