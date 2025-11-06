#include "../game/scene/scene_data.h"
#include "editor_ui.h"
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>

static struct {
    bool show_scene_panel;
} state = {
    .show_scene_panel = true,
};

void editor_Update(float delta_time) {
    if (IsKeyPressed(KEY_U)) {
        state.show_scene_panel = !state.show_scene_panel;
    }
}

static void DrawPathPanel() {
    const UILayout layout = {
        .padding = 20,
        .gap = 10,
        .direction = LAYOUT_DIR_COL,
    };

    ui_StartPanel((Vector2){30, 30}, layout);

    const V2i *waypoints = SCENE_DATA->pathWaypoints;
    const int waypoint_count = SCENE_DATA->pathWaypointsCount;

    char buffer[EDITOR_UI_MAX_CHARS_TEXT_NODE];

    ui_AddTextNode("Path", 32);
    ui_AddSeparator(3);

    for (int i = 1; i < waypoint_count; i++) {
        snprintf(buffer,
            sizeof(buffer),
            "%d. (%d, %d) -> (%d, %d)",
            i,
            waypoints[i - 1].x,
            waypoints[i - 1].y,
            waypoints[i].x,
            waypoints[i].y);

        ui_AddTextNode(buffer, 32);
    }

    ui_EndPanel();
}

void editor_Draw() {
    if (state.show_scene_panel) {
        DrawPathPanel();
    }
}
