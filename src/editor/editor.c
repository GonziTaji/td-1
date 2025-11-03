
#include "../core/asset_manager.h"
#include <raylib.h>

void editor_Update(float delta_time) {
}

void editor_DrawUI() {
    int w = GetScreenWidth();
    int h = uiFont.baseSize * 4;

    DrawRectangle(0, 0, w, h, (Color){0, 0, 0, 220});
}
