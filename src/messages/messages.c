#include "messages.h"
#include "../core/asset_manager.h"
#include <assert.h>
#include <stdio.h>

const bool debugMessageInfo = true;

static void outputMessageInfo(Message m) {
    if (m.type == MESSAGE_NONE) {
        return;
    }

    printf("[DISPATCHED MESSAGE]: %d\n", m.type);

    // output params
    // switch (m.type) {
    // case MESSAGE_CMD_GAMEPLAY_SPEED_CHANGE:
    //     printf("    [args.selection]: %d\n", m.args.selection);
    //     break;
    //
    // default:
    //     break;
    // }
}

bool messages_dispatchMessage(Message msg, Game *g) {
    if (debugMessageInfo) {
        outputMessageInfo(msg);
    }

    // switch (msg.type) {
    // }

    return true;
}
