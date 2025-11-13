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
#define EDITOR_UI_MAX_FLOAT_INPUTS 64
#define EDITOR_UI_MAX_COMBOBOXES 32

static bool show_ui_debug = false;

// colors
const Color color_text = {239, 241, 243, 255};
const Color color_panel_bg = {1, 17, 10, 255};
const Color color_separator_bg = {105, 103, 115, 255};
const Color color_button_bg = {39, 38, 44, 255};
const Color color_button_border = {105, 103, 115, 255};
const Color color_button_border_hover = {222, 222, 222, 255};
const Color color_button_border_active = WHITE;
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
static const int combo_min_width = 100;
static const int combo_arrow_size = 6;
static const int combo_arrow_padding = 10;

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
    bool is_switch;
    int switch_active_button_idx;
} UIToolbarNode;

typedef struct {
    int font_size;
    char text[EDITOR_UI_MAX_CHARS_TEXT_NODE];
} UITextNode;

typedef struct {
    int font_size;
    char text[EDITOR_UI_MAX_CHARS_TEXT_NODE];
} UIButtonNode;

typedef struct {
    bool in_use;
    int *selected_ptr;
    bool open;
} UIComboBoxState;

typedef struct {
    int font_size;
    char label[EDITOR_UI_MAX_CHARS_INPUT_LABEL];
    float label_width;
    int option_count;
    char *options[EDITOR_UI_COMBOBOX_MAX_OPTIONS];
    UIComboBoxState *state;
    Rectangle box_bounds;
    Rectangle dropdown_bounds;
    float option_height;
} UIComboBoxNode;

typedef struct {
    enum {
        NODE_TYPE_TEXT,
        NODE_TYPE_BUTTON,
        NODE_TYPE_TOOL_BAR, // group of horizontal buttons
        NODE_TYPE_INPUT,
        NODE_TYPE_SEPARATOR,
        NODE_TYPE_COMBOBOX,
    } type;

    Color bg_color;
    Color text_color;
    Rectangle aabb;

    union {
        UITextNode text_box;
        UIButtonNode button;
        UIToolbarNode toolbar;
        UIInputNode input;
        UIComboBoxNode combobox;
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

typedef struct {
    bool in_use;
    float *value_ptr;
    char buffer[EDITOR_UI_MAX_CHARS_INPUT_NODE];
} UIFloatInputState;

static UIFloatInputState float_inputs[EDITOR_UI_MAX_FLOAT_INPUTS] = {0};

static UIComboBoxState combobox_states[EDITOR_UI_MAX_COMBOBOXES] = {0};

static void DrawSeparatorNode(UINode *const node) {
    if (panel.layout.direction == LAYOUT_DIR_COL) {
        node->aabb.width = panel.bounds.width - (panel.layout.padding * 2);
    } else {
        node->aabb.height = panel.bounds.height - (panel.layout.padding * 2);
    }

    DrawRectangleRec(node->aabb, color_separator_bg);
}

static void DrawTextNode(UINode *const node) {
    DrawTextEx(uiFont,
        node->text_box.text,
        (Vector2){node->aabb.x, node->aabb.y},
        node->text_box.font_size,
        0,
        color_text);
}

static void DrawButtonNode(UINode *const node) {
    DrawRectangleRec(node->aabb, color_button_bg);

    bool hovered = CheckCollisionPointRec(GetMousePosition(), node->aabb);
    DrawRectangleLinesEx(node->aabb, 2, hovered ? color_button_border_hover : color_button_border);

    Vector2 text_pos = Vector2AddValue((Vector2){node->aabb.x, node->aabb.y}, input_padding);
    DrawTextEx(uiFont, node->button.text, text_pos, node->button.font_size, 0, color_text);
}

static void DrawToolbarNode(UINode *const node) {
    Vector2 text_pos = {0};
    bool hovered = false;

    for (int i = 0; i < node->toolbar.button_count; i++) {
        Rectangle button_rec = node->toolbar.buttons_aabb[i];

        DrawRectangleRec(button_rec, color_button_bg);
        hovered = CheckCollisionPointRec(GetMousePosition(), button_rec);

        Color border_color = color_button_border;

        if (hovered) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                border_color = color_button_border_active;
            } else {
                border_color = color_button_border_hover;
            }

        } else if (node->toolbar.is_switch && i == node->toolbar.switch_active_button_idx) {
            border_color = color_button_border_active;
        }

        DrawRectangleLinesEx(button_rec, 2, border_color);

        text_pos = Vector2AddValue(RectangleGetPosition(button_rec), button_padding);
        DrawTextEx(uiFont, node->toolbar.labels[i], text_pos, node->toolbar.font_size, 0, color_text);
    }
}

static void DrawInput(UINode *const node) {
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

    bool hovered = CheckCollisionPointRec(GetMousePosition(), node->aabb);

    Color border_color = color_input_border;

    if (node->input.active || (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        border_color = color_input_border_active;
    }

    DrawRectangleLinesEx(input_box_aabb, 2, border_color);

    Vector2 text_pos = Vector2AddValue((Vector2){input_box_aabb.x, input_box_aabb.y}, input_padding);
    DrawTextEx(uiFont, node->input.text_value, text_pos, node->input.font_size, 0, color_text);
}

static void DrawComboboxNode(UINode *const node) {
    Vector2 mouse_position = GetMousePosition();

    if (node->combobox.label_width > 0) {
        Vector2 label_pos = {
            node->aabb.x,
            node->aabb.y + input_padding,
        };
        DrawTextEx(uiFont, node->combobox.label, label_pos, node->combobox.font_size, 0, color_text);
    }

    Rectangle combo_box = node->combobox.box_bounds;

    DrawRectangleRec(combo_box, color_input_bg);

    Color border_color = node->combobox.state->open ? color_input_border_active : color_input_border;

    if (CheckCollisionPointRec(mouse_position, combo_box)) {
        border_color = color_input_border_active;
    }

    DrawRectangleLinesEx(combo_box, 2, border_color);

    const int selected_index = (node->combobox.state->selected_ptr != NULL) ? *node->combobox.state->selected_ptr : -1;
    const bool has_selection = selected_index >= 0 && selected_index < node->combobox.option_count;
    const char *display_text = has_selection ? node->combobox.options[selected_index] : "-";

    Vector2 combo_text_pos = {
        combo_box.x + input_padding,
        combo_box.y + input_padding,
    };

    DrawTextEx(uiFont, display_text, combo_text_pos, node->combobox.font_size, 0, color_text);

    Vector2 arrow_center = {
        combo_box.x + combo_box.width - (input_padding * 2) - combo_arrow_padding,
        combo_box.y + (combo_box.height / 2.0f),
    };
    Vector2 arrow_points[3] = {
        {arrow_center.x + combo_arrow_size, arrow_center.y - (combo_arrow_size / 2.0f)},
        {arrow_center.x - combo_arrow_size, arrow_center.y - (combo_arrow_size / 2.0f)},
        {arrow_center.x, arrow_center.y + (combo_arrow_size / 2.0f)},
    };

    DrawTriangle(arrow_points[0], arrow_points[1], arrow_points[2], color_text);
}

static UIFloatInputState *GetFloatInputState(float *value_ptr) {
    UIFloatInputState *available_slot = NULL;

    for (int i = 0; i < EDITOR_UI_MAX_FLOAT_INPUTS; i++) {
        UIFloatInputState *state = &float_inputs[i];

        if (state->in_use && state->value_ptr == value_ptr) {
            return state;
        }

        if (!state->in_use && available_slot == NULL) {
            available_slot = state;
        }
    }

    if (available_slot) {
        *available_slot = (UIFloatInputState){
            .in_use = true,
            .value_ptr = value_ptr,
            .buffer = {0},
        };
        return available_slot;
    }

    assert(false && "Exceeded maximum float inputs being tracked");
    return NULL;
}

static bool ShouldCommitFloatValue(const char *text_value) {
    const size_t len = strlen(text_value);

    if (len == 0) {
        return false;
    }

    char *end_ptr = NULL;
    strtof(text_value, &end_ptr);

    if (end_ptr == text_value) {
        return false;
    }

    if (*end_ptr != '\0') {
        return false;
    }

    const char last_char = text_value[len - 1];
    if (last_char == '.' || last_char == '-' || last_char == '+') {
        return false;
    }

    return true;
}

static UIComboBoxState *GetComboBoxState(int *selected_ptr) {
    UIComboBoxState *available_slot = NULL;

    for (int i = 0; i < EDITOR_UI_MAX_COMBOBOXES; i++) {
        UIComboBoxState *state = &combobox_states[i];

        if (state->in_use && state->selected_ptr == selected_ptr) {
            return state;
        }

        if (!state->in_use && available_slot == NULL) {
            available_slot = state;
        }
    }

    if (available_slot) {
        *available_slot = (UIComboBoxState){
            .in_use = true,
            .selected_ptr = selected_ptr,
            .open = false,
        };
        return available_slot;
    }

    assert(false && "Exceeded maximum combobox states being tracked");
    return NULL;
}

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

static void CommitNewNode() {
    const UINode *const node = &panel.nodes[panel.nodes_count];
    panel.nodes_count++;

    const int double_padding = panel.layout.padding * 2;

    if (panel.layout.direction == LAYOUT_DIR_COL) {
        cursor_current_pos.y += node->aabb.height;

        float new_panel_width = node->aabb.x + node->aabb.width - panel.bounds.x + double_padding;
        if (panel.bounds.width < new_panel_width) {
            panel.bounds.width = new_panel_width;
        }
    } else {
        cursor_current_pos.x += node->aabb.width;

        float new_panel_height = node->aabb.y + node->aabb.height - panel.bounds.y + double_padding;
        if (panel.bounds.height < new_panel_height) {
            panel.bounds.height = new_panel_height;
        }
    }
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
    case INPUT_TYPE_FLOAT:
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

    CommitNewNode();

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
                case INPUT_TYPE_FLOAT: {
                    if ((key >= 48) && (key <= 57)) {
                        key_accepted = true;
                    } else if (key == '.' && strchr(text_value, '.') == NULL) {
                        key_accepted = true;
                    } else if (key == '-' && key_count == 0) {
                        key_accepted = true;
                    }
                    break;
                }
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
            case INPUT_TYPE_FLOAT:
                if (ShouldCommitFloatValue(text_value)) {
                    *(float *)value = strtof(text_value, NULL);
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

IsActive ui_AddFloatInput(char *label, float *value, int font_size, bool is_edit_mode) {
    UIFloatInputState *state = GetFloatInputState(value);

    if (!state) {
        return false;
    }

    if (!is_edit_mode || state->buffer[0] == '\0') {
        snprintf(state->buffer, sizeof(state->buffer), "%g", *value);
    }

    IsActive result = AddInput(INPUT_TYPE_FLOAT, label, state->buffer, value, font_size, is_edit_mode);

    if (!is_edit_mode) {
        snprintf(state->buffer, sizeof(state->buffer), "%g", *value);
    }

    return result;
}

IsActive ui_AddComboBox(const char *label, int *value, char **options, int options_count, int font_size) {
    UIComboBoxState *state = GetComboBoxState(value);

    AssertNodeCanBeAdded();
    AddGapIfNeeded();

    UINode *const node = &panel.nodes[panel.nodes_count];

    node->type = NODE_TYPE_COMBOBOX;
    node->combobox.font_size = font_size;
    node->combobox.state = state;
    node->combobox.label_width = 0;
    node->combobox.label[0] = '\0';

    if (label != NULL && label[0] != '\0') {
        strncpy(node->combobox.label, label, sizeof(node->combobox.label) - 1);
        node->combobox.label[sizeof(node->combobox.label) - 1] = '\0';
        node->combobox.label_width = MeasureTextEx(uiFont, node->combobox.label, font_size, 0).x + input_label_padding;
    }

    node->combobox.option_count = options_count;
    if (node->combobox.option_count > EDITOR_UI_COMBOBOX_MAX_OPTIONS) {
        node->combobox.option_count = EDITOR_UI_COMBOBOX_MAX_OPTIONS;
    }

    for (int i = 0; i < node->combobox.option_count; i++) {
        node->combobox.options[i] = options[i];
    }

    if (node->combobox.option_count == 0) {
        *value = -1;
        state->open = false;
    } else if (*value < 0 || *value >= node->combobox.option_count) {
        *value = 0;
    }

    const char *selected_text = "-";
    if (node->combobox.option_count > 0 && *value >= 0) {
        selected_text = node->combobox.options[*value];
    }

    float max_option_width = MeasureTextEx(uiFont, selected_text, font_size, 0).x;

    for (int i = 0; i < node->combobox.option_count; i++) {
        float option_width = MeasureTextEx(uiFont, node->combobox.options[i], font_size, 0).x;

        option_width += combo_arrow_padding + (combo_arrow_padding * 2);

        if (option_width > max_option_width) {
            max_option_width = option_width;
        }
    }

    float combo_box_width = MAX(combo_min_width, max_option_width + (input_padding * 2));

    node->aabb = (Rectangle){
        cursor_current_pos.x,
        cursor_current_pos.y,
        combo_box_width + node->combobox.label_width,
        font_size + (input_padding * 2),
    };

    node->combobox.box_bounds = (Rectangle){
        node->aabb.x + node->combobox.label_width,
        node->aabb.y,
        combo_box_width,
        node->aabb.height,
    };

    node->combobox.option_height = font_size + (input_padding * 2);

    node->combobox.dropdown_bounds = (Rectangle){
        node->combobox.box_bounds.x,
        node->combobox.box_bounds.y + node->combobox.box_bounds.height + 5,
        node->combobox.box_bounds.width,
        node->combobox.option_height * node->combobox.option_count,
    };

    CommitNewNode();

    IsActive isActive = false;

    Vector2 mouse_pos = GetMousePosition();
    bool main_hovered = CheckCollisionPointRec(mouse_pos, node->combobox.box_bounds);
    bool dropdown_hovered = state->open && CheckCollisionPointRec(mouse_pos, node->combobox.dropdown_bounds);
    bool click_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    if (state->open && node->combobox.option_count > 0 && click_released && active_node_found == false) {
        for (int i = 0; i < node->combobox.option_count; i++) {
            Rectangle option_rect = (Rectangle){
                node->combobox.dropdown_bounds.x,
                node->combobox.dropdown_bounds.y + (node->combobox.option_height * i),
                node->combobox.dropdown_bounds.width,
                node->combobox.option_height,
            };

            if (CheckCollisionPointRec(mouse_pos, option_rect)) {
                if (*value != i) {
                    *value = i;
                }

                state->open = false;
                active_node_found = true;
                break;
            }
        }
    }

    if (click_released && !active_node_found && main_hovered) {
        state->open = !state->open;
        active_node_found = true;
        if (state->open) {
            isActive = true;
        }
    }

    if (state->open) {
        bool click_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        if (click_pressed && !main_hovered && !dropdown_hovered) {
            state->open = false;
        }
    }

    return isActive;
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

void ui_AddSwitchButtons(int button_count, int *active_button_idx, char **labels, int font_size) {
    UINode *const node = &panel.nodes[panel.nodes_count];
    node->toolbar.is_switch = true;
    node->toolbar.switch_active_button_idx = *active_button_idx;

    int button_clicked_idx = ui_AddToolbar(button_count, labels, font_size);

    if (button_clicked_idx != -1) {
        *active_button_idx = button_clicked_idx;
        node->toolbar.switch_active_button_idx = button_clicked_idx;
    }
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

    CommitNewNode();

    return active_button_idx;
}

void ui_AddOptionsButton(const char *label, int *value, char **options, int options_count, int font_size) {
    const Vector2 text_dimensions = MeasureTextEx(uiFont, label, font_size, 0);

    float label_width = input_label_padding + text_dimensions.x;

    cursor_current_pos.x += label_width;
    const IsActive isActive = ui_AddButton(options[*value], font_size);
    cursor_current_pos.x -= label_width;

    if (isActive) {
        (*value)++;

        if (*value >= options_count) {
            *value = 0;
        }
    };

    AssertNodeCanBeAdded();

    const UINode *const button_node = &panel.nodes[panel.nodes_count - 1];
    const Rectangle button_aabb = button_node->aabb;

    UINode *const label_node = &panel.nodes[panel.nodes_count];
    *label_node = (UINode){0};
    label_node->type = NODE_TYPE_TEXT;
    label_node->text_box.font_size = font_size;
    label_node->aabb.x = cursor_current_pos.x;
    label_node->aabb.y = button_aabb.y + (button_aabb.height / 2) - (text_dimensions.y / 2);
    label_node->aabb.height = text_dimensions.y;
    label_node->aabb.width = label_width;

    memcpy(label_node->text_box.text, label, EDITOR_UI_MAX_CHARS_TEXT_NODE);

    // adjustment before commit. Label dimensions are already accounted for
    if (panel.layout.direction == LAYOUT_DIR_COL) {
        cursor_current_pos.y -= label_node->aabb.height;
    } else {
        cursor_current_pos.x -= label_node->aabb.width;
    }

    CommitNewNode();
}

IsActive ui_AddButton(char *text, int font_size) {
    AssertNodeCanBeAdded();
    AddGapIfNeeded();

    UINode *const node = &panel.nodes[panel.nodes_count];

    *node = (UINode){0};
    node->type = NODE_TYPE_BUTTON;

    node->button.font_size = font_size;

    memcpy(node->button.text, text, EDITOR_UI_MAX_CHARS_TEXT_NODE);

    Vector2 dimensions = MeasureTextEx(uiFont, node->button.text, font_size, 0);
    dimensions = Vector2AddValue(dimensions, button_padding * 2);

    if (dimensions.x < EDITOR_UI_BUTTON_MIN_WIDTH) {
        dimensions.x = EDITOR_UI_BUTTON_MIN_WIDTH;
    }

    node->aabb.x = cursor_current_pos.x;
    node->aabb.y = cursor_current_pos.y;
    node->aabb.width = dimensions.x;
    node->aabb.height = dimensions.y;

    CommitNewNode();

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

    CommitNewNode();
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

    CommitNewNode();
}

/// returns final panel bounds
Rectangle ui_EndPanel() {
    assert(is_started && "Panel must be started to end it");

    Vector2 cursor_delta = Vector2Subtract(cursor_current_pos, cursor_initial_pos);

    if (panel.layout.direction == LAYOUT_DIR_COL) {
        panel.bounds.height = cursor_delta.y + (panel.layout.padding * 2);
    } else {
        panel.bounds.width = cursor_delta.x + (panel.layout.padding * 2);
    }

    DrawRectangleRec(panel.bounds, color_panel_bg);

    bool hovered = false;

    int open_combo_idx = -1;

    for (int i = 0; i < panel.nodes_count; i++) {
        UINode *const node = &panel.nodes[i];

        switch (node->type) {
        case NODE_TYPE_SEPARATOR:
            DrawSeparatorNode(node);
            break;

        case NODE_TYPE_TEXT:
            DrawTextNode(node);
            break;

        case NODE_TYPE_BUTTON:
            DrawButtonNode(node);
            break;

        case NODE_TYPE_TOOL_BAR:
            DrawToolbarNode(node);
            break;

        case NODE_TYPE_INPUT:
            DrawInput(node);
            break;

        case NODE_TYPE_COMBOBOX:
            DrawComboboxNode(node);

            if (open_combo_idx == -1 && node->combobox.state->open) {
                open_combo_idx = i;
            }
            break;
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

    if (open_combo_idx != -1) {
        UINode *combo_node = &panel.nodes[open_combo_idx];
        Rectangle dropdown = combo_node->combobox.dropdown_bounds;

        const int dropdown_padding = 2;
        Rectangle dropdown_wrapper = {
            dropdown.x - dropdown_padding,
            dropdown.y - dropdown_padding,
            dropdown.width + dropdown_padding * 2,
            dropdown.height + dropdown_padding * 2,
        };

        DrawRectangleRec(dropdown_wrapper, color_input_border);

        Vector2 mouse_position = GetMousePosition();
        const int selected_index
            = (combo_node->combobox.state->selected_ptr != NULL) ? *combo_node->combobox.state->selected_ptr : -1;

        for (int option_idx = 0; option_idx < combo_node->combobox.option_count; option_idx++) {
            Rectangle option_rect = {
                dropdown.x,
                dropdown.y + (combo_node->combobox.option_height * option_idx),
                dropdown.width,
                combo_node->combobox.option_height,
            };

            bool option_hovered = CheckCollisionPointRec(mouse_position, option_rect);
            bool option_selected = (option_idx == selected_index);

            Color option_bg = option_selected ? color_button_border : color_panel_bg;
            if (option_hovered) {
                option_bg = color_button_bg;
            }

            DrawRectangleRec(option_rect, option_bg);

            Vector2 option_text_pos = {
                option_rect.x + input_padding,
                option_rect.y + input_padding,
            };

            DrawTextEx(uiFont,
                combo_node->combobox.options[option_idx],
                option_text_pos,
                combo_node->combobox.font_size,
                0,
                color_text);
        }
    }

    is_started = false;

    return panel.bounds;
}

void ui_MasterDetailBegin(UIMasterDetailPanel *panel_state, Vector2 origin, float gap) {
    assert(panel_state != NULL);

    *panel_state = (UIMasterDetailPanel){
        .origin = origin,
        .master_origin = origin,
        .detail_origin = origin,
        .gap = gap,
        .master_started = false,
        .detail_started = false,
        .master_bounds = {0},
        .detail_bounds = {0},
    };
}

void ui_MasterDetailBeginMaster(UIMasterDetailPanel *panel_state, UILayout layout) {
    assert(panel_state != NULL);
    assert(!panel_state->master_started && "Master panel already started");

    panel_state->master_layout = layout;
    panel_state->master_started = true;

    ui_StartPanel(panel_state->master_origin, layout);
}

Rectangle ui_MasterDetailEndMaster(UIMasterDetailPanel *panel_state) {
    assert(panel_state != NULL);
    assert(panel_state->master_started && "Master panel not started");

    panel_state->master_bounds = ui_EndPanel();
    panel_state->master_started = false;

    panel_state->detail_origin = (Vector2){
        panel_state->master_bounds.x + panel_state->master_bounds.width + panel_state->gap,
        panel_state->master_bounds.y,
    };

    return panel_state->master_bounds;
}

void ui_MasterDetailBeginDetail(UIMasterDetailPanel *panel_state, UILayout layout) {
    assert(panel_state != NULL);
    assert(!panel_state->detail_started && "Detail panel already started");

    panel_state->detail_layout = layout;
    panel_state->detail_started = true;

    ui_StartPanel(panel_state->detail_origin, layout);
}

Rectangle ui_MasterDetailEndDetail(UIMasterDetailPanel *panel_state) {
    assert(panel_state != NULL);
    assert(panel_state->detail_started && "Detail panel not started");

    panel_state->detail_bounds = ui_EndPanel();
    panel_state->detail_started = false;

    return panel_state->detail_bounds;
}

Rectangle ui_MasterDetailGetBounds(const UIMasterDetailPanel *panel_state) {
    assert(panel_state != NULL);

    const float left = MIN(panel_state->master_bounds.x, panel_state->detail_bounds.x);
    const float top = MIN(panel_state->master_bounds.y, panel_state->detail_bounds.y);

    const float right = MAX(panel_state->master_bounds.x + panel_state->master_bounds.width,
        panel_state->detail_bounds.x + panel_state->detail_bounds.width);
    const float bottom = MAX(panel_state->master_bounds.y + panel_state->master_bounds.height,
        panel_state->detail_bounds.y + panel_state->detail_bounds.height);

    return (Rectangle){
        left,
        top,
        right - left,
        bottom - top,
    };
}
