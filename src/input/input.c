#include "input.h"
#include "../core/asset_manager.h"
#include "../ui/ui_text_box.h"
#include <math.h>
#include <raylib.h>
#include <stdio.h>

InputManager input;

// maybe separate this funciton in steps like input_pollKeyboard, input_pollMouse and let the game
// decide the order of execution?
void input_update(float scale) {
    Vector2 mousePositionAbsolute = GetMousePosition();

    Vector2 newWorldMousePos = (Vector2){
        (mousePositionAbsolute.x) / scale,
        (mousePositionAbsolute.y) / scale,
    };

    input.worldMouseDelta = (Vector2){
        input.worldMousePos.x - newWorldMousePos.x,
        input.worldMousePos.y - newWorldMousePos.y,
    };

    input.worldMousePos = newWorldMousePos;

    for (int i = 0; i < MAX_MOUSE_BUTTON_TO_DETECT; i++) {
        // beautiful
        if (IsMouseButtonPressed(i)) {
            input.mouseButtonState[i] = MOUSE_BUTTON_STATE_PRESSED;

        } else if (IsMouseButtonDown(i)) {
            int threshold = 3;
            bool thresholdPassed = fabs(input.worldMouseDelta.x) > threshold
                                || fabs(input.worldMouseDelta.y) > threshold;

            if (thresholdPassed) {
                input.mouseButtonState[i] = MOUSE_BUTTON_STATE_DRAGGING;

            } else if (input.mouseButtonState[i] != MOUSE_BUTTON_STATE_DRAGGING) {
                input.mouseButtonState[i] = MOUSE_BUTTON_STATE_DOWN;
            }

        } else if (IsMouseButtonReleased(i)) {
            if (input.mouseButtonState[i] == MOUSE_BUTTON_STATE_DRAGGING) {
                input.mouseButtonState[i] = MOUSE_BUTTON_STATE_RELEASED_DRAG;

            } else {
                input.mouseButtonState[i] = MOUSE_BUTTON_STATE_RELEASED;
            }

        } else if (IsMouseButtonUp(i)) {
            input.mouseButtonState[i] = MOUSE_BUTTON_STATE_UP;
        }
    }

    // If we need more than one key pressed here, change this
    input.keyPressed = GetKeyPressed();

    input.mouseWheelMove = GetMouseWheelMove();
}

// For debug purposes
void input_drawMousePos(Vector2 screenSize) {
    UITextBox uiTextBox;
    uiTextBox_init(&uiTextBox,
        debugFont,
        debugFont.baseSize,
        (Rectangle){0, screenSize.y / 2, 300, screenSize.y / 2},
        (Vector2){10, 10});

    char buffer[64];

    snprintf(buffer, 64, "world mouse: %f, %f", input.worldMousePos.x, input.worldMousePos.y);

    uiTextBox_drawTextLine(&uiTextBox, buffer, WHITE);

    Vector2 screenMouse = GetMousePosition();

    snprintf(buffer, 64, "screen mouse: %f, %f", screenMouse.x, screenMouse.y);

    uiTextBox_drawTextLine(&uiTextBox, buffer, WHITE);

    // mouse button states
    for (int i = 0; i < MAX_MOUSE_BUTTON_TO_DETECT; i++) {
        snprintf(buffer, 64, "MOUSE BUTTON %d: %d", i, input.mouseButtonState[i]);

        uiTextBox_drawTextLine(&uiTextBox, buffer, WHITE);
    }
}
