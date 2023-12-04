#pragma once

#include <vector>

#include "util/types.h"

class System;

namespace BIOS {

void PutChar(u8 value);

bool IsPatched();

bool PatchBIOSForPSEXEInjection(std::vector<u8>& bios_image, u32 pc, u32 gp, u32 sp, u32 fp);

void TraceFunction(System* sys, u32 address, u32 index);

}    //namespace BIOS

