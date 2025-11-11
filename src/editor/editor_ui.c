#include "editor_ui.h"
#include "../core/asset_manager.h"
#include "../utils/utils.h"
#include <assert.h>
#include <endian.h>
#include <raylib.h>
#include <raymath.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EDITOR_UI_MAX_PANEL_NODES 128
#define EDITOR_UI_BUTTON_MIN_WIDTH 100
#define EDITOR_UI_MAX_CHARS_INPUT_NODE 32
#define EDITOR_UI_MAX_CHARS_INPUT_LABEL 32

static bool show_ui_debug = false;

// colors
const Color color_text = {239, 241, 243, 255};
const Color color_panel_bg = {1, 17, 10, 255};
const Color color_separator_bg = {105, 103, 115, 255};
const Color color_button_bg = {39, 38, 44, 255};
const Color color_button_border = {105, 103, 115, 255};
const Color color_button_border_hover = WHITE;
const Color color_input_bg = {0, 0, 0, 0};
const Color color_input_border = {105, 103, 115, 255};
const Color color_input_border_active = WHITE;

// spacing
static const int button_padding = 5;
static const int input_padding = 5;
static const int input_label_padding = 5;

// other
static const int input_int_width = 50;
static const int input_text_min_width = 150;

typedef struct {
    int font_size;
    bool active;
    char text_value[EDITOR_UI_MAX_CHARS_INPUT_NODE];
    char label[EDITOR_UI_MAX_CHARS_INPUT_LABEL];
    float label_width;
} UIInputNode;

typedef struct {
    int font_size;
    int gap;
    int button_count;
    char *labels[EDITOR_UI_TOOLBAR_MAX_BUTTONS];
    Rectangle buttons_aabb[EDITOR_UI_TOOLBAR_MAX_BUTTONS];
} UIToolbarNode;

typedef struct {
    int font_size;
    char text[EDITOR_UI_MAX_CHARS_TEXT_NODE];
} UITextNode;

typedef struct {
    int id;
    UITextNode text_node;
} UIButtonNode;

typedef struct {
    enum {
        NODE_TYPE_TEXT,
        NODE_TYPE_BUTTON,
        NODE_TYPE_TOOL_BAR, // group of horizontal buttons
        NODE_TYPE_INPUT,
        NODE_TYPE_SEPARATOR,
    } type;

    Color bg_color;
    Color text_color;
    Rectangle aabb;

    union {
        UITextNode text_box;
        UIButtonNode button;
        UIToolbarNode toolbar;
        UIInputNode input;
    };
} UINode;

typedef struct {
    int nodes_count;
    UINode nodes[EDITOR_UI_MAX_PANEL_NODES];

    Rectangle bounds;
    UILayout layout;
} UIPanel;

static UIPanel panel = {0};

static Vector2 cursor_initial_pos = {0};
static Vector2 cursor_current_pos = {0};

static bool is_started = false;
static bool active_node_found = false;

void ui_StartPanel(Vector2 position, UILayout layout) {
    assert(!is_started && "Panel already started");

    is_started = true;
    active_node_found = false;
    panel = (UIPanel){0};
    panel.bounds = (Rectangle){position.x, position.y, layout.padding * 2, layout.padding * 2};
    panel.layout = layout;
    panel.nodes_count = 0;

    cursor_initial_pos = Vector2AddValue(position, layout.padding);
    cursor_current_pos = cursor_initial_pos;
}

static void AddGapIfNeeded() {
    if (panel.nodes_count > 0) {
        if (panel.layout.direction == LAYOUT_DIR_COL) {
            cursor_current_pos.y += panel.layout.gap;
        } else {
            cursor_current_pos.x += panel.layout.gap;
        }
    }
}

static void AssertNodeCanBeAdded() {
    assert(is_started && "Panel must be started to add a node");
    assert(panel.nodes_count < EDITOR_UI_MAX_PANEL_NODES && "Number of ui nodes in a panel exceeded");
}

static IsActive AddInput(InputType type, char *label, char *text_value, void *value, int font_size, bool is_edit_mode) {
    AssertNodeCanBeAdded();
    AddGapIfNeeded();

    UINode *const node = &panel.nodes[panel.nodes_count];

    memcpy(node->input.text_value, text_value, EDITOR_UI_MAX_CHARS_INPUT_NODE);

    node->type = NODE_TYPE_INPUT;
    node->input.font_size = font_size;

    switch (type) {
    case INPUT_TYPE_TEXT:
        node->aabb.width = MeasureTextEx(uiFont, text_value, font_size, 0).x + (input_label_padding * 2);
        node->aabb.width = MAX(input_text_min_width, node->aabb.width);
        break;

    case INPUT_TYPE_INT:
        node->aabb.width = input_int_width;
        break;
    }

    node->aabb.height = font_size;
    node->aabb.height += input_padding * 2;
    node->input.active = is_edit_mode;

    node->aabb.x = cursor_current_pos.x;
    node->aabb.y = cursor_current_pos.y;

    if ((int)strlen(label) > 0) {
        memcpy(node->input.label, label, EDITOR_UI_MAX_CHARS_INPUT_LABEL);
        node->input.label_width = MeasureTextEx(uiFont, node->input.label, font_size, 0).x + input_label_padding;

        node->aabb.width += node->input.label_width;
    } else {
        node->input.label_width = 0;
    }

    panel.nodes_count++;

    const int double_padding = panel.layout.padding * 2;

    if (panel.layout.direction == LAYOUT_DIR_COL) {
        cursor_current_pos.y += node->aabb.height;

        float total_node_width = node->aabb.width + double_padding;
        if (panel.bounds.width < total_node_width) {
            panel.bounds.width = total_node_width;
        }
    } else {
        cursor_current_pos.x += node->aabb.width;

        float total_node_height = node->aabb.height + double_padding;
        if (panel.bounds.height < total_node_height) {
            panel.bounds.height = total_node_height;
        }
    }

    if (is_edit_mode) {
        bool value_has_changed = false;

        int key_count = (int)strlen(text_value);

        // Only allow keys in range [48..57] (numbers)
        if (key_count < EDITOR_UI_MAX_CHARS_INPUT_NODE) {
            const float text_width = MeasureTextEx(uiFont, text_value, font_size, 0).x;

            if (text_width < node->aabb.width - (input_padding * 2)) {
                int key = GetCharPressed();
                bool key_accepted = false;

                switch (type) {
                case INPUT_TYPE_TEXT:
                    key_accepted = true;
                    break;
                case INPUT_TYPE_INT:
                    if ((key >= 48) && (key <= 57)) {
                        key_accepted = true;
                    }
                    break;
                }

                if (key_accepted) {
                    text_value[key_count] = (char)key;
                    key_count++;
                    value_has_changed = true;
                }
            }
        }

        // Delete text
        if (key_count > 0 && IsKeyPressed(KEY_BACKSPACE)) {
            key_count--;
            text_value[key_count] = '\0';
            value_has_changed = true;
        }

        if (value_has_changed) {
            switch (type) {
            case INPUT_TYPE_TEXT:
                memcpy(value, text_value, sizeof(*text_value));
                break;
            case INPUT_TYPE_INT:
                if (value_has_changed) {
                    *(int *)value = atoi(text_value);
                }
                break;
            }
        }
    }

    // Only detect one click per frame
    if (active_node_found == false && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)
        && CheckCollisionPointRec(GetMousePosition(), node->aabb)) {

        active_node_found = true;
        return true;
    }

    return false;
}

IsActive ui_AddIntInput(char *label, int *value, int font_size, bool is_edit_mode) {
    char text_value[EDITOR_UI_MAX_CHARS_INPUT_NODE];
    snprintf(text_value, sizeof(text_value), "%d", *value);

    return AddInput(INPUT_TYPE_INT, label, text_value, value, font_size, is_edit_mode);
}

IsActive ui_AddTextInput(char *label, char *text_value, int font_size, bool is_edit_mode) {
    UINode *const node = &panel.nodes[panel.nodes_count];

    if (is_edit_mode) {
        bool value_has_changed = false;

        int key_count = (int)strlen(text_value);

        // Only allow keys in range [48..57] (numbers)
        if (key_count < EDITOR_UI_MAX_CHARS_INPUT_NODE) {
            const float text_width = MeasureTextEx(uiFont, text_value, font_size, 0).x;

            if (text_width < node->aabb.width - (input_padding * 2)) {
                int key = GetCharPressed();
                if ((key >= 48) && (key <= 57)) {
                    text_value[key_count] = (char)key;
                    key_count++;
                    value_has_changed = true;
                }
            }
        }

        // Delete text
        if (key_count > 0 && IsKeyPressed(KEY_BACKSPACE)) {
            key_count--;
            text_value[key_count] = '\0';
            value_has_changed = true;
        }

        if (value_has_changed) {
            // *value = atoi(text_value);
        }
    }

    return AddInput(INPUT_TYPE_TEXT, label, text_value, text_value, font_size, is_edit_mode);
}

IsActive ui_isCurrentNodeActive() {
    if (!active_node_found && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)
        && CheckCollisionPointRec(GetMousePosition(), panel.nodes[panel.nodes_count - 1].aabb)) {

        active_node_found = true;
        return true;
    }

    return false;
}

int ui_AddToolbar(int button_count, char **labels, int font_size) {
    AssertNodeCanBeAdded();
    AddGapIfNeeded();

    int active_button_idx = -1;

    UINode *const node = &panel.nodes[panel.nodes_count];

    memcpy(node->toolbar.labels, labels, sizeof(node->toolbar.labels));
    node->type = NODE_TYPE_TOOL_BAR;
    node->toolbar.font_size = font_size;
    node->toolbar.button_count = button_count;
    node->toolbar.gap = 5;
    node->aabb = (Rectangle){cursor_current_pos.x, cursor_current_pos.y, 0, font_size + (button_padding * 2)};

    Vector2 toolbar_cursor = RectangleGetPosition(node->aabb);

    for (int i = 0; i < button_count; i++) {
        float width = MeasureTextEx(uiFont, node->toolbar.labels[i], font_size, 0).x;

        width += button_padding * 2;

        node->toolbar.buttons_aabb[i] = (Rectangle){
            toolbar_cursor.x,
            toolbar_cursor.y,
            width,
            node->aabb.height,
        };

        if (active_button_idx == -1 && active_node_found == false && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)
            && CheckCollisionPointRec(GetMousePosition(), node->toolbar.buttons_aabb[i])) {

            active_node_found = true;
            active_button_idx = i;
        }

        toolbar_cursor.x += node->toolbar.buttons_aabb[i].width;
        if (i != button_count - 1) {
            toolbar_cursor.x += node->toolbar.gap;
        }
    }

    node->aabb.width = toolbar_cursor.x - node->aabb.x;

    panel.nodes_count++;

    const int double_padding = panel.layout.padding * 2;

    if (panel.layout.direction == LAYOUT_DIR_COL) {
        cursor_current_pos.y += node->aabb.height;

        float total_node_width = node->aabb.width + double_padding;
        if (panel.bounds.width < total_node_width) {
            panel.bounds.width = total_node_width;
        }
    } else {
        cursor_current_pos.x += node->aabb.width;

        float total_node_height = node->aabb.height + double_padding;
        if (panel.bounds.height < total_node_height) {
            panel.bounds.height = total_node_height;
        }
    }

    return active_button_idx;
}

IsActive ui_AddButton(char *text, int font_size) {
    AssertNodeCanBeAdded();
    AddGapIfNeeded();

    UINode *const node = &panel.nodes[panel.nodes_count];

    node->type = NODE_TYPE_BUTTON;
    node->text_box.font_size = font_size;
    memcpy(node->text_box.text, text, EDITOR_UI_MAX_CHARS_TEXT_NODE);

    Vector2 dimensions = MeasureTextEx(uiFont, node->text_box.text, font_size, 0);
    dimensions = Vector2AddValue(dimensions, button_padding * 2);

    if (dimensions.x < EDITOR_UI_BUTTON_MIN_WIDTH) {
        dimensions.x = EDITOR_UI_BUTTON_MIN_WIDTH;
    }

    node->aabb.x = cursor_current_pos.x;
    node->aabb.y = cursor_current_pos.y;
    node->aabb.width = dimensions.x;
    node->aabb.height = dimensions.y;

    panel.nodes_count++;

    const int double_padding = panel.layout.padding * 2;

    if (panel.layout.direction == LAYOUT_DIR_COL) {
        cursor_current_pos.y += node->aabb.height;

        float total_node_width = node->aabb.width + double_padding;
        if (panel.bounds.width < total_node_width) {
            panel.bounds.width = total_node_width;
        }
    } else {
        cursor_current_pos.x += node->aabb.width;

        float total_node_height = node->aabb.height + double_padding;
        if (panel.bounds.height < total_node_height) {
            panel.bounds.height = total_node_height;
        }
    }

    // Only detect one click per frame
    if (active_node_found == false && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)
        && CheckCollisionPointRec(GetMousePosition(), node->aabb)) {

        active_node_found = true;
        return true;
    }

    return false;
}

void ui_AddSeparator(int thickness) {
    AssertNodeCanBeAdded();
    AddGapIfNeeded();

    UINode *const node = &panel.nodes[panel.nodes_count];

    node->type = NODE_TYPE_SEPARATOR;

    node->aabb.x = cursor_current_pos.x;
    node->aabb.y = cursor_current_pos.y;
    node->aabb.width = 0;
    node->aabb.height = 0;

    if (panel.layout.direction == LAYOUT_DIR_COL) {
        node->aabb.height = thickness;
        cursor_current_pos.y += thickness;
    } else {
        node->aabb.width = thickness;
        cursor_current_pos.x += thickness;
    }

    panel.nodes_count++;
}

void ui_AddTextNode(char *text, int font_size) {
    AssertNodeCanBeAdded();
    AddGapIfNeeded();

    UINode *const node = &panel.nodes[panel.nodes_count];

    node->type = NODE_TYPE_TEXT;
    node->text_box.font_size = font_size;
    memcpy(node->text_box.text, text, EDITOR_UI_MAX_CHARS_TEXT_NODE);

    // use text_box.text here in case text is truncated on memcpy
    const Vector2 text_measurements = MeasureTextEx(uiFont, node->text_box.text, font_size, 0);
    node->aabb = (Rectangle){
        cursor_current_pos.x,
        cursor_current_pos.y,
        text_measurements.x,
        text_measurements.y,
    };

    const int double_padding = panel.layout.padding * 2;

    if (panel.layout.direction == LAYOUT_DIR_COL) {
        cursor_current_pos.y += node->aabb.height;

        float total_node_width = node->aabb.width + double_padding;
        if (panel.bounds.width < total_node_width) {
            panel.bounds.width = total_node_width;
        }
    } else {
        cursor_current_pos.x += node->aabb.width;

        float total_node_height = node->aabb.height + double_padding;
        if (panel.bounds.height < total_node_height) {
            panel.bounds.height = total_node_height;
        }
    }

    panel.nodes_count++;
}

/// returns final panel bounds
Rectangle ui_EndPanel() {
    assert(is_started && "Panel must be started to end it");

    Rectangle panel_rec = panel.bounds;

    Vector2 cursor_delta = Vector2Subtract(cursor_current_pos, cursor_initial_pos);

    if (panel.layout.direction == LAYOUT_DIR_COL) {
        panel_rec.height = cursor_delta.y + (panel.layout.padding * 2);
    } else {
        panel_rec.width = cursor_delta.x + (panel.layout.padding * 2);
    }

    DrawRectangleRec(panel_rec, color_panel_bg);

    bool hovered = false;
    Vector2 text_pos = {0};

    for (int i = 0; i < panel.nodes_count; i++) {
        UINode *const node = &panel.nodes[i];

        switch (node->type) {
        case NODE_TYPE_SEPARATOR:
            if (panel.layout.direction == LAYOUT_DIR_COL) {
                node->aabb.width = panel_rec.width - (panel.layout.padding * 2);
            } else {
                node->aabb.height = panel_rec.height - (panel.layout.padding * 2);
            }
            DrawRectangleRec(node->aabb, color_separator_bg);
            break;

        case NODE_TYPE_TEXT:
            DrawTextEx(uiFont,
                node->text_box.text,
                (Vector2){node->aabb.x, node->aabb.y},
                node->text_box.font_size,
                0,
                color_text);

            break;

        case NODE_TYPE_BUTTON:
            DrawRectangleRec(node->aabb, color_button_bg);

            hovered = CheckCollisionPointRec(GetMousePosition(), node->aabb);

            DrawRectangleLinesEx(node->aabb, 2, hovered ? color_button_border_hover : color_button_border);

            text_pos = Vector2AddValue((Vector2){node->aabb.x, node->aabb.y}, input_padding);

            DrawTextEx(uiFont, node->text_box.text, text_pos, node->text_box.font_size, 0, color_text);
            break;

        case NODE_TYPE_TOOL_BAR:
            for (int i = 0; i < node->toolbar.button_count; i++) {
                Rectangle button_rec = node->toolbar.buttons_aabb[i];

                DrawRectangleRec(button_rec, color_button_bg);
                hovered = CheckCollisionPointRec(GetMousePosition(), button_rec);

                DrawRectangleLinesEx(button_rec, 2, hovered ? color_button_border_hover : color_button_border);

                text_pos = Vector2AddValue(RectangleGetPosition(button_rec), button_padding);

                DrawTextEx(uiFont, node->toolbar.labels[i], text_pos, node->toolbar.font_size, 0, color_text);
            }

            break;

        case NODE_TYPE_INPUT: {
            Vector2 input_node_cursor = RectangleGetPosition(node->aabb);
            Rectangle input_box_aabb = node->aabb;

            if (node->input.label_width > 0) {
                Vector2 label_pos = input_node_cursor;
                label_pos.y += input_padding;

                DrawTextEx(uiFont, node->input.label, label_pos, node->input.font_size, 0, color_text);

                input_node_cursor.x += node->input.label_width;
                input_box_aabb.x += node->input.label_width;
                input_box_aabb.width -= node->input.label_width;
            }

            DrawRectangleRec(input_box_aabb, color_input_bg);

            hovered = CheckCollisionPointRec(GetMousePosition(), node->aabb);

            const Color border_color = node->input.active ? color_input_border_active : color_input_border;

            DrawRectangleLinesEx(input_box_aabb, 2, border_color);

            text_pos = Vector2AddValue((Vector2){input_box_aabb.x, input_box_aabb.y}, input_padding);

            DrawTextEx(uiFont, node->input.text_value, text_pos, node->input.font_size, 0, color_text);
            break;
        }
        }

        if (show_ui_debug) {
            hovered = CheckCollisionPointRec(GetMousePosition(), node->aabb);

            if (hovered || (IsKeyDown(KEY_B) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)))) {
                DrawRectangleLinesEx(node->aabb, 2, GREEN);
            }

            if (hovered) {
                const Vector2 tooltip_size = {90, 60};
                const int tooltip_margin = 5;
                const int tooltip_padding = 10;

                Vector2 tooltip_cursor = {
                    node->aabb.x + tooltip_margin,
                    node->aabb.y - tooltip_size.y - tooltip_margin,
                };

                if (node->aabb.y < tooltip_size.y) {
                    tooltip_cursor.y = tooltip_margin;
                }

                DrawRectangleV(tooltip_cursor, tooltip_size, BLACK);
                DrawRectangleLinesEx(Vector2ToRec(tooltip_cursor, tooltip_size), 2, GRAY);

                tooltip_cursor.x += tooltip_padding;
                tooltip_cursor.y += tooltip_padding;

                char buffer[32];
                int font_size = 16;
                snprintf(buffer, sizeof(buffer), "[x,y] %.0f %.0f", tooltip_cursor.x, tooltip_cursor.y);
                DrawTextEx(uiFont, buffer, tooltip_cursor, font_size, 0, WHITE);

                tooltip_cursor.y += font_size + tooltip_padding;
                snprintf(buffer, sizeof(buffer), "[w,h] %.0f %.0f", node->aabb.width, node->aabb.height);
                DrawTextEx(uiFont, buffer, tooltip_cursor, font_size, 0, WHITE);
            }
        }
    }

    is_started = false;

    return panel_rec;
}
