#pragma once

#include "types.h"

namespace GDB {

void Init(u16 port);
void Shutdown();

bool HandleClientRequest();

bool ServerRunning();
bool KeepReceiving();
}
