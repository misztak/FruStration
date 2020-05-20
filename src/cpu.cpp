#include "cpu.h"

#include "bus.h"
#include "cpu_common.h"

namespace CPU {

bool DISASM_INSTRUCTION = false;

CPU::CPU() : disassembler(this) {}

void CPU::Init(BUS* b) {
    bus = b;
    cp.prid = 0x2;
    UpdatePC(0xBFC00000);
}

void CPU::Step() {
    was_in_delay_slot = in_delay_slot;
    was_branch_taken = branch_taken;

    in_delay_slot = false;
    branch_taken = false;

    instr.value = Load32(sp.pc);

#ifdef DEBUG
    // printf("Executing instruction 0x%02X [0x%08X] at address 0x%08X\n",
    //       (instr.n.op == PrimaryOpcode::special) ? (u32)instr.s.sop.GetValue() : (u32)instr.n.op.GetValue(),
    //       instr.value, sp.pc - 0xBFC00000);
    if (DISASM_INSTRUCTION) disassembler.DisassembleInstruction(sp.pc, instr.value);
    static u64 instr_counter = 0; instr_counter++;
#endif

    UpdatePC(next_pc);
    // at this point the pc contains the address of the delay slot instruction
    // next_pc points to the instruction right after the delay slot

    if (current_pc & 0x3) {
        Exception(ExceptionCode::LoadAddress);
        return;
    }

    switch (instr.n.op) {
        case PrimaryOpcode::special:
            switch (instr.s.sop) {
                case SecondaryOpcode::sll:
                    Set(instr.s.rd, Get(instr.s.rt) << instr.s.sa);
                    break;
                case SecondaryOpcode::srl:
                    Set(instr.s.rd, Get(instr.s.rt) >> instr.s.sa);
                    break;
                case SecondaryOpcode::sra:
                    Set(instr.s.rd, static_cast<s32>(Get(instr.s.rt)) >> instr.s.sa);
                    break;
                case SecondaryOpcode::sllv:
                    Set(instr.s.rd, Get(instr.s.rt) << (Get(instr.s.rs) & 0x1F));
                    break;
                case SecondaryOpcode::srlv:
                    Set(instr.s.rd, Get(instr.s.rt) >> (Get(instr.s.rs) & 0x1F));
                    break;
                case SecondaryOpcode::srav:
                    Set(instr.s.rd, static_cast<s32>(Get(instr.s.rt)) >> (Get(instr.s.rs) & 0x1F));
                    break;
                case SecondaryOpcode::jr: {
                    u32 jump_address = Get(instr.s.rs);
                    if ((jump_address & 0x3) != 0) Panic("Unaligned return address!");
                    next_pc = jump_address;
                    in_delay_slot = true;
                    branch_taken = true;
                    break;
                }
                case SecondaryOpcode::jalr: {
                    u32 jump_address = Get(instr.s.rs);
                    if ((jump_address & 0x3) != 0) Panic("Unaligned return address!");
                    Set(instr.s.rd, next_pc);
                    next_pc = jump_address;
                    in_delay_slot = true;
                    branch_taken = true;
                    break;
                }
                case SecondaryOpcode::syscall:
                    Exception(ExceptionCode::Syscall);
                    break;
                case SecondaryOpcode::breakpoint:
                    Exception(ExceptionCode::Break);
                    break;
                case SecondaryOpcode::mfhi:
                    Set(instr.s.rd, sp.hi);
                    break;
                case SecondaryOpcode::mthi:
                    sp.hi = Get(instr.s.rs);
                    break;
                case SecondaryOpcode::mflo:
                    Set(instr.s.rd, sp.lo);
                    break;
                case SecondaryOpcode::mtlo:
                    sp.lo = Get(instr.s.rs);
                    break;
                case SecondaryOpcode::mult: {
                    const s64 a = static_cast<s32>(Get(instr.s.rs));
                    const s64 b = static_cast<s32>(Get(instr.s.rt));
                    const u64 result = static_cast<u64>(a * b);

                    sp.lo = result & 0xFFFFFFFF;
                    sp.hi = result >> 32;
                    break;
                }
                case SecondaryOpcode::multu: {
                    const u64 a = Get(instr.s.rs);
                    const u64 b = Get(instr.s.rt);
                    const u64 result = a * b;

                    sp.lo = result & 0xFFFFFFFF;
                    sp.hi = result >> 32;
                    break;
                }
                case SecondaryOpcode::div: {
                    const s32 n = static_cast<s32>(Get(instr.s.rs));
                    const s32 d = static_cast<s32>(Get(instr.s.rt));

                    // check for special cases
                    if (d == 0) {
                        sp.hi = static_cast<u32>(n);
                        sp.lo = (n >= 0) ? 0xFFFFFFFF : 0x1;
                    } else if (static_cast<u32>(n) == 0x80000000 && d == -1) {
                        sp.hi = 0x0;
                        sp.lo = 0x80000000;
                    } else {
                        sp.hi = static_cast<u32>(n % d);
                        sp.lo = static_cast<u32>(n / d);
                    }
                    break;
                }
                case SecondaryOpcode::divu: {
                    const u32 n = Get(instr.s.rs);
                    const u32 d = Get(instr.s.rt);

                    // check for special case
                    if (d == 0) {
                        sp.hi = n;
                        sp.lo = 0xFFFFFFFF;
                    } else {
                        sp.hi = n % d;
                        sp.lo = n / d;
                    }
                    break;
                }
                case SecondaryOpcode::add: {
                    const u32 a = Get(instr.s.rs);
                    const u32 b = Get(instr.s.rt);
                    const u32 result = a + b;
                    if (!((a ^ b) & 0x80000000) && ((result ^ a) & 0x80000000)) Exception(ExceptionCode::Overflow);
                    Set(instr.s.rd, result);
                    break;
                }
                case SecondaryOpcode::addu:
                    Set(instr.s.rd, Get(instr.s.rs) + Get(instr.s.rt));
                    break;
                case SecondaryOpcode::sub: {
                    const u32 a = Get(instr.s.rs);
                    const u32 b = Get(instr.s.rt);
                    const u32 result = a - b;
                    if (((a ^ b) & 0x80000000) && ((result ^ a) & 0x80000000)) Exception(ExceptionCode::Overflow);
                    Set(instr.s.rd, result);
                    break;
                }
                case SecondaryOpcode::subu:
                    Set(instr.s.rd, Get(instr.s.rs) - Get(instr.s.rt));
                    break;
                case SecondaryOpcode::andr:
                    Set(instr.s.rd, Get(instr.s.rs) & Get(instr.s.rt));
                    break;
                case SecondaryOpcode::orr:
                    Set(instr.s.rd, Get(instr.s.rs) | Get(instr.s.rt));
                    break;
                case SecondaryOpcode::xorr:
                    Set(instr.s.rd, Get(instr.s.rs) ^ Get(instr.s.rt));
                    break;
                case SecondaryOpcode::nor:
                    Set(instr.s.rd, ~(Get(instr.s.rs) | Get(instr.s.rt)));
                    break;
                case SecondaryOpcode::slt:
                    Set(instr.s.rd, (static_cast<s32>(Get(instr.s.rs)) < static_cast<s32>(Get(instr.s.rt))) ? 1 : 0);
                    break;
                case SecondaryOpcode::sltu:
                    Set(instr.s.rd, (Get(instr.s.rs) < Get(instr.s.rt)) ? 1 : 0);
                    break;
                default:
                    printf("Invalid special opcode 0x%02X [0x%08X]\n", (u32)instr.s.sop.GetValue(), instr.value);
                    Exception(ExceptionCode::ReservedInstr);
            }
            break;
        case PrimaryOpcode::bxxx: {
            const bool is_bgez = (instr.n.rt & 0x01) != 0;
            const bool is_link = (instr.n.rt & 0x1E) == 0x10;

            // check if lz
            bool test = static_cast<s32>(Get(instr.n.rs)) < 0;
            // flip check for gez
            test ^= is_bgez;

            in_delay_slot = true;
            if (is_link) gp.ra = next_pc;
            if (test) {
                next_pc = sp.pc + (instr.imm_se() << 2);
                branch_taken = true;
            }
            break;
        }
        case PrimaryOpcode::jmp:
            next_pc &= 0xF0000000;
            next_pc |= instr.jump_target << 2;
            in_delay_slot = true;
            branch_taken = true;
            break;
        case PrimaryOpcode::jal:
            gp.ra = next_pc;
            next_pc &= 0xF0000000;
            next_pc |= instr.jump_target << 2;
            in_delay_slot = true;
            branch_taken = true;
            break;
        case PrimaryOpcode::beq:
            in_delay_slot = true;
            if (Get(instr.n.rs) == Get(instr.n.rt)) {
                next_pc = sp.pc + (instr.imm_se() << 2);
                branch_taken = true;
            }
            break;
        case PrimaryOpcode::bne:
            in_delay_slot = true;
            if (Get(instr.n.rs) != Get(instr.n.rt)) {
                next_pc = sp.pc + (instr.imm_se() << 2);
                branch_taken = true;
            }
            break;
        case PrimaryOpcode::blez:
            in_delay_slot = true;
            if (static_cast<s32>(Get(instr.n.rs)) <= 0) {
                next_pc = sp.pc + (instr.imm_se() << 2);
                branch_taken = true;
            }
            break;
        case PrimaryOpcode::bgtz:
            in_delay_slot = true;
            if (static_cast<s32>(Get(instr.n.rs)) > 0) {
                next_pc = sp.pc + (instr.imm_se() << 2);
                branch_taken = true;
            }
            break;
        case PrimaryOpcode::addi: {
            const u32 old = Get(instr.n.rs);
            const u32 add = instr.imm_se();
            const u32 result = old + add;
            if (!((old ^ add) & 0x80000000) && ((result ^ old) & 0x80000000)) Exception(ExceptionCode::Overflow);
            Set(instr.n.rt, result);
            break;
        }
        case PrimaryOpcode::addiu:
            Set(instr.n.rt, Get(instr.n.rs) + instr.imm_se());
            break;
        case PrimaryOpcode::slti:
            Set(instr.n.rt, (static_cast<s32>(Get(instr.n.rs)) < static_cast<s32>(instr.imm_se())) ? 1 : 0);
            break;
        case PrimaryOpcode::sltiu:
            Set(instr.n.rt, (Get(instr.n.rs) < instr.imm_se()) ? 1 : 0);
            break;
        case PrimaryOpcode::andi:
            Set(instr.n.rt, Get(instr.n.rs) & instr.n.imm);
            break;
        case PrimaryOpcode::ori:
            Set(instr.n.rt, Get(instr.n.rs) | instr.n.imm);
            break;
        case PrimaryOpcode::xori:
            Set(instr.n.rt, Get(instr.n.rs) ^ instr.n.imm);
            break;
        case PrimaryOpcode::lui:
            Set(instr.n.rt, instr.n.imm << 16);
            break;
        case PrimaryOpcode::cop0:
            switch (instr.cop.cop_op) {
                case CoprocessorOpcode::mf:
                    SetDelayEntry(instr.cop.rt, GetCP0(instr.cop.rd));
                    break;
                case CoprocessorOpcode::mt:
                    SetCP0(instr.cop.rd, Get(instr.cop.rt));
                    break;
                case CoprocessorOpcode::rfe: {
                    if ((instr.value & 0x3F) != 0b010000) Panic("Invalid CP0 instruction 0x%08X!", instr.value);
                    // restore the interrupt/user pairs that we changed before jumping into the exception handler
                    const u32 mode = cp.sr & 0x3C;
                    cp.sr &= ~0xFu; // bits 4-5 are left unchanged
                    cp.sr |= (mode >> 2);
                    break;
                }
                default:
                    Panic("Invalid coprocessor opcode 0x%02X!", (u32)instr.cop.cop_op.GetValue());
            }
            break;
        case PrimaryOpcode::cop2:
            Panic("GTE not implemented");
            break;
        case PrimaryOpcode::cop1:
        case PrimaryOpcode::cop3:
            Exception(ExceptionCode::CopError);
            break;
        case PrimaryOpcode::lb: {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            u32 value = static_cast<s8>(Load8(address));
            SetDelayEntry(instr.n.rt, value);
            break;
        }
        case PrimaryOpcode::lh: {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            u32 value = static_cast<s16>(Load16(address));
            if (address & 0x1)
                Exception(ExceptionCode::LoadAddress);
            else
                SetDelayEntry(instr.n.rt, value);
            break;
        }
        case PrimaryOpcode::lwl:{
            if (cp.sr & 0x10000) Panic("Load with isolated cache");
            u32 address = Get(instr.n.rs) + instr.imm_se();

            u32 aligned_value = Load32(address & ~0x3);
            u32 old_value = (delay_entries[0].reg == instr.n.rt) ? delay_entries[0].value : Get(instr.n.rt);

            u32 new_value = 0;
            switch (address & 0x3) {
                case 0: new_value = (old_value & 0x00FFFFFF) | (aligned_value << 24); break;
                case 1: new_value = (old_value & 0x0000FFFF) | (aligned_value << 16); break;
                case 2: new_value = (old_value & 0x000000FF) | (aligned_value << 8); break;
                case 3: new_value = aligned_value; break;
            }
            SetDelayEntry(instr.n.rt, new_value);
            break;
        }
        case PrimaryOpcode::lw: {
            if (cp.sr & 0x10000) Panic("Load with isolated cache");
            u32 address = Get(instr.n.rs) + instr.imm_se();
            if (address & 0x3)
                Exception(ExceptionCode::LoadAddress);
            else
                SetDelayEntry(instr.n.rt, Load32(address));
            break;
        }
        case PrimaryOpcode::lbu: {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            SetDelayEntry(instr.n.rt, Load8(address));
            break;
        }
        case PrimaryOpcode::lhu: {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            if (address & 0x1)
                Exception(ExceptionCode::LoadAddress);
            else
                SetDelayEntry(instr.n.rt, Load16(address));
            break;
        }
        case PrimaryOpcode::lwr: {
            if (cp.sr & 0x10000) Panic("Load with isolated cache");
            u32 address = Get(instr.n.rs) + instr.imm_se();

            u32 aligned_value = Load32(address & ~0x3);
            u32 old_value = (delay_entries[0].reg == instr.n.rt) ? delay_entries[0].value : Get(instr.n.rt);

            u32 new_value = 0;
            switch (address & 0x3) {
                case 0: new_value = aligned_value; break;
                case 1: new_value = (old_value & 0xFF000000) | (aligned_value >> 8); break;
                case 2: new_value = (old_value & 0xFFFF0000) | (aligned_value >> 16); break;
                case 3: new_value = (old_value & 0xFFFFFF00) | (aligned_value >> 24); break;
            }
            SetDelayEntry(instr.n.rt, new_value);
            break;
        }
        case PrimaryOpcode::sb: {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            u8 byte = static_cast<u8>(Get(instr.n.rt) & 0xFF);
            Store8(address, byte);
            break;
        }
        case PrimaryOpcode::sh: {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            u16 halfword = static_cast<u16>(Get(instr.n.rt) & 0xFFFF);
            if (address & 0x1)
                Exception(ExceptionCode::StoreAddress);
            else
                Store16(address, halfword);
            break;
        }
        case PrimaryOpcode::swl: {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            u32 aligned_value = Load32(address & ~0x3);
            u32 old_value = Get(instr.n.rt);

            u32 new_value = 0;
            switch (address & 0x3) {
                case 0: new_value = (aligned_value & 0xFFFFFF00) | (old_value >> 24); break;
                case 1: new_value = (aligned_value & 0xFFFF0000) | (old_value >> 16); break;
                case 2: new_value = (aligned_value & 0xFF000000) | (old_value >> 8); break;
                case 3: new_value = old_value; break;
            }
            Store32(address & ~0x3, new_value);
            break;
        }
        case PrimaryOpcode::sw: {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            if (address & 0x3)
                Exception(ExceptionCode::StoreAddress);
            else
                Store32(address, Get(instr.n.rt));
            break;
        }
        case PrimaryOpcode::swr: {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            u32 aligned_value = Load32(address & ~0x3);
            u32 old_value = Get(instr.n.rt);

            u32 new_value = 0;
            switch (address & 0x3) {
                case 0: new_value = old_value; break;
                case 1: new_value = (aligned_value & 0x000000FF) | (old_value << 8); break;
                case 2: new_value = (aligned_value & 0x0000FFFF) | (old_value << 16); break;
                case 3: new_value = (aligned_value & 0x00FFFFFF) | (old_value << 24); break;
            }
            Store32(address & ~0x3, new_value);
            break;
        }
        case PrimaryOpcode::lwc2:
            Panic("LWC2 (GTE) not implemented");
            break;
        case PrimaryOpcode::swc2:
            Panic("SWC2 (GTE) not implemented");
            break;
        case PrimaryOpcode::lwc0:
        case PrimaryOpcode::lwc1:
        case PrimaryOpcode::lwc3:
        case PrimaryOpcode::swc0:
        case PrimaryOpcode::swc1:
            Exception(ExceptionCode::CopError);
            break;
        default:
            printf("Invalid opcode 0x%02X [0x%08X]\n", (u32)instr.n.op.GetValue(), instr.value);
            Exception(ExceptionCode::ReservedInstr);
    }

    UpdateDelayEntries();
}

void CPU::Exception(ExceptionCode cause) {
    const u32 handler = ((cp.sr & (1u << 22)) != 0) ? 0xBFC00180 : 0x80000080;

    // get interrupt/user pairs
    const u32 mode = cp.sr & 0x3F;
    // clear old values
    cp.sr &= ~0x3F;
    // update pair values
    cp.sr |= (mode << 2) & 0x3F;

    // TODO: interrupt pending bits?
    u32 old_cause = cp.cause & 0xFF00;
    cp.cause = static_cast<u32>(cause) << 2;
    cp.cause |= old_cause;
    cp.epc = current_pc;
    if (was_in_delay_slot) {
        cp.epc -= 4;
        cp.cause |= 1 << 31;
        cp.jumpdest = sp.pc;
        if (was_branch_taken) {
            cp.cause |= 1 << 30;
        }
    }

    sp.pc = handler;
    next_pc = handler + 4;
    printf("CPU Exception 0x%02X\n", (u32)cause);
}

u32 CPU::Load32(u32 address) { return bus->Load<u32>(address); }

void CPU::Store32(u32 address, u32 value) {
    if (cp.sr & 0x10000) return;
    bus->Store(address, value);
}

u16 CPU::Load16(u32 address) {
    if (cp.sr & 0x10000) Panic("Load with isolated cache");
    return bus->Load<u16>(address);
}

void CPU::Store16(u32 address, u16 value) {
    if (cp.sr & 0x10000) return;
    bus->Store(address, value);
}

u8 CPU::Load8(u32 address) {
    if (cp.sr & 0x10000) Panic("Load with isolated cache");
    return bus->Load<u8>(address);
}

void CPU::Store8(u32 address, u8 value) {
    if (cp.sr & 0x10000) return;
    bus->Store(address, value);
}

void CPU::Set(u32 index, u32 value) {
    Assert(index < 32);
    if (delay_entries[0].reg == index) {
        delay_entries[0].reg = 0;
        delay_entries[0].value = 0;
    }
    gp.r[index] = value;
    gp.zero = 0;
}

u32 CPU::Get(u32 index) {
    Assert(index < 32);
    return gp.r[index];
}

void CPU::SetCP0(u32 index, u32 value) {
    Assert(index < 16);
    // Assert(value == 0 || (index == 12 && value == 0x10000));
    cp.cpr[index] = value;
}

u32 CPU::GetCP0(u32 index) {
    // TODO: behaviour for CP0 registers 16-63
    Assert(index < 16);
    return cp.cpr[index];
}

void CPU::SetDelayEntry(u32 reg, u32 value) {
    Assert(reg < 32);
    //if (reg == 0) return;
    if (delay_entries[0].reg == reg) {
        delay_entries[0].reg = 0;
        delay_entries[0].value = 0;
    }

    delay_entries[1] = {reg, value};
}

void CPU::UpdateDelayEntries() {
    gp.r[delay_entries[0].reg] = delay_entries[0].value;
    delay_entries[0] = delay_entries[1];
    delay_entries[1] = {0, 0};
    gp.zero = 0;
}

void CPU::UpdatePC(u32 address) {
    current_pc = sp.pc;
    sp.pc = address;
    next_pc = address + 4;
}

}  // namespace CPU