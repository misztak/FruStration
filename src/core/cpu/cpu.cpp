#include "cpu.h"

#include "imgui.h"

#include "bios.h"
#include "bus.h"
#include "common/asserts.h"
#include "common/log.h"
#include "cpu_common.h"
#include "interrupt.h"
#include "system.h"

LOG_CHANNEL(CPU);

namespace CPU {

constexpr bool DISASM_INSTRUCTION = false;
constexpr bool TRACE_BIOS_CALLS = false;

CPU::CPU(System* system) : sys(system), disassembler(this) {
    cp.prid = 0x2;
    UpdatePC(0xBFC00000);
}

void CPU::Reset() {
    for (auto& r : gp.r) r = 0;
    for (auto& r : cp.cpr) r = 0;
    sp.hi = 0;
    sp.lo = 0;

    sp.pc = 0;
    // resets current_pc and next_pc
    UpdatePC(0xBFC00000);
    cp.prid = 0x2;

    was_in_delay_slot = false, in_delay_slot = false;
    was_branch_taken = false, branch_taken = false;
    pending_delay_entry = {0, 0}, new_delay_entry = {0, 0};
    instr.value = 0;

    gte.Reset();
}

void CPU::Step() {
    if (sys->debugger->IsBreakpoint(sp.pc)) {
        bool enabled = sys->debugger->IsBreakpointEnabled(sp.pc);
        sys->debugger->ToggleBreakpoint(sp.pc);

        if (enabled) {
            LogDebug("Hit breakpoint");
            halt = true;
            return;
        }
    }

    was_in_delay_slot = in_delay_slot;
    was_branch_taken = branch_taken;

    in_delay_slot = false;
    branch_taken = false;

    // handle interrupts
    if ((cp.cause.IP & cp.sr.IM) && cp.sr.interrupt_enable) {
        current_pc = sp.pc;
        Exception(ExceptionCode::Interrupt);
    }

    instr.value = Load32(sp.pc);

    halt = sys->debugger->single_step;
    sys->debugger->StoreLastInstruction(sp.pc, instr.value);

#ifndef NDEBUG
    if (TRACE_BIOS_CALLS && (sp.pc & 0x3FFFFFFF) <= 0xC0) {
        u32 masked_pc = sp.pc & 0x3FFFFFFF;
        if (masked_pc == 0xA0 || masked_pc == 0xB0 || masked_pc == 0xC0) [[unlikely]] {
            BIOS::TraceFunction(sys, sp.pc, Get(9));
        }
    }
    if (DISASM_INSTRUCTION) {
        LogDebug(disassembler.InstructionAt(sp.pc, instr.value));
    }
    //static u64 instr_counter = 0;
    //instr_counter++;
#endif

    // bios put_char calls
    if (sp.pc == 0xA0 && Get(9) == 0x3C) BIOS::PutChar(static_cast<u8>(Get(4)));
    if (sp.pc == 0xB0 && Get(9) == 0x3D) BIOS::PutChar(static_cast<u8>(Get(4)));

    UpdatePC(next_pc);
    // at this point the pc contains the address of the delay slot instruction
    // next_pc points to the instruction right after the delay slot

    // TODO: at what point should this be called/checked?
    if (current_pc & 0x3) [[unlikely]] {
        LogCrit("Invalid pc address");
        Exception(ExceptionCode::LoadAddress);
        return;
    }

    // clang-format off

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
                case SecondaryOpcode::jr:
                {
                    u32 jump_address = Get(instr.s.rs);
                    in_delay_slot = true;
                    if ((jump_address & 0x3) != 0) {
                        Exception(ExceptionCode::StoreAddress);
                        break;
                    }
                    next_pc = jump_address;
                    branch_taken = true;
                    break;
                }
                case SecondaryOpcode::jalr:
                {
                    u32 jump_address = Get(instr.s.rs);
                    Set(instr.s.rd, next_pc);
                    in_delay_slot = true;
                    if ((jump_address & 0x3) != 0) {
                        Exception(ExceptionCode::StoreAddress);
                        break;
                    }
                    next_pc = jump_address;
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
                case SecondaryOpcode::multu:
                {
                    const u64 a = Get(instr.s.rs);
                    const u64 b = Get(instr.s.rt);
                    const u64 result = a * b;

                    sp.lo = result & 0xFFFFFFFF;
                    sp.hi = result >> 32;
                    break;
                }
                case SecondaryOpcode::div:
                {
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
                case SecondaryOpcode::divu:
                {
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
                case SecondaryOpcode::add:
                {
                    const u32 a = Get(instr.s.rs);
                    const u32 b = Get(instr.s.rt);
                    const u32 result = a + b;
                    if (!((a ^ b) & 0x80000000) && ((result ^ a) & 0x80000000))
                        Exception(ExceptionCode::Overflow);
                    else
                        Set(instr.s.rd, result);
                    break;
                }
                case SecondaryOpcode::addu:
                    Set(instr.s.rd, Get(instr.s.rs) + Get(instr.s.rt));
                    break;
                case SecondaryOpcode::sub:
                {
                    const u32 a = Get(instr.s.rs);
                    const u32 b = Get(instr.s.rt);
                    const u32 result = a - b;
                    if (((a ^ b) & 0x80000000) && ((result ^ a) & 0x80000000))
                        Exception(ExceptionCode::Overflow);
                    else
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
                    LogCrit("Invalid special opcode 0x%02X [0x%08X]\n", (u32)instr.s.sop.GetValue(), instr.value);
                    Exception(ExceptionCode::ReservedInstr);
            }
            break;
        case PrimaryOpcode::bxxx:
        {
            const bool is_bgez = (instr.n.rt & 0x01) != 0;
            const bool is_link = (instr.n.rt & 0x1E) == 0x10;

            // check if lz
            bool test = static_cast<s32>(Get(instr.n.rs)) < 0;
            // flip check for gez
            test ^= is_bgez;

            in_delay_slot = true;
            if (is_link) Set(GP_Registers::RA, next_pc);
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
            Set(GP_Registers::RA, next_pc);
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
        case PrimaryOpcode::addi:
        {
            const u32 old = Get(instr.n.rs);
            const u32 add = instr.imm_se();
            const u32 result = old + add;
            if (!((old ^ add) & 0x80000000) && ((result ^ old) & 0x80000000))
                Exception(ExceptionCode::Overflow);
            else
                Set(instr.n.rt, result);
            break;
        }
        case PrimaryOpcode::addiu: Set(instr.n.rt, Get(instr.n.rs) + instr.imm_se()); break;
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
                    //LOG_DEBUG << fmt::format("MTC: Set Register {} to 0x{:08X}", instr.cop.rd, Get(instr.cop.rt));
                    SetCP0(instr.cop.rd, Get(instr.cop.rt));
                    break;
                case CoprocessorOpcode::rfe:
                {
                    if ((instr.value & 0x3F) != 0b010000) Panic("Invalid CP0 instruction 0x{:08X}!", instr.value);
                    // restore the interrupt/user pairs that we changed before jumping into the exception handler
                    const u32 mode = cp.sr.value & 0x3C;
                    cp.sr.value &= ~0xFu;    // bits 4-5 are left unchanged
                    cp.sr.value |= (mode >> 2);
                    break;
                }
                default:
                    Panic("Invalid coprocessor opcode 0x{:02X}!", (u32) instr.cop.cop_op.GetValue());
            }
            break;
        case PrimaryOpcode::cop2:
            if ((instr.value >> 25) == COP2_IMM_OPCODE) {
                gte.ExecuteCommand(instr.value);
                break;
            }
            switch (instr.cop.cop_op) {
                case CoprocessorOpcode::mf:
                    Set(instr.cop.rt, gte.GetReg(instr.cop.rd));
                    break;
                case CoprocessorOpcode::mcf:
                    Set(instr.cop.rt, gte.GetReg(instr.cop.rd + 32));
                    break;
                case CoprocessorOpcode::mt:
                    gte.SetReg(instr.cop.rd, Get(instr.cop.rt));
                    break;
                case CoprocessorOpcode::mct:
                    gte.SetReg(instr.cop.rd + 32, Get(instr.cop.rt));
                    break;
                default:
                    Panic("Invalid GTE coprocessor opcode 0x{:02X}!", (u32) instr.cop.cop_op.GetValue());
            }
            break;
        case PrimaryOpcode::cop1:
        case PrimaryOpcode::cop3:
            Exception(ExceptionCode::CopError);
            break;
        case PrimaryOpcode::lb:
        {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            u32 value = static_cast<s8>(Load8(address));
            SetDelayEntry(instr.n.rt, value);
            break;
        }
        case PrimaryOpcode::lh:
        {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            u32 value = static_cast<s16>(Load16(address));
            if (address & 0x1)
                Exception(ExceptionCode::LoadAddress);
            else
                SetDelayEntry(instr.n.rt, value);
            break;
        }
        case PrimaryOpcode::lwl:
        {
            if (cp.sr.isolate_cache) Panic("Load with isolated cache");
            u32 address = Get(instr.n.rs) + instr.imm_se();

            u32 aligned_value = Load32(address & ~0x3);
            u32 old_value = (pending_delay_entry.reg == instr.n.rt) ? pending_delay_entry.value : Get(instr.n.rt);

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
        case PrimaryOpcode::lw:
        {
            if (cp.sr.isolate_cache) Panic("Load with isolated cache");
            u32 address = Get(instr.n.rs) + instr.imm_se();
            if (address & 0x3)
                Exception(ExceptionCode::LoadAddress);
            else
                SetDelayEntry(instr.n.rt, Load32(address));
            break;
        }
        case PrimaryOpcode::lbu:
        {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            SetDelayEntry(instr.n.rt, Load8(address));
            break;
        }
        case PrimaryOpcode::lhu:
        {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            if (address & 0x1)
                Exception(ExceptionCode::LoadAddress);
            else
                SetDelayEntry(instr.n.rt, Load16(address));
            break;
        }
        case PrimaryOpcode::lwr:
        {
            if (cp.sr.isolate_cache) Panic("Load with isolated cache");
            u32 address = Get(instr.n.rs) + instr.imm_se();

            u32 aligned_value = Load32(address & ~0x3);
            u32 old_value = (pending_delay_entry.reg == instr.n.rt) ? pending_delay_entry.value : Get(instr.n.rt);

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
        case PrimaryOpcode::sb:
        {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            u8 byte = static_cast<u8>(Get(instr.n.rt) & 0xFF);
            Store8(address, byte);
            break;
        }
        case PrimaryOpcode::sh:
        {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            u16 halfword = static_cast<u16>(Get(instr.n.rt) & 0xFFFF);
            if (address & 0x1)
                Exception(ExceptionCode::StoreAddress);
            else
                Store16(address, halfword);
            break;
        }
        case PrimaryOpcode::swl:
        {
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
        case PrimaryOpcode::sw:
        {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            if (address & 0x3)
                Exception(ExceptionCode::StoreAddress);
            else
                Store32(address, Get(instr.n.rt));
            break;
        }
        case PrimaryOpcode::swr:
        {
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
        {
            u32 address = Get(instr.n.rs) + instr.imm_se();
            u32 value = Load32(address);

            // TODO: wait until last GTE command is done before writing to the register
            gte.SetReg(instr.n.rt, value);

            break;
        }
        case PrimaryOpcode::swc2:
        {
            u32 address = Get(instr.n.rs) + instr.imm_se();

            // TODO: wait until last GTE command is done before reading from the register
            u32 value = gte.GetReg(instr.n.rt);
            Store32(address, value);

            break;
        }
        case PrimaryOpcode::lwc0:
        case PrimaryOpcode::lwc1:
        case PrimaryOpcode::lwc3:
        case PrimaryOpcode::swc0:
        case PrimaryOpcode::swc1:
            Exception(ExceptionCode::CopError);
            break;
        default:
            LogCrit("Invalid opcode 0x{:02X} [0x{:08X}]", (u32)instr.n.op.GetValue(), instr.value);
            Exception(ExceptionCode::ReservedInstr);
    }

    // clang-format on

    // update delay entries
    gp.r[pending_delay_entry.reg] = pending_delay_entry.value;
    pending_delay_entry = new_delay_entry;
    new_delay_entry = {0, 0};

    // first register always contains 0
    gp.zero = 0;

    // tick the components (2 is a bad approximation but seems to be better than 1 for now)
    sys->AddCycles(2);
}

void CPU::Exception(ExceptionCode cause) {
    if (cause == ExceptionCode::Interrupt) {
        Instruction i;
        i.value = Load32(sp.pc);
        if (instr.n.op == PrimaryOpcode::cop2) {
            LogDebug("GTE command during interrupt, delaying interrupt");
            return;
        }
    }

    const u32 handler = ((cp.sr.value & (1u << 22)) != 0) ? 0xBFC00180 : 0x80000080;

    // get interrupt/user pairs
    const u32 mode = cp.sr.value & 0x3F;
    // clear old values
    cp.sr.value &= ~0x3F;
    // update pair values
    cp.sr.value |= (mode << 2) & 0x3F;

    // TODO: interrupt pending bits
    // TODO: CE
    cp.cause.excode = static_cast<u32>(cause) & 0x1F;
    cp.epc = current_pc;
    if (was_in_delay_slot) {
        cp.epc -= 4;
        cp.cause.BD = 1;
        cp.jumpdest = sp.pc;
        if (was_branch_taken) {
            cp.cause.value |= 1 << 30;
        }
    }

    // TODO: bad vaddr
    //if (cause == ExceptionCode::LoadAddress || cause == ExceptionCode::StoreAddress) cp.bad_vaddr = bad_vaddr;

    LogDebug("EXCEPTION - Cause: {} (0x{:02X}) - in delay slot: {} - in branch: {}", EXCEPT_NAMES[(u32)cause],
             (u32)cause, was_in_delay_slot, was_branch_taken);
    LogDebug("    Interrupted at 0x{:08X} - jumping to handler at 0x{:08X}", current_pc, handler);
    if (cause == ExceptionCode::Interrupt)
        LogDebug("    IRQ Source: {}", InterruptController::GetInterruptName(sys->interrupt->last_irq));
    if (cause == ExceptionCode::Syscall)
        LogDebug("    Syscall Type: {}", gp.a0 < 5 ? SYSCALL_NAMES[gp.a0] : SYSCALL_NAMES[4]);

    sp.pc = handler;
    next_pc = handler + 4;
}

u32 CPU::Load32(u32 address) {
    return sys->bus->Load<u32>(address);
}

void CPU::Store32(u32 address, u32 value) {
    if (cp.sr.isolate_cache) return;
    sys->bus->Store(address, value);
}

u16 CPU::Load16(u32 address) {
    if (cp.sr.isolate_cache) Panic("Load with isolated cache");
    return sys->bus->Load<u16>(address);
}

void CPU::Store16(u32 address, u16 value) {
    if (cp.sr.isolate_cache) return;
    sys->bus->Store(address, value);
}

u8 CPU::Load8(u32 address) {
    if (cp.sr.isolate_cache) Panic("Load with isolated cache");
    return sys->bus->Load<u8>(address);
}

void CPU::Store8(u32 address, u8 value) {
    if (cp.sr.isolate_cache) return;
    sys->bus->Store(address, value);
}

void CPU::Set(u32 index, u32 value) {
    Assert(index < 32);
    // overwrite delay entry if it's in the same register
    if (pending_delay_entry.reg == index) {
        pending_delay_entry.reg = 0;
        pending_delay_entry.value = 0;
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
    // TODO: only allow setting writable registers
    if (index == 13) {
        LogDebug("Set value of CAUSE.IP to {}", (value & 0x300U) >> 8);
        // only bits 8-9 of CAUSE are writable
        cp.cause.value = (cp.cause.value & ~0x300U) | (value & 0x300U);
        return;
    }
    cp.cpr[index] = value;
}

u32 CPU::GetCP0(u32 index) {
    Assert(index < 16);
    return cp.cpr[index];
}

void CPU::SetDelayEntry(u32 reg, u32 value) {
    Assert(reg < 32);
    //if (reg == 0) return;
    // overwrite delay entry if it's in the same register
    if (pending_delay_entry.reg == reg) {
        pending_delay_entry.reg = 0;
        pending_delay_entry.value = 0;
    }

    new_delay_entry.reg = reg;
    new_delay_entry.value = value;
}

void CPU::UpdatePC(u32 address) {
    current_pc = sp.pc;
    sp.pc = address;
    next_pc = address + 4;
}

void CPU::DrawCpuState(bool* open) {
    ImGui::Begin("CPU State", open);
    ImGui::Separator();

    if (ImGui::TreeNode("Edit")) {
        static int curr_gp_reg = 0;
        ImGui::Text("GP Registers ");
        ImGui::SameLine();
        ImGui::Combo("##gp_reg_list", &curr_gp_reg, REG_NAMES, GP_REG_COUNT, 10);
        ImGui::SameLine();
        ImGui::InputScalar("##gp_reg_edit", ImGuiDataType_U32, &gp.r[curr_gp_reg], nullptr, nullptr, "%08X",
                           ImGuiInputTextFlags_CharsHexadecimal);

        static int curr_sp_reg = 0;
        ImGui::Text("SP Registers ");
        ImGui::SameLine();
        ImGui::Combo("##sp_reg_list", &curr_sp_reg, SP_REG_NAMES, SP_REG_COUNT);
        ImGui::SameLine();
        ImGui::InputScalar("##sp_reg_edit", ImGuiDataType_U32, &sp.spr[curr_sp_reg], nullptr, nullptr, "%08X",
                           ImGuiInputTextFlags_CharsHexadecimal);

        static int curr_cop_reg = 0;
        ImGui::Text("COP0 Registers ");
        ImGui::SameLine();
        ImGui::Combo("##cop_reg_list", &curr_cop_reg, COP0_REG_NAMES, COP_REG_COUNT, 10);
        ImGui::SameLine();
        ImGui::InputScalar("##cop_reg_edit", ImGuiDataType_U32, &cp.cpr[curr_cop_reg], nullptr, nullptr, "%08X",
                           ImGuiInputTextFlags_CharsHexadecimal);

        ImGui::TreePop();
    }

    ImGui::Separator();

    ImGui::Text("GP Registers");
    ImGui::Columns(4);
    for (u32 col = 0; col < 4; col++) {
        for (u32 row = 0; row < 8; row++) {
            const u32 index = row + col * 8;
            ImGui::Text(" $%-3s %08X", REG_NAMES[index], gp.r[row + col * 8]);
        }
        ImGui::NextColumn();
    }
    ImGui::Columns(1);

    ImGui::Separator();
    ImGui::Text("SP Registers");
    ImGui::Columns(4);
    ImGui::Text(" PC   %08X", sp.pc);
    ImGui::NextColumn();
    ImGui::Text(" HI   %08X", sp.hi);
    ImGui::NextColumn();
    ImGui::Text(" LO   %08X", sp.lo);
    ImGui::Columns(1);

    ImGui::Separator();
    ImGui::Text("COP0 Registers");
    ImGui::Columns(4);
    for (u32 col = 0; col < 4; col++) {
        for (u32 row = 0; row < 4; row++) {
            const u32 index = row + col * 4;
            ImGui::Text(" $%-10s %08X", COP0_REG_NAMES[index], cp.cpr[row + col * 4]);
        }
        ImGui::NextColumn();
    }
    ImGui::Columns(1);

    ImGui::End();
}

}    // namespace CPU