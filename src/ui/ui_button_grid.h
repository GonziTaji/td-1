#pragma once
#include "../input/input.h"
#include "button.h"

// to avoid dynamic array + malloc. The memory "wasted" is negligible
#define UI_BUTTON_PANNEL_MAX_BUTTONS 20

typedef struct {
    int x, y;
} Vector2i;

typedef struct {
    // config
    int rows, cols;
    Vector2i buttonDimensions, buttonSpacing, padding, origin;
    // buttons
    UIButton buttons[UI_BUTTON_PANNEL_MAX_BUTTONS];
    int hoveredButtonIndex;
    int activeButtonIndex;
} UIButtonGrid;

int uiButtonGrid_getFullWidth(UIButtonGrid *grid);
int uiButtonGrid_getButtonsCount(UIButtonGrid *grid);
int uiButtonGrid_getFullHeight(UIButtonGrid *grid);
void uiButtonGrid_updateDimensions(UIButtonGrid *bp, int cols, int rows);
void uiButtonGrid_init(UIButtonGrid *bp,
    int cols,
    int rows,
    Vector2i buttonDimensions,
    Vector2i buttonSpacing,
    Vector2i padding,
    Vector2i origin);
Message uiButtonGrid_processInput(UIButtonGrid *bn, InputManager *input);
void uiButtonGrid_draw(UIButtonGrid *bp, int fontSize);
int uiButtonGrid_getWidth(UIButtonGrid *bp);
int uiButtonGrid_getHeight(UIButtonGrid *bp);
