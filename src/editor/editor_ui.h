#pragma once

#include <raylib.h>

#define EDITOR_UI_MAX_PANEL_NODES 32
#define EDITOR_UI_MAX_CHARS_TEXT_NODE 32

typedef struct {
    int padding, gap;

    enum {
        LAYOUT_DIR_COL,
        LAYOUT_DIR_ROW,
    } direction;
} UILayout;

void ui_StartPanel(Vector2 position, UILayout layout);
void ui_EndPanel();

void ui_AddTextNode(char *text, int font_size);
void ui_AddSeparator(int thickness);
