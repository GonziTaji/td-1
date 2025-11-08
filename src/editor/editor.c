#include "../game/game.h"
#include "../game/scene/scene_data.h"
#include "../game/scene/view_mamanger.h"
#include "../game/wave/wave.h"
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

int wave_editor_wave_idx = 0;

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

    Vector2 ui_origin = {30, 30};

    const float game_scale = (float)GetScreenWidth() / GAME_VIEW_WIDTH;
    const Vector2 game_scale_v = {game_scale, game_scale};

    const Vector2 mouse_game_pos = Vector2Divide(GetMousePosition(), game_scale_v);
    const V2i hovered_coords = grid_worldPointToCoords(SCENE_TRANSFORM, mouse_game_pos.x, mouse_game_pos.y);
    const bool is_hovered_coords_valid = grid_isValidCoords(SCENE_DATA->cols, SCENE_DATA->rows, hovered_coords);

    ui_StartPanel(ui_origin, layout);

    //
    // SCENE SECTION
    //
    snprintf(buffer, sizeof(buffer), "SCENE - cols/rows = %d / %d", SCENE_DATA->cols, SCENE_DATA->rows);

    ui_AddSeparator(3);
    ui_AddTextNode(buffer, font_sizes.title);
    ui_AddSeparator(3);

    int action = ui_AddToolbar(4, (char *[]){"+ COL", "- COL", "+ ROW", "- ROW"}, font_sizes.button);

    switch (action) {
    case 0:
        scene_data_ChangeGridDimensions(SCENE_DATA->cols + 1, SCENE_DATA->rows);
        break;
    case 1:
        scene_data_ChangeGridDimensions(SCENE_DATA->cols - 1, SCENE_DATA->rows);
        break;
    case 2:
        scene_data_ChangeGridDimensions(SCENE_DATA->cols, SCENE_DATA->rows + 1);
        break;
    case 3:
        scene_data_ChangeGridDimensions(SCENE_DATA->cols, SCENE_DATA->rows - 1);
        break;
    }

    if (ui_AddButton("Restore saved data", font_sizes.button)) {
        scene_data_ReloadCurrentScene();
        wave_initData();
    }

    //
    // PATH SECTION
    //
    snprintf(buffer, sizeof(buffer), "PATH - %d waypoints", SCENE_DATA->pathWaypointsCount);

    ui_AddSeparator(3);
    ui_AddTextNode(buffer, font_sizes.title);
    ui_AddSeparator(3);

    if (ui_AddButton("Remove last waypoint", font_sizes.button)) {
        scene_data_RemoveLastWaypoint();
        wave_initData();
    }

    if (path_editor_status == EDITOR_STATUS_NORMAL) {
        if (ui_AddButton("Add waypoints", font_sizes.button)) {
            path_editor_status = EDITOR_STATUS_WAYPOINT_SELECTION;
        }
    } else if (path_editor_status == EDITOR_STATUS_WAYPOINT_SELECTION) {
        ui_AddTextNode("Click on a tile to set the new waypoint", font_sizes.text);

        if (ui_AddButton("Cancel waypoint selection ", font_sizes.button)) {
            path_editor_status = EDITOR_STATUS_NORMAL;
        }

        if (is_hovered_coords_valid) {
            const V2i *last_waypoint = &SCENE_DATA->pathWaypoints[SCENE_DATA->pathWaypointsCount - 1];
            Vector2 source = grid_getTileCenter(SCENE_TRANSFORM, last_waypoint->x, last_waypoint->y);
            Vector2 dest = grid_getTileCenter(SCENE_TRANSFORM, hovered_coords.x, hovered_coords.y);

            source = Vector2Multiply(source, game_scale_v);
            dest = Vector2Multiply(dest, game_scale_v);

            Color line_color = scene_data_WaypointCanBeSet(hovered_coords) ? GREEN : RED;
            DrawLineEx(source, dest, 10, line_color);

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                scene_data_AddWaypoint(hovered_coords);
                wave_initData();
            }
        }
    }

    //
    // TODO: WAVE SECTION
    //
    snprintf(buffer, sizeof(buffer), "WAVES - %d waves", SCENE_DATA->wavesCount);

    ui_AddSeparator(3);
    ui_AddTextNode(buffer, font_sizes.title);
    ui_AddSeparator(3);

    if (ui_AddButton("Restart waves", font_sizes.button)) {
        wave_initData();
    }

    action = ui_AddToolbar(4, (char *[]){"< PREV", "NEXT >", "ADD", "RM"}, font_sizes.button);

    switch (action) {
    case 0:
        if (wave_editor_wave_idx != 0) {
            wave_editor_wave_idx--;
        }
        break;
    case 1:
        if (wave_editor_wave_idx < SCENE_DATA->wavesCount - 1) {
            wave_editor_wave_idx++;
        }
        break;
    case 2:
        scene_data_AddWave(0, 0, 0);
        break;
    case 3:
        scene_data_RemoveWave(wave_editor_wave_idx);
        if (wave_editor_wave_idx == SCENE_DATA->wavesCount) {
            wave_editor_wave_idx--;
        }
        break;
    }

    snprintf(buffer, sizeof(buffer), "Wave %d of %d:", wave_editor_wave_idx + 1, SCENE_DATA->wavesCount);
    ui_AddTextNode(buffer, font_sizes.text);

    const WaveData *wave = &SCENE_DATA->waves[wave_editor_wave_idx];

    snprintf(buffer, sizeof(buffer), "Start delay %d:", wave->startDelaySeconds);
    ui_AddTextNode(buffer, font_sizes.text);

    snprintf(buffer, sizeof(buffer), "Mob type %d:", wave->mobType);
    ui_AddTextNode(buffer, font_sizes.text);

    snprintf(buffer, sizeof(buffer), "Mobs count %d:", wave->mobsCount);
    ui_AddTextNode(buffer, font_sizes.text);

    //
    // NEW PANNEL
    //

    Rectangle panel_rec = ui_EndPanel();
    ui_origin.x += panel_rec.width + 10;

    ui_StartPanel(ui_origin, layout);

    //
    // RANDOM INFO SECTION
    //
    ui_AddSeparator(3);
    ui_AddTextNode("INFO", font_sizes.title);
    ui_AddSeparator(3);

    snprintf(buffer, sizeof(buffer), "Mouse pos: %.0f, %.0f", mouse_game_pos.x, mouse_game_pos.y);
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
