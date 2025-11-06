#include "ui_button_grid.h"
#include "../core/asset_manager.h"
#include "raylib.h"
#include <assert.h>

Rectangle getButtonBounds(UIButtonGrid *bp, int index) {
    int col = index % bp->cols;
    int row = index / bp->cols;
    float x = bp->origin.x + bp->padding.x + col * (bp->buttonDimensions.x + bp->buttonSpacing.x);
    float y = bp->origin.y + bp->padding.y + row * (bp->buttonDimensions.y + bp->buttonSpacing.y);

    return (Rectangle){x, y, bp->buttonDimensions.x, bp->buttonDimensions.y};
}

// to avoid values not synchronized
int uiButtonGrid_getButtonsCount(UIButtonGrid *grid) {
    int buttonsCount = grid->cols * grid->rows;

    assert(buttonsCount <= UI_BUTTON_PANNEL_MAX_BUTTONS);

    return buttonsCount;
}

int uiButtonGrid_getFullWidth(UIButtonGrid *grid) {
    return ((grid->padding.x * 2) + ((grid->cols - 1) * grid->buttonSpacing.x)
            + (grid->cols * grid->buttonDimensions.x));
}

int uiButtonGrid_getFullHeight(UIButtonGrid *grid) {
    return ((grid->padding.y * 2) + ((grid->rows - 1) * grid->buttonSpacing.y)
            + (grid->rows * grid->buttonDimensions.y));
}

void uiButtonGrid_init(UIButtonGrid *bp,
    int cols,
    int rows,
    Vector2i buttonDimensions,
    Vector2i buttonSpacing,
    Vector2i padding,
    Vector2i origin) {

    bp->cols = cols;
    bp->rows = rows;
    bp->buttonDimensions = buttonDimensions;
    bp->buttonSpacing = buttonSpacing;
    bp->padding = padding;
    bp->origin = origin;
    bp->activeButtonIndex = -1;
    bp->hoveredButtonIndex = -1;
}

int getButtonIndexFromPoint(UIButtonGrid *bn, Vector2 point) {
    int maxX = bn->origin.x + uiButtonGrid_getFullWidth(bn);
    int maxY = bn->origin.y + uiButtonGrid_getFullHeight(bn);

    // Not in panel
    if (point.x < bn->origin.x || point.y < bn->origin.y || point.x > maxX || point.y > maxY) {
        return -1;
    }

    const float dx = point.x - bn->origin.x;
    const float dy = point.y - bn->origin.y;

    int gridCoordsX = dx / bn->buttonDimensions.x;
    int gridCoordsY = dy / bn->buttonDimensions.y;

    return gridCoordsX + (gridCoordsY * gridCoordsX);
}

Message uiButtonGrid_processInput(UIButtonGrid *bn, InputManager *input) {
    Message msg = {MESSAGE_NONE};

    bn->hoveredButtonIndex = getButtonIndexFromPoint(bn, input->worldMousePos);

    if (bn->hoveredButtonIndex != -1 && bn->hoveredButtonIndex < uiButtonGrid_getButtonsCount(bn)) {
        if (input->mouseButtonState[MOUSE_BUTTON_LEFT] == MOUSE_BUTTON_STATE_PRESSED) {
            msg = bn->buttons[bn->hoveredButtonIndex].command;
            bn->activeButtonIndex = bn->hoveredButtonIndex;
        }
    }

    return msg;
}

int uiButtonGrid_getWidth(UIButtonGrid *bp) {
    return (bp->cols * bp->buttonDimensions.x) + ((bp->cols - 1) * bp->buttonSpacing.x)
         + (bp->padding.x * 2);
}

int uiButtonGrid_getHeight(UIButtonGrid *bp) {
    return (bp->rows * bp->buttonDimensions.y) + ((bp->rows - 1) * bp->buttonSpacing.y)
         + (bp->padding.y * 2);
}

void uiButtonGrid_draw(UIButtonGrid *bp, int fontSize) {
    DrawRectangle(
        bp->origin.x, bp->origin.y, uiButtonGrid_getWidth(bp), uiButtonGrid_getHeight(bp), BLACK);

    const int buttonsCount = uiButtonGrid_getButtonsCount(bp);

    for (int i = 0; i < buttonsCount; i++) {
        Color buttonColor;

        if (i == bp->activeButtonIndex) {
            buttonColor = WHITE;
        } else if (i == bp->hoveredButtonIndex) {
            buttonColor = LIGHTGRAY;
        } else {
            buttonColor = GRAY;
        }

        Rectangle bounds = getButtonBounds(bp, i);

        DrawRectangleRec(bounds, buttonColor);

        if (bp->buttons[i].type == BUTTON_TYPE_SPRITE) {
            Rectangle dest = {
                bounds.x,
                bounds.y,
                bounds.width,
                bounds.height,
            };

            if (bp->buttons[i].content.icon.sourceRect.width
                > bp->buttons[i].content.icon.sourceRect.height) {

                dest.height = dest.width * bp->buttons[i].content.icon.sourceRect.height
                            / bp->buttons[i].content.icon.sourceRect.width;

            } else {
                dest.width = dest.height * bp->buttons[i].content.icon.sourceRect.width
                           / bp->buttons[i].content.icon.sourceRect.height;
            }

            // center sprite in button
            dest.x = bounds.x + (bounds.width - dest.width) / 2.0f;
            dest.y = bounds.y + (bounds.height - dest.height) / 2.0f;

            // Vector2 pivot = {dest.width / 2, dest.height};
            Vector2 pivot = {0, 0};

            DrawTexturePro(bp->buttons[i].content.icon.sprite,
                bp->buttons[i].content.icon.sourceRect,
                dest,
                pivot,
                0,
                WHITE);

        } else {
            Vector2 textSize = MeasureTextEx(uiFont, bp->buttons[i].content.label, fontSize, 0);
            Vector2 textPos = {
                bounds.x + (bounds.width - textSize.x) / 2,
                bounds.y + (bounds.height - textSize.y) / 2,
            };

            DrawTextEx(uiFont, bp->buttons[i].content.label, textPos, fontSize, 0, BLACK);
        }
    }
}
