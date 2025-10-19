#include "../core/asset_manager.h"
#include "../game/constants.h"
#include "../game/scenes/scene.h"
#include "../game/scenes/scene_data.h"
#include "../input/input.h"
#include "../utils/grid.h"
#include "../utils/utils.h"
#include <raylib.h>
#include <stdio.h>

const Color PANEL_BG_COLOR = {0, 0, 0, 200};
const Color PANEL_FONT_COLOR = WHITE;
const int PANEL_X = 20;
const int PANEL_Y = 20;
const int PANEL_W = 300;
const int PANEL_H = 500;
const int PANEL_FONT_SIZE = 32 * 2;
const int PANEL_FONT_LINE_HEIGHT = PANEL_FONT_SIZE + 4;
const int PANEL_MARGIN = 10;

static char buffer[64];

int cursorX = 0;
int cursorY = 0;

void cursorToNextLine() {
    cursorY += PANEL_FONT_SIZE + (PANEL_FONT_LINE_HEIGHT / 2);
}

void drawBufferAndMoveToNextLine() {
    DrawTextEx(uiFont, buffer, (Vector2){cursorX, cursorY}, PANEL_FONT_SIZE, 1, PANEL_FONT_COLOR);
    // DrawText(buffer, cursorX, cursorY, PANEL_FONT_SIZE, PANEL_FONT_COLOR);
    cursorToNextLine();
}

void writeHoveredTile() {
    V2i tileCoords = grid_getCoordsFromTileIndex(SCENE_DATA->cols, *scene_hoveredTileIndex);
    snprintf(buffer, sizeof(buffer), "[ %d, %d ]", tileCoords.x, tileCoords.y);
}

void writeMousePos() {
    Vector2 *m = &input.worldMousePos;
    snprintf(buffer, sizeof(buffer), "[ %.0f, %.0f ]", m->x, m->y);
}

void debugPanel_draw() {
    cursorX = PANEL_X + PANEL_MARGIN;
    cursorY = PANEL_Y + PANEL_MARGIN + (PANEL_FONT_LINE_HEIGHT / 2);

    DrawRectangle(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, PANEL_BG_COLOR);

    writeMousePos();
    drawBufferAndMoveToNextLine();

    writeHoveredTile();
    drawBufferAndMoveToNextLine();
}
