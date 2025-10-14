#pragma once
#include "../input/input.h"
#include "../messages/messages.h"

typedef struct {
    Font font;
    int fontSize;
    Rectangle bounds;
    Vector2 padding;
    Vector2 cursorPosition;
    bool isMouseOver;
} UITextBox;

void uiTextBox_init(UITextBox *bp, Font font, int fontSize, Rectangle bounds, Vector2 padding);
Message uiTextBox_processInput(UITextBox *bn, InputManager *input);
void uiTextBox_translate(UITextBox *bp, Vector2 newPos);
void uiTextBox_drawTextLine(UITextBox *tb, const char *text, Color color);
