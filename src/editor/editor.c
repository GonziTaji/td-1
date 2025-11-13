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
    char *label;
    void *value;

    enum {
        INPUT_VALUE_TYPE_INT,
        INPUT_VALUE_TYPE_FLOAT,
        INPUT_VALUE_TYPE_TEXT,
        INPUT_VALUE_TYPE_SWITCH,
        INPUT_VALUE_TYPE_LIST,
    } type;

    char **option_labels;
    int options_count;
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
    int status_effect_selected_idx;

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
    .status_effect_selected_idx = 0,
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
    EDITOR_TAB_SCENE,
    EDITOR_TAB_TOWERS,
    EDITOR_TAB_TOWER_MODIFIERS,
    EDITOR_TAB_BULLET_MODIFIERS,
    EDITOR_TAB_STATUS_EFFECTS,
    EDITOR_TAB_COUNT,
} EditorTab;

// @review: move to editor state maybe?
struct {
    char *labels[EDITOR_TAB_COUNT];
    EditorTab tab_selected;
} tabs_state = {
    .tab_selected = EDITOR_TAB_SCENE,
    .labels = {"Scene", "Towers", "Tower mods", "Bullet mods", "Status effects"},
};

static Rectangle DrawTabsHeader(Vector2 position) {
    UILayout layout = base_layout;
    layout.padding = 2;

    ui_StartPanel(position, layout);

    ui_AddSwitchButtons(EDITOR_TAB_COUNT, (int *)&tabs_state.tab_selected, tabs_state.labels, font_sizes.subtitle);

    return ui_EndPanel();
}

static Rectangle DrawTowerModsPanel(Vector2 position) {
    ui_StartPanel(position, base_layout);
    return ui_EndPanel();
}

static Rectangle DrawBulletModsPanel(Vector2 position) {
    ui_StartPanel(position, base_layout);

    return ui_EndPanel();
}

static Rectangle DrawStatusEffectsPanel(Vector2 position) {
    UIMasterDetailPanel layout;
    ui_MasterDetailBegin(&layout, position, base_layout.gap);

    ui_MasterDetailBeginMaster(&layout, base_layout);

    ui_AddTextNode("Status effects", font_sizes.title);

    int status_count = status_effect_data_GetEffectsCount();

    if (status_count == 0) {
        editor_state.status_effect_selected_idx = 0;
        ui_AddTextNode("No status effects available", font_sizes.subtitle);
    } else {
        char *effect_labels[EDITOR_UI_COMBOBOX_MAX_OPTIONS];
        const int capped_count = MIN(status_count, EDITOR_UI_COMBOBOX_MAX_OPTIONS);

        for (int idx = 0; idx < capped_count; idx++) {
            effect_labels[idx] = status_effect_data_GetMutableStatusData(idx)->name;
        }

        if (editor_state.status_effect_selected_idx >= capped_count) {
            editor_state.status_effect_selected_idx = capped_count - 1;
        }

        const int combobox_id = getId();

        if (ui_AddComboBox("Effect:",
                &editor_state.status_effect_selected_idx,
                effect_labels,
                capped_count,
                font_sizes.subtitle)) {

            editor_state.active_input_id = combobox_id;
        }
    }

    ui_MasterDetailEndMaster(&layout);

    status_count = status_effect_data_GetEffectsCount();

    ui_MasterDetailBeginDetail(&layout, base_layout);

    if (status_count == 0) {
        ui_AddTextNode("Add status effects in data to edit them here.", font_sizes.text);
        ui_MasterDetailEndDetail(&layout);
        return ui_MasterDetailGetBounds(&layout);
    }

    if (editor_state.status_effect_selected_idx < 0) {
        editor_state.status_effect_selected_idx = 0;
    }
    if (editor_state.status_effect_selected_idx >= status_count) {
        editor_state.status_effect_selected_idx = status_count - 1;
    }

    StatusEffect *effect = status_effect_data_GetMutableStatusData(editor_state.status_effect_selected_idx);

    InputData input_data[] = {
        {.label = "Name:", .value = &effect->name, .type = INPUT_VALUE_TYPE_TEXT},

        {.label = "Status effect type:",
            .value = &effect->type,
            .type = INPUT_VALUE_TYPE_LIST,
            .option_labels = status_effect_data_GetEffectTypeLabels(),
            .options_count = STATUS_EFFECT_TYPE_COUNT},

        {
            .label = "Duration type:",
            .value = &effect->duration_type,
            .type = INPUT_VALUE_TYPE_LIST,
            .option_labels = utils_GetAllDurationTypeLabels(),
            .options_count = DURATION_TYPE_COUNT,
        },

        {.label = "Value type:",
            .value = &effect->value_type,
            .type = INPUT_VALUE_TYPE_LIST,
            .option_labels = utils_GetModValueTypeLabels(),
            .options_count = MOD_VALUE_TYPE_COUNT},

        {.label = "Value:", &effect->value, INPUT_VALUE_TYPE_FLOAT},

        {.label = "Duration:", &effect->duration, INPUT_VALUE_TYPE_FLOAT},

        {.label = "Dot interval:", &effect->dot_interval, INPUT_VALUE_TYPE_FLOAT},
    };

    const int input_data_count = (int)(sizeof(input_data) / sizeof(input_data[0]));

    for (int j = 0; j < input_data_count; j++) {
        const int node_id = getId();
        const bool is_edit_mode = editor_state.active_input_id == node_id;

        switch (input_data[j].type) {
        case INPUT_VALUE_TYPE_INT:
            if (ui_AddIntInput(input_data[j].label, input_data[j].value, font_sizes.subtitle, is_edit_mode)) {
                editor_state.active_input_id = node_id;
            }
            break;

        case INPUT_VALUE_TYPE_FLOAT:
            if (ui_AddFloatInput(input_data[j].label, input_data[j].value, font_sizes.subtitle, is_edit_mode)) {
                editor_state.active_input_id = node_id;
            }
            break;

        case INPUT_VALUE_TYPE_TEXT:
            if (ui_AddTextInput(input_data[j].label, input_data[j].value, font_sizes.subtitle, is_edit_mode)) {
                editor_state.active_input_id = node_id;
            }
            break;

        case INPUT_VALUE_TYPE_SWITCH: {
            ui_AddOptionsButton(input_data[j].label,
                input_data[j].value,
                input_data[j].option_labels,
                input_data[j].options_count,
                font_sizes.button);
            break;
        }
        case INPUT_VALUE_TYPE_LIST: {
            ui_AddComboBox(input_data[j].label,
                input_data[j].value,
                input_data[j].option_labels,
                input_data[j].options_count,
                font_sizes.button);
            break;
        }
        }
    }

    ui_MasterDetailEndDetail(&layout);

    return ui_MasterDetailGetBounds(&layout);
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
        {"Start delay", &wave->startDelaySeconds},
        {"Mob type", (int *)&wave->mobType},
        {"Mobs quantity", &wave->mobsCount},
    };

    const int wave_values_count = 3;

    int node_id;
    bool is_edit_mode;

    for (int i = 0; i < wave_values_count; i++) {

        const InputData *input = &wave_inputs[i];
        node_id = getId();
        is_edit_mode = editor_state.active_input_id == node_id;

        if (ui_AddIntInput(input->label, input->value, font_sizes.input, is_edit_mode)) {
            editor_state.active_input_id = node_id;
        }
    }

    return ui_EndPanel();
}

Rectangle DrawTowersPanel(Vector2 panel_position) {
    UIMasterDetailPanel layout;
    ui_MasterDetailBegin(&layout, panel_position, base_layout.gap);

    ui_MasterDetailBeginMaster(&layout, base_layout);

    ui_AddTextNode("Towers", font_sizes.title);

    if (ui_AddButton("Restore saved tower data", font_sizes.button)) {
        tower_data_load();
    }

    ui_AddSeparator(1);

    int tower_types_count = tower_data_getTowerTypeCount();

    if (tower_types_count == 0) {
        editor_state.tower_selected_id = 0;
        ui_AddTextNode("No towers defined", font_sizes.subtitle);
    } else {
        char *tower_labels[EDITOR_UI_COMBOBOX_MAX_OPTIONS];
        const int capped_count = MIN(tower_types_count, EDITOR_UI_COMBOBOX_MAX_OPTIONS);

        for (int type_id = 0; type_id < capped_count; type_id++) {
            tower_labels[type_id] = tower_data_GetMutableTowerData(type_id)->name;
        }

        if (editor_state.tower_selected_id >= capped_count) {
            editor_state.tower_selected_id = capped_count - 1;
        }

        const int combobox_id = getId();

        if (ui_AddComboBox("Tower:",
                &editor_state.tower_selected_id,
                tower_labels,
                capped_count,
                font_sizes.subtitle)) {

            editor_state.active_input_id = combobox_id;
        }
    }

    ui_AddSeparator(1);

    if (ui_AddButton("Add new tower", font_sizes.button)) {
        editor_state.tower_selected_id = tower_data_CreateNewTowerType();
    }

    ui_MasterDetailEndMaster(&layout);

    tower_types_count = tower_data_getTowerTypeCount();

    ui_MasterDetailBeginDetail(&layout, base_layout);

    if (tower_types_count == 0) {
        ui_AddTextNode("Add a tower type to start editing its data.", font_sizes.text);
        ui_MasterDetailEndDetail(&layout);
        return ui_MasterDetailGetBounds(&layout);
    }

    if (editor_state.tower_selected_id < 0) {
        editor_state.tower_selected_id = 0;
    }
    if (editor_state.tower_selected_id >= tower_types_count) {
        editor_state.tower_selected_id = tower_types_count - 1;
    }
    TowerBaseData *tower_data = tower_data_GetMutableTowerData(editor_state.tower_selected_id);
    TowerAttributes *tower_attrs = &tower_data->attributes;

    ui_AddTextNode("Tower selected:", font_sizes.subtitle);
    ui_AddSeparator(1);

    const int element_id = getId();
    const bool is_edit_mode = editor_state.active_input_id == element_id;

    if (ui_AddTextInput("Name: ", tower_data->name, font_sizes.subtitle, is_edit_mode)) {
        editor_state.active_input_id = element_id;
    }

    for (TowerAttributeType attr_type = 0; attr_type < TOWER_ATTR_COUNT; attr_type++) {
        snprintf(buffer, sizeof(buffer), "%s: ", tower_mod_data_GetAttrLabel(attr_type));

        const int element_id = getId();
        const bool is_edit_mode = editor_state.active_input_id == element_id;

        switch (attr_type) {
        case TOWER_ATTR_DAMAGE:
        case TOWER_ATTR_RANGE:
        case TOWER_ATTR_BULLET_SPEED:
        case TOWER_ATTR_MULTISHOT: {
            int value = tower_attrs->values[attr_type];
            if (ui_AddIntInput(buffer, &value, font_sizes.text, is_edit_mode)) {
                editor_state.active_input_id = element_id;
            }

            tower_attrs->values[attr_type] = value;

            break;
        }

        case TOWER_ATTR_RATE_OF_FIRE:
        case TOWER_ATTR_CRIT_CHANCE_PERCENT: {
            if (ui_AddFloatInput(buffer, &tower_attrs->values[attr_type], font_sizes.text, is_edit_mode)) {
                editor_state.active_input_id = element_id;
            }

            break;
        }

        case TOWER_ATTR_COUNT:
            assert(false && "Invalid tower attribute type");
            break;
        }
    }

    // TODO: confirmation mechanism
    if (tower_types_count > 1 && ui_AddButton("Delete tower", font_sizes.button)) {
        towers_clear();
        tower_data_RemoveTowerData(editor_state.tower_selected_id);
    }

    ui_MasterDetailEndDetail(&layout);

    return ui_MasterDetailGetBounds(&layout);
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
    case EDITOR_TAB_SCENE:
        rec = DrawScenePanel(ui_cursor);
        break;
    case EDITOR_TAB_TOWERS:
        rec = DrawTowersPanel(ui_cursor);
        break;
    case EDITOR_TAB_TOWER_MODIFIERS:
        rec = DrawTowerModsPanel(ui_cursor);
        break;
    case EDITOR_TAB_BULLET_MODIFIERS:
        rec = DrawBulletModsPanel(ui_cursor);
        break;
    case EDITOR_TAB_STATUS_EFFECTS:
        rec = DrawStatusEffectsPanel(ui_cursor);
        break;

    case EDITOR_TAB_COUNT:
        assert(false && "Count enum value used as identifier");
        break;
    }

    info_panel_origin.x = MAX(info_panel_origin.x, rec.x + rec.width + 10);

    DrawInfoPanel(info_panel_origin);
}
