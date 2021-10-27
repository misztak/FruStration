#include "bios.h"

#include "debug_utils.h"

LOG_CHANNEL(BIOS);

BIOS::BIOS() {}

void BIOS::TraceFunction(u32 address, u32 index) {
    if      (address == 0xA0 && index < 0xBF) LOG_DEBUG << "A function " << bios_functions_A[index];
    else if (address == 0xB0 && index < 0x5E) LOG_DEBUG << "B function " << bios_functions_B[index];
    else if (address == 0xC0 && index < 0x1E) LOG_DEBUG << "C function " << bios_functions_C[index];
}

void BIOS::PutChar(s8 value) {
    if (value == '\n') {
        LOG_TTY << stdout_buffer;
        stdout_buffer.clear();
    } else {
        stdout_buffer.push_back(value);
    }
}
