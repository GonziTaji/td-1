#include "../game/game.h"
#include "../game/scene/scene_data.h"
#include "../game/scene/view_mamanger.h"
#include "../game/tower/tower.h"
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

static const UILayout base_layout = {
    .padding = 20,
    .gap = 10,
    .direction = LAYOUT_DIR_COL,
};

typedef enum {
    TAB_TYPE_SCENE,
    TAB_TYPE_TOWERS,
    TAB_TYPE_COUNT,
} TabType;

struct {
    char *labels[TAB_TYPE_COUNT];
    TabType tab_selected;
} tabs_state = {
    .tab_selected = TAB_TYPE_SCENE,
    .labels = {"Scene", "Towers"},
};

static Rectangle DrawTabsHeader(Vector2 position) {
    UILayout layout = base_layout;
    layout.padding = 2;

    ui_StartPanel(position, layout);

    const int active_tab_button = ui_AddToolbar(TAB_TYPE_COUNT, tabs_state.labels, font_sizes.subtitle);

    if (active_tab_button != -1) {
        tabs_state.tab_selected = active_tab_button;
    }

    return ui_EndPanel();
}

/// returns the final panel aabb
static Rectangle DrawScenePanel(Vector2 panel_origin) {
    ui_StartPanel(panel_origin, base_layout);

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

        if (ui_AddIntInput(input->label, input->value, font_sizes.input, is_edit_mode)) {
            editor_state.active_input_id = input->id;
        }
    }

    return ui_EndPanel();
}

Rectangle DrawTowersPanel(Vector2 panel_position) {
    ui_StartPanel(panel_position, base_layout);

    ui_AddTextNode("Towers", font_sizes.title);

    if (ui_AddButton("Restore saved tower data", font_sizes.button)) {
        tower_data_load();
    }

    ui_AddSeparator(1);

    ui_AddTextNode("Tower list", font_sizes.subtitle);

    ui_AddSeparator(1);

    const int tower_types_count = tower_data_getTowerTypeCount();

    for (int type_id = 0; type_id < tower_types_count; type_id++) {
        if (type_id == editor_state.tower_selected_id) {
            snprintf(buffer, sizeof(buffer), "%s >>", tower_data_GetMutableTowerData(type_id)->name);
        } else {
            snprintf(buffer, sizeof(buffer), "%s", tower_data_GetMutableTowerData(type_id)->name);
        }

        if (ui_AddButton(buffer, font_sizes.button)) {
            editor_state.tower_selected_id = type_id;
        }
    }

    ui_AddSeparator(1);

    if (ui_AddButton("Add new tower", font_sizes.button)) {
        editor_state.tower_selected_id = tower_data_CreateNewTowerType();
    }

    const Rectangle main_panel_rec = ui_EndPanel();
    Vector2 selected_tower_panel_origin = {main_panel_rec.x + main_panel_rec.width, main_panel_rec.y};

    ui_StartPanel(selected_tower_panel_origin, base_layout);

    TowerBaseData *tower_data = tower_data_GetMutableTowerData(editor_state.tower_selected_id);
    TowerAttributes *tower_attrs = &tower_data->attributes;

    ui_AddTextNode("Tower selected:", font_sizes.subtitle);
    ui_AddSeparator(1);

    const int element_id = getId();
    const bool is_edit_mode = editor_state.active_input_id == element_id;

    if (ui_AddTextInput("Name: ", tower_data->name, font_sizes.subtitle, is_edit_mode)) {
        editor_state.active_input_id = element_id;
    }

    // Color tower_color;
    // Color bullet_color;
    // int bullet_width;

    for (TowerAttributeType attr_type = 0; attr_type < TOWER_ATTR_COUNT; attr_type++) {
        snprintf(buffer, sizeof(buffer), "%s: ", tower_modifiers_data_GetAttrLabel(attr_type));

        const int element_id = getId();
        const bool is_edit_mode = editor_state.active_input_id == element_id;

        switch (attr_type) {
        case TOWER_ATTR_DAMAGE:
        case TOWER_ATTR_RANGE:
        case TOWER_ATTR_BULLET_SPEED:
        case TOWER_ATTR_MULTISHOT: {
            int int_value = tower_attrs->values[attr_type];
            if (ui_AddIntInput(buffer, &int_value, font_sizes.text, is_edit_mode)) {
                editor_state.active_input_id = element_id;
            }

            tower_attrs->values[attr_type] = int_value;

            break;
        }

        case TOWER_ATTR_RATE_OF_FIRE:
        case TOWER_ATTR_CRIT_CHANCE_PERCENT: {
            int int_value = tower_attrs->values[attr_type] * 100;

            if (ui_AddIntInput(buffer, &int_value, font_sizes.text, is_edit_mode)) {
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

    if (tower_types_count > 1 && ui_AddButton("Delete tower", font_sizes.button)) {
        towers_clear();
        tower_data_RemoveTowerData(editor_state.tower_selected_id);
    }

    Rectangle tower_editor_panel_rec = ui_EndPanel();

    tower_editor_panel_rec.y = main_panel_rec.y;

    return tower_editor_panel_rec;
}

Rectangle DrawInfoPanel(Vector2 panel_origin) {
    ui_StartPanel(panel_origin, base_layout);

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

const Vector2 initial_ui_cursor = {20, 20};

void editor_Draw() {
    editor_state.last_element_id = editor_initial_state.last_element_id;
    editor_state.game_scale = (float)GetScreenWidth() / GAME_VIEW_WIDTH;
    editor_state.game_scale_v = (Vector2){editor_state.game_scale, editor_state.game_scale};

    editor_state.mouse_game_pos = Vector2Divide(GetMousePosition(), editor_state.game_scale_v);
    editor_state.hovered_coords
        = grid_worldPointToCoords(SCENE_TRANSFORM, editor_state.mouse_game_pos.x, editor_state.mouse_game_pos.y);
    editor_state.is_hovered_coords_valid
        = grid_isValidCoords(SCENE_DATA->cols, SCENE_DATA->rows, editor_state.hovered_coords);

    Vector2 ui_cursor = initial_ui_cursor;

    Rectangle rec = DrawTabsHeader(ui_cursor);

    Vector2 info_panel_origin = {rec.x + rec.width + 2, rec.y};

    ui_cursor.y += rec.height + 10;

    switch (tabs_state.tab_selected) {
    case TAB_TYPE_SCENE:
        rec = DrawScenePanel(ui_cursor);
        break;
    case TAB_TYPE_TOWERS:
        rec = DrawTowersPanel(ui_cursor);
        break;
    case TAB_TYPE_COUNT:
        break;
    }

    info_panel_origin.x = MAX(info_panel_origin.x, rec.x + rec.width + 10);

    DrawInfoPanel(info_panel_origin);
}
