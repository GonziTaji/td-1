#pragma once

#include <raylib.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) >= (b) ? (a) : (b))

typedef struct {
    float scale;
    Vector2 translation;
    Vector2 previousTranslation;
} Transform2D;

typedef struct {
    Vector2 left;
    Vector2 top;
    Vector2 right;
    Vector2 bottom;
} IsoRec;

typedef struct {
    int x, y;
} V2i;

// math
float utils_clampf(float min, float max, float value);

bool utils_checkCollisionPointEllipse(
    Vector2 point, Vector2 ellipseCenter, float ellipseRadiusX, float ellipseRadiusY);
