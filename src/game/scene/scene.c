#include "scene.h"
#include "../../core/asset_manager.h"
#include "../../input/input.h"
#include "../../utils/grid.h"
#include "../constants.h"
#include "../gameplay.h"
#include "../tower/tower.h"
#include "../tower/towers_data.h"
#include "../wave/wave.h"
#include "./scene_data.h"
#include "./view_mamanger.h"
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

typedef enum {
    TILE_TYPE_NORMAL,
    TILE_TYPE_PATH,
} TileType;

void scene_init(int sceneIndex) {
    tower_data_load();
    bullet_mod_data_load();
    tower_mod_data_load();
    status_effect_data_load();

    scene_data_load(sceneIndex);

    towers_clear();
    wave_initData();
}

void scene_handleInput() {
    V2i hoveredCoords = grid_worldPointToCoords(SCENE_TRANSFORM, input.worldMousePos.x, input.worldMousePos.y);

    hoveredTileIndex
        = grid_getTileIndexFromCoords(SCENE_DATA->cols, SCENE_DATA->rows, hoveredCoords.x, hoveredCoords.y);

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

void drawTiles() {
    int tileCount = SCENE_DATA->cols * SCENE_DATA->rows;

    for (int i = 0; i < tileCount; i++) {
        V2i coords = grid_getCoordsFromTileIndex(SCENE_DATA->cols, i);
        IsoRec tile = grid_toIsoRec(SCENE_TRANSFORM, coords, (V2i){1, 1});

        TileType tile_type = TILE_TYPE_NORMAL;

        if (wave_isPath(coords.x, coords.y)) {
            tile_type = TILE_TYPE_PATH;
        }

        Rectangle source = {64 * tile_type, 0, 64, 32};
        Rectangle dest = {
            tile.left.x,
            tile.top.y,
            TILE_WIDTH * SCENE_TRANSFORM->scale,
            TILE_HEIGHT * SCENE_TRANSFORM->scale,
        };

        DrawTexturePro(slabsTexture, source, dest, (Vector2){0, 0}, 0, WHITE);
    }
}

void scene_draw() {
    drawTiles();
    towers_draw();
    wave_draw();

    // draw hovered indicator
    if (hoveredTileIndex != -1) {
        V2i coords = grid_getCoordsFromTileIndex(SCENE_DATA->cols, hoveredTileIndex);
        IsoRec tile = grid_toIsoRec(SCENE_TRANSFORM, coords, (V2i){1, 1});
        drawIsoRecLines(tile, BROWN);
    }
}
