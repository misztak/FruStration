#include "gte.h"

#include "fmt/format.h"

#include "debug_utils.h"

LOG_CHANNEL(GTE);

void GTE::SetReg(u32 index, u32 value) {
    DebugAssert(index < 64);

    LOG_DEBUG << fmt::format("Set register {} to 0x{:08X}", index, value);
}

void GTE::Reset() {}
