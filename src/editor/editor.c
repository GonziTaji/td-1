#include "../game/game.h"
#include "../game/scene/scene_data.h"
#include "../game/scene/view_mamanger.h"
#include "../game/tower/towers_data.h"
#include "../game/wave/wave.h"
#include "../utils/grid.h"
#include "editor_ui.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/**
 *
 * General Editor.
 *
 * The editor should only control data from each module,
 * and each module react to that data change
 *
 */

typedef struct {
    int id;
    char *label;
    int *value;
} InputData;

typedef enum {
    EDITOR_STATUS_NORMAL,
    EDITOR_STATUS_WAYPOINT_SELECTION,
} EditorStatus;

typedef struct {
    // general state
    float game_scale;
    Vector2 game_scale_v;
    Vector2 mouse_game_pos;
    V2i hovered_coords;
    bool is_hovered_coords_valid;
    int last_element_id;
    int tower_selected_id;

    // scene panel state
    bool show_scene_panel;
    V2i prev_waypoints[SCENE_DATA_MAX_WAYPOINTS];
    int prev_waypoints_count;
    int active_input_id;
    EditorStatus path_editor_status;
    int wave_selected_idx;

} EditorState;

static const EditorState editor_initial_state = {
    .game_scale = 0,
    .game_scale_v = {0, 0},
    .mouse_game_pos = {0, 0},
    .hovered_coords = {0, 0},
    .is_hovered_coords_valid = false,
    .last_element_id = 0,
    .tower_selected_id = 0,

    .show_scene_panel = true,
    .prev_waypoints = {},
    .prev_waypoints_count = 0,
    .active_input_id = 0,
    .path_editor_status = EDITOR_STATUS_NORMAL,
    .wave_selected_idx = 0,
};

static EditorState editor_state = editor_initial_state;

int getId() {
    editor_state.last_element_id += 1;
    return editor_state.last_element_id;
}

void editor_Update(float delta_time) {
    if (IsKeyPressed(KEY_U)) {
        editor_state.show_scene_panel = !editor_state.show_scene_panel;
    }

    // Reset focus for this frame if /anything/ was clicked
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        editor_state.active_input_id = editor_initial_state.active_input_id;
    }
}

static char buffer[EDITOR_UI_MAX_CHARS_TEXT_NODE];

static const struct {
    int title;
    int subtitle;
    int text;
    int button;
    int input;
} font_sizes = {
    .title = 24,
    .subtitle = 18,
    .text = 16,
    .button = 16,
    .input = 16,
};

static const UILayout default_layout = {
    .padding = 20,
    .gap = 10,
    .direction = LAYOUT_DIR_COL,
};

/// returns the final panel aabb
static Rectangle DrawScenePanel(Vector2 panel_origin) {
    ui_StartPanel(panel_origin, default_layout);

    //
    // SCENE SECTION
    //
    snprintf(buffer, sizeof(buffer), "SCENE %d:", SCENE_DATA->id);
    ui_AddTextNode(buffer, font_sizes.title);
    snprintf(buffer, sizeof(buffer), "%s", SCENE_DATA->name);
    ui_AddTextNode(buffer, font_sizes.subtitle);

    if (ui_AddButton("Restore saved data", font_sizes.button)) {
        scene_data_ReloadCurrentScene();
        wave_initData();
        editor_state = editor_initial_state;
    }

    ui_AddSeparator(1);

    snprintf(buffer, sizeof(buffer), "cols = %d, rows = %d", SCENE_DATA->cols, SCENE_DATA->rows);
    ui_AddTextNode(buffer, font_sizes.subtitle);

    int action = ui_AddToolbar(4,
        (char *[]){
            "+ COL",
            "- COL",
            "+ ROW",
            "- ROW",
        },
        font_sizes.button);

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

    //
    // PATH SECTION
    //

    ui_AddSeparator(1);

    snprintf(buffer, sizeof(buffer), "waypoints (%d)", SCENE_DATA->pathWaypointsCount);
    ui_AddTextNode(buffer, font_sizes.subtitle);

    if (editor_state.path_editor_status == EDITOR_STATUS_NORMAL) {
        if (ui_AddButton("Edit waypoints", font_sizes.button)) {
            editor_state.path_editor_status = EDITOR_STATUS_WAYPOINT_SELECTION;

            memcpy(editor_state.prev_waypoints, SCENE_DATA->pathWaypoints, sizeof(SCENE_DATA->pathWaypoints));
            editor_state.prev_waypoints_count = SCENE_DATA->pathWaypointsCount;

            wave_StopWaves();
        }
    } else if (editor_state.path_editor_status == EDITOR_STATUS_WAYPOINT_SELECTION) {
        ui_AddTextNode("Click on a tile to set a new waypoint", font_sizes.text);

        if (ui_AddButton("Remove last waypoint", font_sizes.button)) {
            scene_data_RemoveLastWaypoint();
        }

        for (int end_idx = 1; end_idx < SCENE_DATA->pathWaypointsCount; end_idx++) {
            const V2i *start = &SCENE_DATA->pathWaypoints[end_idx - 1];
            const V2i *end = &SCENE_DATA->pathWaypoints[end_idx];
            snprintf(buffer, sizeof(buffer), "%d: %d, %d > %d %d", end_idx, start->x, start->y, end->x, end->y);
            ui_AddTextNode(buffer, font_sizes.text);
        }

        action = ui_AddToolbar(2, (char *[]){"Confirm", "Cancel"}, font_sizes.button);

        switch (action) {
        case 0:
            editor_state.path_editor_status = EDITOR_STATUS_NORMAL;
            wave_initData();
            break;

        case 1:
            scene_data_ReplaceWaypoints(editor_state.prev_waypoints, editor_state.prev_waypoints_count);
            editor_state.path_editor_status = EDITOR_STATUS_NORMAL;
            wave_initData();
            break;
        }

        if (editor_state.is_hovered_coords_valid) {
            const V2i *last_waypoint = &SCENE_DATA->pathWaypoints[SCENE_DATA->pathWaypointsCount - 1];
            Vector2 source = grid_getTileCenter(SCENE_TRANSFORM, last_waypoint->x, last_waypoint->y);
            Vector2 dest
                = grid_getTileCenter(SCENE_TRANSFORM, editor_state.hovered_coords.x, editor_state.hovered_coords.y);

            source = Vector2Multiply(source, editor_state.game_scale_v);
            dest = Vector2Multiply(dest, editor_state.game_scale_v);

            Color line_color = scene_data_WaypointCanBeSet(editor_state.hovered_coords) ? GREEN : RED;
            DrawLineEx(source, dest, 10, line_color);

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                scene_data_AddWaypoint(editor_state.hovered_coords);
            }
        }
    }

    //
    // WAVE SECTION
    //
    snprintf(buffer, sizeof(buffer), "Waves (%d)", SCENE_DATA->wavesCount);

    ui_AddSeparator(1);
    ui_AddTextNode(buffer, font_sizes.subtitle);

    if (ui_AddButton("Restart waves", font_sizes.button)) {
        wave_initData();
    }

    action = ui_AddToolbar(4, (char *[]){"< PREV", "NEXT >", "ADD", "RM"}, font_sizes.button);

    switch (action) {
    case 0:
        if (editor_state.wave_selected_idx != 0) {
            editor_state.wave_selected_idx--;
        }
        break;
    case 1:
        if (editor_state.wave_selected_idx < SCENE_DATA->wavesCount - 1) {
            editor_state.wave_selected_idx++;
        }
        break;
    case 2:
        scene_data_AddWave(0, 0, 0);
        break;
    case 3:
        scene_data_RemoveWave(editor_state.wave_selected_idx);
        if (editor_state.wave_selected_idx == SCENE_DATA->wavesCount) {
            editor_state.wave_selected_idx--;
        }
        break;
    }

    snprintf(buffer, sizeof(buffer), "Wave %d of %d:", editor_state.wave_selected_idx + 1, SCENE_DATA->wavesCount);
    ui_AddTextNode(buffer, font_sizes.text);

    WaveData *wave = scene_data_GetMutableWave(editor_state.wave_selected_idx);

    InputData wave_inputs[] = {
        {getId(), "Start delay", &wave->startDelaySeconds},
        {getId(), "Mob type", (int *)&wave->mobType},
        {getId(), "Mobs quantity", &wave->mobsCount},
    };

    const int wave_values_count = sizeof(wave_inputs) / sizeof(InputData);

    for (int i = 0; i < wave_values_count; i++) {
        const InputData *input = &wave_inputs[i];
        const bool is_edit_mode = editor_state.active_input_id == input->id;

        if (ui_AddValueBox(input->label, input->value, font_sizes.input, is_edit_mode)) {
            editor_state.active_input_id = input->id;
        }
    }

    return ui_EndPanel();
}

Rectangle DrawTowersPanel(Vector2 panel_position) {
    ui_StartPanel(panel_position, default_layout);

    ui_AddTextNode("Towers", font_sizes.title);

    const TowerBaseData *tower_data;
    TowerAttributes *tower_attrs;

    const int tower_types_count = tower_data_getTowerTypeCount();

    for (int type_id = 0; type_id < tower_types_count; type_id++) {
        tower_data = tower_data_getDataByIndex(type_id);
        tower_attrs = tower_data_GetMutableTowerAttributes(type_id);

        // Color tower_color;
        // Color bullet_color;
        // int bullet_width;

        ui_AddSeparator(1);

        if (editor_state.tower_selected_id != type_id) {
            snprintf(buffer, sizeof(buffer), "[+] %s", tower_data->name);

            if (ui_AddButton(buffer, font_sizes.button)) {
                editor_state.tower_selected_id = type_id;
            }

            continue;
        }

        snprintf(buffer, sizeof(buffer), "%s", tower_data->name);
        ui_AddTextNode(buffer, font_sizes.subtitle);

        for (TowerAttributeType attr_type = 0; attr_type < TOWER_ATTR_COUNT; attr_type++) {
            snprintf(buffer, sizeof(buffer), "%s", tower_modifiers_data_GetAttrLabel(attr_type));

            const int element_id = getId();
            const bool is_edit_mode = editor_state.active_input_id == element_id;

            switch (attr_type) {
            case TOWER_ATTR_DAMAGE:
            case TOWER_ATTR_RANGE:
            case TOWER_ATTR_BULLET_SPEED:
            case TOWER_ATTR_MULTISHOT: {
                int int_value = tower_attrs->values[attr_type];
                if (ui_AddValueBox(buffer, &int_value, font_sizes.text, is_edit_mode)) {
                    editor_state.active_input_id = element_id;
                }

                tower_attrs->values[attr_type] = int_value;

                break;
            }

            case TOWER_ATTR_RATE_OF_FIRE:
            case TOWER_ATTR_CRIT_CHANCE_PERCENT: {
                int int_value = tower_attrs->values[attr_type] * 100;
                if (ui_AddValueBox(buffer, &int_value, font_sizes.text, is_edit_mode)) {
                    editor_state.active_input_id = element_id;
                }

                tower_attrs->values[attr_type] = (float)int_value / 100;

                break;
            }

            case TOWER_ATTR_COUNT:
                assert(false && "Invalid tower attribute type");
                break;
            }
        }
    }

    return ui_EndPanel();
}

Rectangle DrawInfoPanel(Vector2 panel_origin) {
    ui_StartPanel(panel_origin, default_layout);

    //
    // RANDOM INFO SECTION
    //
    ui_AddSeparator(1);
    ui_AddTextNode("INFO", font_sizes.title);

    snprintf(buffer,
        sizeof(buffer),
        "Mouse pos: %.0f, %.0f",
        editor_state.mouse_game_pos.x,
        editor_state.mouse_game_pos.y);
    ui_AddTextNode(buffer, font_sizes.text);

    if (editor_state.is_hovered_coords_valid) {
        snprintf(buffer,
            sizeof(buffer),
            "Hovered coords: %d, %d",
            editor_state.hovered_coords.x,
            editor_state.hovered_coords.y);
        ui_AddTextNode(buffer, font_sizes.text);
    } else {
        ui_AddTextNode("Hovered coords: NONE", font_sizes.text);
    }

    return ui_EndPanel();
}

void editor_Draw() {
    editor_state.last_element_id = editor_initial_state.last_element_id;
    editor_state.game_scale = (float)GetScreenWidth() / GAME_VIEW_WIDTH;
    editor_state.game_scale_v = (Vector2){editor_state.game_scale, editor_state.game_scale};

    editor_state.mouse_game_pos = Vector2Divide(GetMousePosition(), editor_state.game_scale_v);
    editor_state.hovered_coords
        = grid_worldPointToCoords(SCENE_TRANSFORM, editor_state.mouse_game_pos.x, editor_state.mouse_game_pos.y);
    editor_state.is_hovered_coords_valid
        = grid_isValidCoords(SCENE_DATA->cols, SCENE_DATA->rows, editor_state.hovered_coords);

    Vector2 ui_cursor = {30, 30};
    Rectangle rec;

    if (editor_state.show_scene_panel) {
        rec = DrawScenePanel(ui_cursor);
        ui_cursor = RectangleGetPosition(rec);
        ui_cursor.x += rec.width + 10;

        rec = DrawTowersPanel(ui_cursor);
        ui_cursor = RectangleGetPosition(rec);
        ui_cursor.x += rec.width + 10;
    }

    DrawInfoPanel(ui_cursor);
}
