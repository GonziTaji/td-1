#include "ui_text_box.h"
#include "../core/asset_manager.h"
#include "raylib.h"
#include <assert.h>

void uiTextBox_init(UITextBox *tb, Font font, int fontSize, Rectangle bounds, Vector2 padding) {
    assert(font.texture.id != 0);

    tb->bounds = bounds;
    tb->fontSize = fontSize;
    tb->font = font;
    tb->cursorPosition = (Vector2){bounds.x + padding.x, bounds.y + padding.y};
}

Message uiTextBox_processInput(UITextBox *bn, InputManager *input) {
    if (CheckCollisionPointRec(input->worldMousePos, bn->bounds)) {
        bn->isMouseOver = true;

        // if (input->mouseButtonState[MOUSE_BUTTON_LEFT] == MOUSE_BUTTON_STATE_PRESSED) {
        //     return (Message){MESSAGE_EV_UI_CLICKED};
        // }
    }

    return (Message){MESSAGE_NONE};
}

void uiTextBox_drawTextLine(UITextBox *tb, const char *text, Color color) {
    DrawTextEx(tb->font, text, tb->cursorPosition, tb->fontSize, 0, color);
    tb->cursorPosition.y += tb->fontSize;
}
