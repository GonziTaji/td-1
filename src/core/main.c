#include "../editor/editor.h"
#include "../game/game.h"
#include "asset_manager.h"
#include "raylib.h"
#include <stdlib.h>
#include <time.h>

#define TARGET_FPS 144
#define MIN_FPS 30

int main(void) {
    srand(time(NULL));

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

#ifdef ENABLE_EDITOR
        editor_Update(deltaTime);
#endif

        const Texture *game_texture = game_Render(&g);

        // Draw render texture in game texture
        BeginDrawing();

        ClearBackground(BLACK);

        Rectangle source = {0.0f, 0.0f, game_texture->width, -game_texture->height};
        Rectangle dest = {0, 0, GetScreenWidth(), GetScreenHeight()};
        Vector2 origin = {0, 0};

        DrawTexturePro(*game_texture, source, dest, origin, 0.0f, WHITE);

#ifdef ENABLE_EDITOR
        editor_Draw();
#endif

        EndDrawing();
    }

    // Should we?
    assetManager_unloadAssets();

    return 0;
}
