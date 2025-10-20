#include "scene.h"
#include "../../core/asset_manager.h"
#include "../../input/input.h"
#include "../../utils/grid.h"
#include "../constants.h"
#include "../gameplay.h"
#include "./scene_data.h"
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

void scene_init(int sceneIndex) {
    scene_data_load(sceneIndex);

    towers_clear();
    wave_clear();
}

void scene_handleInput() {
    V2i hoveredCoords
        = grid_worldPointToCoords(SCENE_TRANSFORM, input.worldMousePos.x, input.worldMousePos.y);

    hoveredTileIndex = grid_getTileIndexFromCoords(
        SCENE_DATA->cols, SCENE_DATA->rows, hoveredCoords.x, hoveredCoords.y);

    if (input.keyPressed == KEY_SPACE) {
        wave_startNext();
    }

    if (input.keyPressed == KEY_LEFT_ALT) {
        gameplay_drawInfo = !gameplay_drawInfo;
    }

    // temporal
    if (input.keyPressed == KEY_F1) {
        scene_init(1);
    } else if (input.keyPressed == KEY_F2) {
        scene_init(2);
    }

    view_handleInput();
    towers_handleInput();
}

void scene_update(float deltaTime) {
    view_update();
    wave_update(deltaTime);
    towers_update(deltaTime);
}

void scene_draw() {
    // draw tiles
    int tileCount = SCENE_DATA->cols * SCENE_DATA->rows;

    for (int i = 0; i < tileCount; i++) {
        V2i coords = grid_getCoordsFromTileIndex(SCENE_DATA->cols, i);
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
        V2i coords = grid_getCoordsFromTileIndex(SCENE_DATA->cols, hoveredTileIndex);
        IsoRec tile = grid_toIsoRec(SCENE_TRANSFORM, coords, (V2i){1, 1});
        drawIsoRecLines(tile, BROWN);
    }
}
