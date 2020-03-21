#include "cpu.h"

#include "bus.h"
#include "cpu_common.h"

namespace CPU {

CPU::CPU() = default;

void CPU::Init(BUS* b) { bus = b; }

void CPU::Step() {
    instr.value = next_instr.value;
    next_instr.value = Load32(sp.pc);
    printf("Executing instruction 0x%02X [0x%08X]\n", instr.n.op.GetValue(), instr.value);

    sp.pc += 4;

    switch (instr.n.op) {
        case PrimaryOpcode::special:
            switch (instr.s.sop) {
                case SecondaryOpcode::sll:
                    Set(instr.s.rd, Get(instr.s.rt) << instr.s.sa);
                    break;
                case SecondaryOpcode::orr:
                    Set(instr.s.rd, instr.s.rs | instr.s.rt);
                    break;
                case SecondaryOpcode::addu:
                    Set(instr.s.rd, Get(instr.s.rt) + Get(instr.s.rs));
                    break;
                case SecondaryOpcode::sltu:
                    Set(instr.s.rd, (Get(instr.s.rs) < Get(instr.s.rt)) ? 1 : 0);
                    break;
                default:
                    Panic("Unimplemented special opcode 0x%02X [0x%08X]", instr.s.sop.GetValue(), instr.value);
            }
            break;
        case PrimaryOpcode::jmp:
            sp.pc &= 0xF0000000;
            sp.pc |= instr.jump_target << 2;
            break;
        case PrimaryOpcode::bne:
            if (Get(instr.n.rs) != Get(instr.n.rt)) {
                sp.pc += instr.imm_se() << 2;
                // sp.pc -= 4;
            }
            break;
        case PrimaryOpcode::addi: {
            const u32 old = Get(instr.n.rs);
            const u32 add = instr.imm_se();
            const u32 result = old + add;
            if (!((old ^ add) & 0x80000000) && ((result ^ old) & 0x80000000)) {
                Panic("ADDI: Integer overflow!");
            }
            Set(instr.n.rt, result);
            break;
        }
        case PrimaryOpcode::addiu:
            Set(instr.n.rt, Get(instr.n.rs) + instr.imm_se());
            break;
        case PrimaryOpcode::ori:
            Set(instr.n.rt, Get(instr.n.rs) | instr.n.imm);
            break;
        case PrimaryOpcode::lui:
            Set(instr.n.rt, instr.n.imm << 16);
            break;
        case PrimaryOpcode::mtc0:
            SetCP0(instr.cop.rd, Get(instr.cop.rt));
            break;
        case PrimaryOpcode::lw: {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            if (cp.sr & 0x10000) break;
            SetDelayEntry(instr.n.rt, Load32(address));
            break;
        }
        case PrimaryOpcode::sw: {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            Store32(address, Get(instr.n.rt));
            break;
        }
        default:
            Panic("Unimplemented opcode 0x%02X [0x%08X]", instr.n.op.GetValue(), instr.value);
    }

    UpdateDelayEntries();
}

u32 CPU::Load32(u32 address) { return bus->Load32(address); }

void CPU::Store32(u32 address, u32 value) {
    if (cp.sr & 0x10000) return;
    bus->Store32(address, value);
}

void CPU::Set(u32 index, u32 value) {
    Assert(index < 32);
    if (index == 0) return;
    if (delay_entries[0].reg == index) {
        delay_entries[0].reg = 0;
        delay_entries[0].value = 0;
    }
    gp.r[index] = value;
}

u32 CPU::Get(u32 index) {
    Assert(index < 32);
    return gp.r[index];
}

void CPU::SetCP0(u32 index, u32 value) {
    Assert(index < 16);
    Assert(value == 0 || (index == 12 && value == 0x10000));
    cp.cpr[index] = value;
}

u32 CPU::GetCP0(u32 index) {
    // TODO: behaviour for CP0 registers 16-63
    Assert(index < 16);
    return cp.cpr[index];
}

void CPU::SetDelayEntry(u32 reg, u32 value) {
    Assert(reg < 32);
    if (reg == 0) return;
    //if (delay_entries[0].reg == reg) {
    //    delay_entries[0].reg = 0;
    //    delay_entries[0].value = 0;
    //}

    delay_entries[1] = {reg, value};
}

void CPU::UpdateDelayEntries() {
    gp.r[delay_entries[0].reg] = delay_entries[0].value;
    delay_entries[0] = delay_entries[1];
    delay_entries[1] = {0, 0};
}

}  // namespace CPU