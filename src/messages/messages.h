#pragma once

#include <raylib.h>
#include <stdbool.h>
typedef struct Game Game;

// Events and Commands are treated the same
typedef enum {
    MESSAGE_NONE = 0,
    MESSAGE_CMD_VIEW_MOVE,
    MESSAGE_CMD_VIEW_ZOOM_UP,
    MESSAGE_CMD_VIEW_ZOOM_DOWN,
    MESSAGE_CMD_VIEW_ZOOM_RESET,
} MessageType;

// used to have more members and will probably will have more members eventually
typedef union {
} MessageArgs;

typedef struct {
    MessageType type;
    MessageArgs args;
} Message;

bool messages_dispatchMessage(Message msg, Game *g);
