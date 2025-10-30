#include "./view_mamanger.h"
#include "../../input/input.h"
#include "../../utils/utils.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>

#define SCENE_SCALE_STEP 0.4f
#define SCENE_SCALE_MIN SCENE_SCALE_INITIAL - (2 * SCENE_SCALE_STEP)
#define SCENE_SCALE_MAX SCENE_SCALE_INITIAL + (5 * SCENE_SCALE_STEP)

const Vector2 SCENE_INITIAL_POS = {200, 600};

Transform2D TRANSFORM = {
    .scale = SCENE_SCALE_INITIAL,
    .translation = SCENE_INITIAL_POS,
    .previousTranslation = SCENE_INITIAL_POS,
};

const Transform2D *const SCENE_TRANSFORM = &TRANSFORM;

static void resetZoomView() {
    TRANSFORM.scale = SCENE_SCALE_INITIAL;
}

static void zoomView(float amount) {
    float newScale = utils_clampf(SCENE_SCALE_MIN, SCENE_SCALE_MAX, TRANSFORM.scale + amount);
    if (newScale != TRANSFORM.scale) {
        TRANSFORM.scale = newScale;
    }
}

static void moveView(Vector2 delta) {
    TRANSFORM.translation.x -= delta.x;
    TRANSFORM.translation.y -= delta.y;
}

void view_handleInput() {
    if (input.mouseWheelMove > 0) {
        return zoomView(SCENE_SCALE_STEP);

    } else if (input.mouseWheelMove < 0) {
        return zoomView(-SCENE_SCALE_STEP);
    }

    if (input.keyPressed == KEY_ZERO) {
        return resetZoomView();
    }

    if (input.mouseButtonState[MOUSE_BUTTON_RIGHT] == MOUSE_BUTTON_STATE_DRAGGING) {
        if (fabs(input.worldMouseDelta.x + input.worldMouseDelta.y) > 2) {
            return moveView(input.worldMouseDelta);
        }
    }
}

void view_update() {
    if (!Vector2Equals(TRANSFORM.previousTranslation, TRANSFORM.translation)) {
        TRANSFORM.previousTranslation = TRANSFORM.translation;
    }
}
