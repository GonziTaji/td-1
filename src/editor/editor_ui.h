#pragma once

#include <raylib.h>

#define EDITOR_UI_MAX_CHARS_TEXT_NODE 128
#define EDITOR_UI_TOOLBAR_MAX_BUTTONS 8

typedef struct {
    int padding, gap;

    enum {
        LAYOUT_DIR_COL,
        LAYOUT_DIR_ROW,
    } direction;
} UILayout;

typedef enum {
    INPUT_TYPE_TEXT,
    INPUT_TYPE_INT,
} InputType;

typedef bool IsActive;

void ui_StartPanel(Vector2 position, UILayout layout);
Rectangle ui_EndPanel();

void ui_AddTextNode(char *text, int font_size);
void ui_AddSeparator(int thickness);
IsActive ui_AddButton(char *text, int font_size);
int ui_AddToolbar(int button_count, char **labels, int font_size);
IsActive ui_AddIntInput(char *label, int *value, int font_size, bool is_edit_mode);
IsActive ui_AddTextInput(char *label, char *text_value, int font_size, bool is_edit_mode);
