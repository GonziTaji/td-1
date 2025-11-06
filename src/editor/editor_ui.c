#include "editor_ui.h"
#include "../core/asset_manager.h"
#include <assert.h>
#include <endian.h>
#include <raylib.h>
#include <raymath.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

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
        NODE_TYPE_SEPARATOR,
    } type;

    Color bg_color;
    Color text_color;
    Rectangle aabb;

    union {
        UITextNode text_box;
        UIButtonNode button;
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

static bool isStarted = false;

void ui_StartPanel(Vector2 position, UILayout layout) {
    assert(!isStarted && "Panel already started");

    isStarted = true;
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
    assert(isStarted && "Panel must be started to add a node");
    assert(panel.nodes_count < EDITOR_UI_MAX_PANEL_NODES && "Number of ui nodes in a panel exceeded");
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

    UINode *const node = &panel.nodes[panel.nodes_count];

    node->type = NODE_TYPE_TEXT;
    node->text_box.font_size = font_size;
    memcpy(node->text_box.text, text, EDITOR_UI_MAX_CHARS_TEXT_NODE);

    AddGapIfNeeded();

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

void ui_EndPanel() {
    assert(isStarted && "Panel must be started to end it");

    Rectangle panel_rec = panel.bounds;

    Vector2 cursor_delta = Vector2Subtract(cursor_current_pos, cursor_initial_pos);

    if (panel.layout.direction == LAYOUT_DIR_COL) {
        panel_rec.height = cursor_delta.y + (panel.layout.padding * 2);
    } else {
        panel_rec.width = cursor_delta.x + (panel.layout.padding * 2);
    }

    DrawRectangleRec(panel_rec, GRAY);

    for (int i = 0; i < panel.nodes_count; i++) {
        UINode *const node = &panel.nodes[i];

        switch (node->type) {
        case NODE_TYPE_SEPARATOR:
            if (panel.layout.direction == LAYOUT_DIR_COL) {
                node->aabb.width = panel_rec.width - (panel.layout.padding * 2);
            } else {
                node->aabb.height = panel_rec.height - (panel.layout.padding * 2);
            }
            DrawRectangleRec(node->aabb, DARKGRAY);
            break;

        case NODE_TYPE_TEXT:
            DrawTextEx(uiFont,
                node->text_box.text,
                (Vector2){node->aabb.x, node->aabb.y},
                node->text_box.font_size,
                0,
                WHITE);

            break;

        case NODE_TYPE_BUTTON:
            break;
        }

        const bool hovered = CheckCollisionPointRec(GetMousePosition(), node->aabb);

        // Activate/deactivate?
        if (hovered || (IsKeyDown(KEY_B) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)))) {
            DrawRectangleLinesEx(node->aabb, 2, GREEN);
        }

        if (hovered) {
            const Vector2 tooltip_size = {90, 60};
            const int tooltip_margin = 5;
            const int tooltip_padding = 5;

            Vector2 tooltip_cursor = {
                node->aabb.x,
                node->aabb.y - tooltip_size.y - tooltip_margin,
            };

            if (node->aabb.y < tooltip_size.y) {
                tooltip_cursor.y = tooltip_margin;
            }

            DrawRectangleV(tooltip_cursor, tooltip_size, BLACK);

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

    isStarted = false;
}
