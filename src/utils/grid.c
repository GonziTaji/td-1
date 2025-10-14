#include "grid.h"
#include "../game/constants.h"
#include "utils.h"
#include <assert.h>

bool grid_isValidCoords(int gridCols, int gridRows, float x, float y) {
    if (x < 0 || y < 0 || x >= gridCols || y >= gridRows) {
        return false;
    }

    return true;
}

V2i grid_getCoordsFromTileIndex(int gridCols, int i) {
    // should avoid division by 0? Crashes here expose bugs,
    // and should never happen (getting grid coords from a grid with 0 cols?)
    return (V2i){i % gridCols, (int)(i / gridCols)};
}

/// Returns -1 if the coords are not a valid cell of the grid
int grid_getTileIndexFromCoords(int gridCols, int gridRows, int x, int y) {
    // get this out and use it when it's really necesary? to avoid gridRows as function parameter
    if (!grid_isValidCoords(gridCols, gridRows, x, y)) {
        return -1;
    }

    return ((int)y * gridCols) + (int)x;
}

V2i grid_worldPointToCoords(Transform2D transform, float x, float y) {

    const float TW = TILE_WIDTH * transform.scale;
    const float TH = TILE_HEIGHT * transform.scale;

    const float dx = x - transform.translation.x;
    const float dy = y - transform.translation.y;

    return (V2i){
        (dx / TW) - (dy / TH),
        (dx / TW) + (dy / TH),
    };
}

Vector2 grid_getTileOrigin(Transform2D transform, V2i coords) {
    return grid_coordsToWorldPoint(transform, coords.x, coords.y);
}

Vector2 grid_coordsToWorldPoint(const Transform2D transform, int x, int y) {

    int sumX = +x + y;
    int sumY = -x + y;

    return (Vector2){
        transform.translation.x + (sumX * TILE_WIDTH * transform.scale / 2.0f),
        transform.translation.y + (sumY * TILE_HEIGHT * transform.scale / 2.0f),
    };
}

IsoRec grid_toIsoRec(const Transform2D transform, V2i coords, V2i size) {
    Vector2 left = {coords.x, coords.y};
    Vector2 top = {coords.x + size.x, coords.y};
    Vector2 right = {coords.x + size.x, coords.y + size.y};
    Vector2 bottom = {coords.x, coords.y + size.y};

    IsoRec isoRec = {
        grid_coordsToWorldPoint(transform, left.x, left.y),
        grid_coordsToWorldPoint(transform, top.x, top.y),
        grid_coordsToWorldPoint(transform, right.x, right.y),
        grid_coordsToWorldPoint(transform, bottom.x, bottom.y),
    };

    return isoRec;
}

Vector2 grid_getDistanceFromFarthestTile(int x, int y, int cols, int rows) {
    return (Vector2){cols - 1 - x, y};
}

int grid_getZIndex(int x, int y, int cols, int rows) {
    Vector2 distance = grid_getDistanceFromFarthestTile(x, y, cols, rows);
    int localCols;
    int localRows;

    localCols = cols;
    localRows = rows;

    int d = distance.x + distance.y;
    d += distance.y * localCols;
    d += distance.x * localRows;

    return d;
}

Vector2 grid_getIsoRecCenter(IsoRec isoRec) {
    return (Vector2){
        isoRec.left.x + ((isoRec.right.x - isoRec.left.x) / 2),
        isoRec.left.y + ((isoRec.right.y - isoRec.left.y) / 2),
    };
}

Vector2 grid_getTileCenter(Transform2D transform, int x, int y) {
    IsoRec tile = grid_toIsoRec(transform, (V2i){x, y}, (V2i){1, 1});

    return grid_getIsoRecCenter(tile);
}
