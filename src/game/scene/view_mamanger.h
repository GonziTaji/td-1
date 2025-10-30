#pragma once

#include "../../utils/utils.h"

#define SCENE_SCALE_INITIAL 5.0f

extern const Transform2D *const SCENE_TRANSFORM;

void view_handleInput();
void view_update();
