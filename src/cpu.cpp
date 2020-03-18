#include "cpu.h"

#include "bus.h"
#include "cpu_common.h"

namespace CPU {

CPU::CPU() = default;

void CPU::Init(BUS* b) { bus = b; }

void CPU::Step() {
    instr.value = next_instr.value;
    next_instr.value = Load32(sp.pc);
    printf("Executing instruction 0x%08X\n", instr.value);

    sp.pc += 4;

    switch (instr.n.op) {
        case PrimaryOpcode::special:
            switch (instr.s.sop) {
                case SecondaryOpcode::sll:
                    Set(instr.s.rd, Get(instr.s.rt) << instr.s.sa);
                    break;
                default:
                    Panic("Unimplemented special opcode 0x%02X [0x%08X]", instr.s.sop.GetValue(), instr.value);
            }
            break;
        case PrimaryOpcode::jmp:
            sp.pc &= 0xF0000000;
            sp.pc |= instr.jump_target << 2;
            break;
        case PrimaryOpcode::addiu:
            Set(instr.n.rt, Get(instr.n.rs) + instr.imm_se());
            break;
        case PrimaryOpcode::ori:
            Set(instr.n.rt, Get(instr.n.rs) | instr.n.imm);
            break;
        case PrimaryOpcode::lui:
            Set(instr.n.rt, instr.n.imm << 16);
            break;
        case PrimaryOpcode::sw: {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            Store32(address, Get(instr.n.rt));
            break;
        }
        default:
            Panic("Unimplemented opcode 0x%02X [0x%08X]", instr.n.op.GetValue(), instr.value);
    }
}

u32 CPU::Load32(u32 address) { return bus->Load32(address); }

void CPU::Store32(u32 address, u32 value) { bus->Store32(address, value); }

void CPU::Set(u32 index, u32 value) {
    Assert(index < 32);
    if (index == 0) return;
    gp.r[index] = value;
}

u32 CPU::Get(u32 index) {
    Assert(index < 32);
    return gp.r[index];
}

}  // namespace CPU