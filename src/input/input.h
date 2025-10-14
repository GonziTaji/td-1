#pragma once

#include <raylib.h>

typedef enum {
    MOUSE_BUTTON_STATE_UP,
    MOUSE_BUTTON_STATE_PRESSED,
    MOUSE_BUTTON_STATE_DOWN,
    MOUSE_BUTTON_STATE_DRAGGING,
    MOUSE_BUTTON_STATE_RELEASED,
    MOUSE_BUTTON_STATE_RELEASED_DRAG,
} MouseButtonState;

// left and right
#define MAX_MOUSE_BUTTON_TO_DETECT 2

typedef struct {
    Vector2 worldMousePos;
    Vector2 worldMouseDelta;
    int mouseWheelMove;
    int mouseButtonState[MAX_MOUSE_BUTTON_TO_DETECT];
    int keyPressed;
} InputManager;

extern InputManager input;

void input_update(float scale);
void input_drawMousePos(Vector2 screenSize);
