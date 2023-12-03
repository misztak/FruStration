#include "peripherals.h"

#include "common/asserts.h"
#include "common/log.h"
#include "system.h"

LOG_CHANNEL(Peripheral);

Peripherals::Peripherals(System *sys): sys(sys) {}

u32 Peripherals::Read(u32 address) {
    u32 rel_addr = address & 0xF;
    u32 value = 0;

    switch (rel_addr) {
        case 0x0:
            LogDebug("Read to JOY_DATA");
            value = 0;
            break;
        case 0x4:
            LogWarn("Read to JOY_STAT [Read-only]");
            //value = stat.bits;
            value = 0b11;
            break;
        case 0x8:
            LogDebug("Read to JOY_MODE");
            value = static_cast<u32>(mode.bits);
            break;
        case 0xA:
            LogDebug("Read to JOY_CTRL");
            value = static_cast<u32>(control.bits);
            break;
        case 0xE:
            LogDebug("Read to JOY_BAUD");
            value = static_cast<u32>(baudrate_reload);
            break;
        default:
            Panic("Invalid peripheral I/O address: 0x{:08X}", address);
    }

    return value;
}
void Peripherals::Write(u32 address, u32 value) {
    u32 rel_addr = address & 0xF;

    switch (rel_addr) {
        case 0x0:
            LogDebug("Write to JOY_DATA: 0x{:08X}", value);
            break;
        case 0x4:
            LogWarn("Attempted to write to JOY_STAT [Read-only]");
            break;
        case 0x8:
            LogDebug("Write to JOY_MODE: 0x{:04X}", static_cast<u16>(value));
            break;
        case 0xA:
            LogDebug("Write to JOY_CTRL: 0x{:04X}", static_cast<u16>(value));
            break;
        case 0xE:
            LogDebug("Write to JOY_BAUD: 0x{:04X}", static_cast<u16>(value));
            break;
        default:
            Panic("Invalid peripheral I/O address: 0x{:08X}", address);
    }
}

Controller& Peripherals::GetController1() {
    return controller;
}

void Peripherals::Reset() {
    stat.bits = 0;
    mode.bits = 0;
    control.bits = 0;

    baudrate_reload = 0;
}
