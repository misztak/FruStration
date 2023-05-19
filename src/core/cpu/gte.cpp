#include "gte.h"

#include "common/asserts.h"
#include "common/log.h"

LOG_CHANNEL(GTE);

void GTE::ExecuteCommand(u32 cmd) {
    LogDebug("COMMAND 0x{:02X}", cmd);
}

void GTE::SetReg(u32 index, u32 value) {
    DebugAssert(index < 64);

    LogDebug("Set register {} to 0x{:08X}", index, value);
}

u32 GTE::GetReg(u32 index) {
    DebugAssert(index < 64);

    LogDebug("Load from register {}", index);

    return 0;
}

void GTE::Reset() {}
