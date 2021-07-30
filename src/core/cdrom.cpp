#include "cdrom.h"

#include "macros.h"
#include "fmt/format.h"

LOG_CHANNEL(CDROM);

CDROM::CDROM() {}

void CDROM::Init() {}

void CDROM::Step() {

}

u8 CDROM::Load(u32 address) {
    LOG_DEBUG << fmt::format("Load call to CDROM [@ 0x{:01X}]", address);

    if (address == 0x0) {
        return status.value;
    }

    return 0;
}

void CDROM::Store(u32 address, u8 value) {
    LOG_DEBUG << fmt::format("Store call to CDROM [0x{:02X} @ 0x{:01}]", value, address);

    if (address == 0x0) {
        // only the index field is writable
        status.index = value & 0x3;
        return;
    }
}

void CDROM::Reset() {
    status.value = 0;
    command = 0;
    request = 0;
    for (auto& r: response_fifo) r = 0;
    interrupt_enable = 0;
    interrupt_flag = 0;
}
