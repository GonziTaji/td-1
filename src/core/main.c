#include "../game/game.h"
#include "asset_manager.h"
#include "raylib.h"

#define TARGET_FPS 144
#define MIN_FPS 30

int main(void) {
    SetTargetFPS(TARGET_FPS);

    Vector2 windowSize = {0, 0};

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(windowSize.x, windowSize.y, "raylib_game td-1");
    SetExitKey(KEY_NULL);

    assetManager_loadAssets();

    Game g;
    game_init(&g);

    /*
     * MAIN LOOP
     */
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        if (deltaTime > 1.0f / MIN_FPS) {
            float newDeltaTime = 1.0f / MIN_FPS;

            TraceLog(LOG_WARNING,
                "Frame took %.2fms. Clamping to %.2fms for smooth movement",
                deltaTime * 1000.0f,
                newDeltaTime * 1000.0f);

            deltaTime = newDeltaTime;
        }

        game_processInput(&g);
        game_update(&g, deltaTime);
        game_draw(&g);
    }

    // Should we?
    assetManager_unloadAssets();

    return 0;
}
