#include "CPU.hpp"

#include "BUS.hpp"

namespace CPU {

CPU::CPU() = default;

void CPU::Init(BUS* b) { bus = b; }

void CPU::Step() {
    u32 instr = Load32(sp.pc);
    printf("Executing instruction 0x%08X\n", instr);

    switch (op(instr)) {
        case PrimaryOpcode::ori:
            SetReg(dst(instr), GetReg(src(instr)) | imm(instr));
            break;
        case PrimaryOpcode::lui:
            SetReg(dst(instr), imm(instr) << 16);
            break;
        default:
            Panic("Unimplemented opcode 0x%02X [0x%08X]", op(instr), instr);
    }

    sp.pc += 4;
}

u32 CPU::Load32(u32 address) { return bus->Load32(address); }

void CPU::Store32(u32 address, u32 value) { bus->Store32(address, value); }

void CPU::SetReg(u32 index, u32 value) {
    Assert(index < 32);
    gp.r[index] = value;
}

u32 CPU::GetReg(u32 index) {
    Assert(index < 32);
    return gp.r[index];
}

}  // namespace CPU