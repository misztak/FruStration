#include "gte.h"

#include "log.h"
#include "asserts.h"

LOG_CHANNEL(GTE);

void GTE::SetReg(u32 index, u32 value) {
    DebugAssert(index < 64);

    LogDebug("Set register {} to 0x{:08X}", index, value);
}

void GTE::Reset() {}
