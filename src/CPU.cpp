#include "CPU.hpp"

#include "BUS.hpp"

CPU::CPU() = default;

void CPU::Init(BUS* b) {
    bus = b;
}

void CPU::Step() {
    u32 instruction = Load32(pc);

    printf("Executing instruction 0x%08X\n", instruction);
    Panic("Unimplemented instruction")
}

u32 CPU::Load32(u32 address) {
    return bus->Load32(address);
}

void CPU::Store32(u32 address, u32 value) {
    bus->Store32(address, value);
}
