#include "../game/game.h"
#include "asset_manager.h"
#include "raylib.h"

#define TARGET_FPS 144

int main(void) {
    SetTargetFPS(TARGET_FPS);

    Vector2 windowSize = {0, 0};

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(windowSize.x, windowSize.y, "My little plant");
    SetExitKey(KEY_NULL);

    assetManager_loadAssets();

    Game g;
    game_init(&g);

    /*
     * MAIN LOOP
     */
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        if (deltaTime > 1.0f) {
            TraceLog(LOG_WARNING, "Frame took %.2fms. Clamping to target frame time", deltaTime);
            deltaTime = 1.0f / TARGET_FPS;
        }

        game_processInput(&g);
        game_update(&g, deltaTime);
        game_draw(&g);
    }

    // Should we?
    assetManager_unloadAssets();

    return 0;
}
