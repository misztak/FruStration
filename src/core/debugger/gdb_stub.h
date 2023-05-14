#pragma once

#include "util/types.h"

class Debugger;

namespace GDB {

void Init(u16 port, Debugger* debugger);
void Shutdown();

void HandleClientRequest();

bool ServerRunning();
}    //namespace GDB
