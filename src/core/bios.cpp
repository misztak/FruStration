#include "bios.h"

#include "bus.h"
#include "cpu.h"
#include "log.h"
#include "asserts.h"
#include "system.h"

LOG_CHANNEL(BIOS);

BIOS::BIOS(System* system) : sys(system) {}

void BIOS::TraceFunction(u32 address, u32 index) {
    if (address == 0xA0 && index < 0xBF)
        LogDebug("A function {}", bios_functions_A[index]);
    else if (address == 0xB0 && index < 0x5E)
        LogDebug("B function {}", bios_functions_B[index]);
    else if (address == 0xC0 && index < 0x1E)
        LogDebug("C function {}", bios_functions_C[index]);
}

template<u32 arg_count>
void BIOS::PrintFnWithArgs(const char* format_string) {
    static_assert(arg_count < 7);

    const auto& gp_regs = sys->cpu->gp;

    if constexpr (arg_count == 0) {
        LogInfo(format_string);
    }
    if constexpr (arg_count == 1) {
        LogInfo(format_string, gp_regs.a0);
    }
    if constexpr (arg_count == 2) {
        LogInfo(format_string, gp_regs.a0, gp_regs.a1);
    }
    if constexpr (arg_count == 3) {
        LogInfo(format_string, gp_regs.a0, gp_regs.a1, gp_regs.a2);
    }
    if constexpr (arg_count == 4) {
        LogInfo(format_string, gp_regs.a0, gp_regs.a1, gp_regs.a2, gp_regs.a3);
    }
    if constexpr (arg_count == 5) {
        u32 arg_4 = sys->bus->Peek32(gp_regs.sp + 16);
        LogInfo(format_string, gp_regs.a0, gp_regs.a1, gp_regs.a2, gp_regs.a3, arg_4);
    }
    if constexpr (arg_count == 6) {
        u32 arg_4 = sys->bus->Peek32(gp_regs.sp + 16);
        u32 arg_5 = sys->bus->Peek32(gp_regs.sp + 20);
        LogInfo(format_string, gp_regs.a0, gp_regs.a1, gp_regs.a2, gp_regs.a3, arg_4, arg_5);
    }
}

#define FN_WITH_ARGS(args, definition)     \
    PrintFnWithArgs<(args)>((definition)); \
    break

void BIOS::TraceAFunction(u32 index) {
    switch (index) {
        case 0x00: FN_WITH_ARGS(2, "open(filename=0x{:08X}, accessmode=0x{:08X})");
        case 0x01: FN_WITH_ARGS(3, "lseek(fd={}, offset={}, seektype=0x{:08X})");
        case 0x2F: FN_WITH_ARGS(0, "rand()");
        default: Panic("Invalid A function index %u", index);
    }
}

#undef FN_WITH_ARGS

void BIOS::PutChar(s8 value) {
    if (value == '\n') {
        LogInfo("TTY: {}", stdout_buffer);
        stdout_buffer.clear();
    } else {
        stdout_buffer.push_back(value);
    }
}
