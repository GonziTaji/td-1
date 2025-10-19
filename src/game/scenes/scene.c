#include "scene.h"
#include "../../core/asset_manager.h"
#include "../../input/input.h"
#include "../../utils/grid.h"
#include "../constants.h"
#include "../gameplay.h"
#include "./towers_manager.h"
#include "./view_mamanger.h"
#include "./wave_manager.h"
#include <assert.h>
#include <float.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>

int hoveredTileIndex = -1;
const int *const scene_hoveredTileIndex = &hoveredTileIndex;

void drawIsoRecLines(IsoRec isoRec, Color color) {
    DrawLineEx(isoRec.top, isoRec.left, 2, color);
    DrawLineEx(isoRec.left, isoRec.bottom, 2, color);
    DrawLineEx(isoRec.bottom, isoRec.right, 2, color);
    DrawLineEx(isoRec.right, isoRec.top, 2, color);
}

void scene_init() {
    //
}

void scene_handleInput() {
    V2i hoveredCoords
        = grid_worldPointToCoords(SCENE_TRANSFORM, input.worldMousePos.x, input.worldMousePos.y);

    hoveredTileIndex
        = grid_getTileIndexFromCoords(SCENE_COLS, SCENE_ROWS, hoveredCoords.x, hoveredCoords.y);

    view_handleInput();
    towers_handleInput();

    if (input.keyPressed == KEY_SPACE) {
        wave_start(40);
    }

    if (input.keyPressed == KEY_LEFT_ALT) {
        gameplay_drawInfo = !gameplay_drawInfo;
    }
}

void scene_update(float deltaTime) {
    view_update();
    wave_update(deltaTime);
    towers_update(deltaTime);
}

void scene_draw() {
    // draw tiles
    for (int i = 0; i < SCENE_TILE_COUNT; i++) {
        V2i coords = grid_getCoordsFromTileIndex(SCENE_COLS, i);
        IsoRec tile = grid_toIsoRec(SCENE_TRANSFORM, coords, (V2i){1, 1});

        DrawTexturePro(slab1Texture,
            (Rectangle){0, 0, slab1Texture.width, slab1Texture.height},
            (Rectangle){
                tile.left.x,
                tile.top.y,
                TILE_WIDTH * SCENE_TRANSFORM->scale,
                TILE_HEIGHT * SCENE_TRANSFORM->scale,
            },
            (Vector2){0, 0},
            0,
            WHITE);
    }

    towers_draw();
    wave_draw();

    // draw hovered indicator
    if (hoveredTileIndex != -1) {
        V2i coords = grid_getCoordsFromTileIndex(SCENE_COLS, hoveredTileIndex);
        IsoRec tile = grid_toIsoRec(SCENE_TRANSFORM, coords, (V2i){1, 1});
        drawIsoRecLines(tile, BROWN);
    }
}
