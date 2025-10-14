#pragma once

#include "../messages/messages.h"

#define INPUT_MAP_CAPACITY 64

Message keyMap_processInput();
void keyMap_registerCommand(int key, Message cmd);
