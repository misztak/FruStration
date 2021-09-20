#pragma once

#include "types.h"

class Debugger;

namespace GDB {

void Init(u16 port, Debugger* debugger);
void Shutdown();

bool HandleClientRequest();

bool ServerRunning();
bool KeepReceiving();
}
