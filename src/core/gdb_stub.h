#pragma once

#include "types.h"
#include "debugger.h"

namespace GDB {

void Init(u16 port, Debugger* debugger);
void Shutdown();

bool HandleClientRequest();

bool ServerRunning();
bool KeepReceiving();
}
