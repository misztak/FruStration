#include "bios.h"

BIOS::BIOS() {}

void BIOS::TraceFunction(u32 address, u32 index) {
    if      (address == 0xA0 && index < 0xBF) printf("[BIOS] A function %s\n", bios_functions_A[index]);
    else if (address == 0xB0 && index < 0x5E) printf("[BIOS] B function %s\n", bios_functions_B[index]);
    else if (address == 0xC0 && index < 0x1E) printf("[BIOS] C function %s\n", bios_functions_C[index]);
}

void BIOS::PutChar(s8 value) {
    stdout_buffer.push_back(value);
    if (value == '\n') {
        printf("[TTY] %s", stdout_buffer.c_str());
        stdout_buffer.clear();
    }
}
