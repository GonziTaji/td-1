#include "key_map.h"
#include <assert.h>
#include <raylib.h>

typedef struct {
    Message command;
    int key;
} Keybind;

Keybind registeredCommands[INPUT_MAP_CAPACITY];
int registeredCommandsCount = 0;

void registerCommand(int key, Message cmd) {
    assert(registeredCommandsCount != INPUT_MAP_CAPACITY - 1
           && "Trying to register a command in a full keyMap");

    registeredCommands[registeredCommandsCount] = (Keybind){cmd, key};

    registeredCommandsCount++;
}

Message keyMap_processInput() {
    return (Message){MESSAGE_NONE};
}
