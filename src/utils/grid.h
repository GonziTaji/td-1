#pragma once

#include "utils.h"
#include <raylib.h>
#include <stdbool.h>

// WIP
typedef struct {
    int cols;
    int rows;
    int tileWidth;
    int tileHeight;
    int tileCount;
} TileGrid;

bool grid_isValidCoords(int gridCols, int gridRows, float x, float y);

V2i grid_getCoordsFromTileIndex(int gridCols, int i);

int grid_getTileIndexFromCoords(int gridCols, int gridRows, int x, int y);

V2i grid_worldPointToCoords(const Transform2D *transform, float x, float y);

Vector2 grid_coordsToWorldPoint(const Transform2D *transform, int x, int y);

IsoRec grid_toIsoRec(const Transform2D *transform, V2i coords, V2i size);

Vector2 grid_getDistanceFromFarthestTile(int x, int y, int cols, int rows);

int grid_getZIndex(int x, int y, int cols, int rows);

Vector2 grid_getTileOrigin(const Transform2D *transform, V2i coords);

Vector2 grid_getIsoRecCenter(IsoRec isoRec);

Vector2 grid_getTileCenter(const Transform2D *transform, int x, int y);
