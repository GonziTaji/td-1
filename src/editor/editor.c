#include "../game/scene/scene_data.h"
#include "../game/scene/view_mamanger.h"
#include "../utils/grid.h"
#include "editor_ui.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>

/**
 *
 * General Editor.
 *
 * The editor should only control data from each module,
 * and each module react to that data change
 *
 */

static struct {
    bool show_scene_panel;
} state = {
    .show_scene_panel = true,
};

typedef enum {
    EDITOR_STATUS_NORMAL,
    EDITOR_STATUS_WAYPOINT_SELECTION,
} EditorStatus;

EditorStatus path_editor_status = EDITOR_STATUS_NORMAL;

void editor_Update(float delta_time) {
    if (IsKeyPressed(KEY_U)) {
        state.show_scene_panel = !state.show_scene_panel;
    }
}

static char buffer[EDITOR_UI_MAX_CHARS_TEXT_NODE];

static const struct {
    int title;
    int text;
    int button;
} font_sizes = {
    .title = 24,
    .text = 16,
    .button = 16,
};

static void DrawPathPanel() {
    const UILayout layout = {
        .padding = 20,
        .gap = 10,
        .direction = LAYOUT_DIR_COL,
    };

    Vector2 panel_pos = {30, 30};

    const Vector2 mouse_pos = GetMousePosition();
    const V2i hovered_coords = grid_worldPointToCoords(SCENE_TRANSFORM, mouse_pos.x, mouse_pos.y);
    const bool is_hovered_coords_valid = grid_isValidCoords(SCENE_DATA->cols, SCENE_DATA->rows, hovered_coords);

    ui_StartPanel(panel_pos, layout);

    //
    // SCENE SECTION
    //
    ui_AddSeparator(3);
    ui_AddTextNode("SCENE", font_sizes.title);
    ui_AddSeparator(3);

    snprintf(buffer, sizeof(buffer), "COLS: %d", SCENE_DATA->cols);
    ui_AddTextNode(buffer, font_sizes.text);

    snprintf(buffer, sizeof(buffer), "ROWS: %d", SCENE_DATA->rows);
    ui_AddTextNode(buffer, font_sizes.text);

    if (ui_AddButton("Add column", font_sizes.button)) {
        scene_data_ChangeGridDimensions(SCENE_DATA->cols + 1, SCENE_DATA->rows);
    }

    if (ui_AddButton("Remove column", font_sizes.button)) {
        scene_data_ChangeGridDimensions(SCENE_DATA->cols - 1, SCENE_DATA->rows);
    }

    if (ui_AddButton("Add row", font_sizes.button)) {
        scene_data_ChangeGridDimensions(SCENE_DATA->cols, SCENE_DATA->rows + 1);
    }

    if (ui_AddButton("Remove row", font_sizes.button)) {
        scene_data_ChangeGridDimensions(SCENE_DATA->cols, SCENE_DATA->rows - 1);
    }

    //
    // PATH SECTION
    //
    ui_AddSeparator(3);
    ui_AddTextNode("PATH", font_sizes.title);
    ui_AddSeparator(3);

    if (path_editor_status == EDITOR_STATUS_NORMAL) {
        if (ui_AddButton("Restore saved data", font_sizes.button)) {
            scene_data_ReloadCurrentScene();
        }

        if (ui_AddButton("Remove last waypoint", font_sizes.button)) {
            scene_data_RemoveLastWaypoint();
        }

        if (ui_AddButton("Add waypoints", font_sizes.button)) {
            path_editor_status = EDITOR_STATUS_WAYPOINT_SELECTION;
        }
    } else if (path_editor_status == EDITOR_STATUS_WAYPOINT_SELECTION) {
        ui_AddTextNode("Click on a tile to set the new waypoint", font_sizes.text);

        if (ui_AddButton("Cancel waypoint selection ", font_sizes.button)) {
            path_editor_status = EDITOR_STATUS_NORMAL;
        }

        if (is_hovered_coords_valid) {
            V2i last_waypoint = SCENE_DATA->pathWaypoints[SCENE_DATA->pathWaypointsCount - 1];
            Vector2 source = grid_getTileCenter(SCENE_TRANSFORM, last_waypoint.x, last_waypoint.y);
            Vector2 dest = grid_getTileCenter(SCENE_TRANSFORM, hovered_coords.x, hovered_coords.y);

            Color line_color = scene_data_WaypointCanBeSet(hovered_coords) ? GREEN : RED;
            DrawLineEx(source, dest, 10, line_color);

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                scene_data_AddWaypoint(hovered_coords);
            }
        }
    }

    ui_AddSeparator(3);

    const V2i *waypoints = SCENE_DATA->pathWaypoints;
    const int waypoint_count = SCENE_DATA->pathWaypointsCount;

    snprintf(buffer, sizeof(buffer), "Current path (%d waypoints)", waypoint_count);

    ui_AddTextNode(buffer, font_sizes.title);

    for (int i = 1; i < waypoint_count; i++) {
        snprintf(buffer,
            sizeof(buffer),
            "%d, %d  >  %d, %d",
            waypoints[i - 1].x,
            waypoints[i - 1].y,
            waypoints[i].x,
            waypoints[i].y);

        ui_AddTextNode(buffer, font_sizes.text);
    }

    //
    // TODO: WAVE SECTION
    //
    ui_AddSeparator(3);
    ui_AddTextNode("WAVES", font_sizes.title);
    ui_AddSeparator(3);

    if (ui_AddButton("Remove selected wave", font_sizes.button)) {
    }

    Rectangle panel_rec = ui_EndPanel();

    //
    // NEW PANNEL
    //

    panel_pos.x += panel_rec.width + 10;

    ui_StartPanel(panel_pos, layout);

    //
    // RANDOM INFO SECTION
    //
    ui_AddSeparator(3);
    ui_AddTextNode("INFO", font_sizes.title);
    ui_AddSeparator(3);

    snprintf(buffer, sizeof(buffer), "Mouse pos: %.0f, %.0f", mouse_pos.x, mouse_pos.y);
    ui_AddTextNode(buffer, font_sizes.text);

    if (is_hovered_coords_valid) {
        snprintf(buffer, sizeof(buffer), "Hovered coords: %d, %d", hovered_coords.x, hovered_coords.y);
        ui_AddTextNode(buffer, font_sizes.text);
    } else {
        ui_AddTextNode("Hovered coords: NONE", font_sizes.text);
    }

    ui_EndPanel();
}

void editor_Draw() {
    if (state.show_scene_panel) {
        DrawPathPanel();
    }
}
