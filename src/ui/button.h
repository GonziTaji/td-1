#pragma once
#include "../messages/messages.h"
#include <raylib.h>

enum ButtonType {
    BUTTON_TYPE_TEXT_LABEL,
    BUTTON_TYPE_SPRITE,
};

typedef union {
    struct {
        Texture2D sprite;
        Rectangle sourceRect;
    } icon;
    const char *label;
} ButtonContent;

typedef struct {
    enum ButtonType type;
    ButtonContent content;
    Message command;
} UIButton;
