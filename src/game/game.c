#include "game.h"
#include "../core/asset_manager.h"
#include "../input/input.h"
#include "../input/key_map.h"
#include "./constants.h"
#include "./scene/scene.h"
#include "gameplay.h"
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>

static RenderTexture2D target = {0};

static bool paused = false;

static void calculateGameView(Game *game) {
    float new_scale = (float)GetScreenWidth() / GAME_VIEW_WIDTH;

    Vector2 new_dimensions = (Vector2){GetScreenWidth() / new_scale, GetScreenHeight() / new_scale};

    if (game->scale == new_scale && target.texture.width == new_dimensions.x
        && target.texture.height == new_dimensions.y) {

        return;
    }

    game->scale = new_scale;

    UnloadRenderTexture(target);
    target = LoadRenderTexture(new_dimensions.x, new_dimensions.y);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
}

void game_init(Game *game) {
    game->gameplaySpeed = GAMEPLAY_SPEED_NORMAL;
    game->scale = 0;

    scene_init(1);

    calculateGameView(game);
}

void game_processInput(Game *game) {
    input_update(game->scale);

    scene_handleInput();

    // move to keymap
    if (input.keyPressed == KEY_P) {
        paused = !paused;
    }

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

    if (!paused) {
        scene_update(deltaTime);
    }
}

const Texture *const game_Render(Game *game) {
    // Draw scene in target render texture
    BeginTextureMode(target);

    ClearBackground((Color){100, 100, 100, 100});

    scene_draw();

    if (paused) {
        DrawRectangle(0, 0, target.texture.width, target.texture.height, (Color){0, 0, 0, 100});

        // to some kind of text table?
        const char *text = "Paused";
        const int fontSize = uiFont.baseSize * 2;

        Vector2 textMeasurements = MeasureTextEx(uiFont, text, fontSize, 0);
        Vector2 textPos = {
            (target.texture.width - textMeasurements.x) / 2,
            (target.texture.height - textMeasurements.y) / 2,
        };

        DrawTextEx(uiFont, text, textPos, fontSize, 0, WHITE);
    }

    EndTextureMode();

    return &target.texture;
}

#ifdef ENABLE_EDITOR
Vector2 game_ToScreen(const Game *game, Vector2 game_pos) {
    return (Vector2){
        game_pos.x * game->scale,
        game_pos.y * game->scale,
    };
}

Vector2 game_ToGame(const Game *game, Vector2 screen_point) {
    return (Vector2){
        screen_point.x / game->scale,
        screen_point.y / game->scale,
    };
}
#endif
