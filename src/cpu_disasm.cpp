#include "cpu_disasm.h"

#include <string>

#include "cpu.h"
#include "cpu_common.h"
#include "fmt/format.h"

namespace CPU {

Disassembler::Disassembler(CPU* cpu) : cpu(cpu) {}

void Disassembler::DisassembleInstruction(u32 address, u32 value) {
    Instruction i{value};

    fmt::print("{:08x}: {:08x} ", address, value);

    switch (i.n.op) {
        case PrimaryOpcode::special:
            switch (i.s.sop) {
                case SecondaryOpcode::sll:
                    PrintInstruction("sll", i.s.rd, i.s.rt);
                    break;
                case SecondaryOpcode::srl:
                    PrintInstruction("srl", i.s.rd, i.s.rt);
                    break;
                case SecondaryOpcode::sra:
                    PrintInstruction("sra", i.s.rd, i.s.rt);
                    break;
                case SecondaryOpcode::sllv:
                    PrintInstruction("sllv", i.s.rd, i.s.rt, i.s.rs);
                    break;
                case SecondaryOpcode::srlv:
                    PrintInstruction("srlv", i.s.rd, i.s.rt, i.s.rs);
                    break;
                case SecondaryOpcode::srav:
                    PrintInstruction("srav", i.s.rd, i.s.rt, i.s.rs);
                    break;
                case SecondaryOpcode::jr:
                    PrintInstruction("jr", i.s.rs);
                    break;
                case SecondaryOpcode::jalr:
                    PrintInstruction("jalr", i.s.rd, i.s.rs);
                    break;
                case SecondaryOpcode::syscall:
                    fmt::print("syscall");
                    break;
                case SecondaryOpcode::breakpoint:
                    fmt::print("breakpoint");
                    break;
                case SecondaryOpcode::mfhi:
                    PrintInstruction("mfhi", i.s.rd);
                    break;
                case SecondaryOpcode::mthi:
                    PrintInstruction("mthi", i.s.rs);
                    break;
                case SecondaryOpcode::mflo:
                    PrintInstruction("mflo", i.s.rd);
                    break;
                case SecondaryOpcode::mtlo:
                    PrintInstruction("mtlo", i.s.rs);
                    break;
                case SecondaryOpcode::mult:
                    PrintInstruction("mult", i.s.rs, i.s.rt);
                    break;
                case SecondaryOpcode::multu:
                    PrintInstruction("multu", i.s.rs, i.s.rt);
                    break;
                case SecondaryOpcode::div:
                    PrintInstruction("div", i.s.rs, i.s.rt);
                    break;
                case SecondaryOpcode::divu:
                    PrintInstruction("divu", i.s.rs, i.s.rt);
                    break;
                case SecondaryOpcode::add:
                    PrintInstruction("add", i.s.rd, i.s.rs, i.s.rt);
                    break;
                case SecondaryOpcode::addu:
                    PrintInstruction("addu", i.s.rd, i.s.rs, i.s.rt);
                    break;
                case SecondaryOpcode::sub:
                    PrintInstruction("sub", i.s.rd, i.s.rs, i.s.rt);
                    break;
                case SecondaryOpcode::subu:
                    PrintInstruction("subu", i.s.rd, i.s.rs, i.s.rt);
                    break;
                case SecondaryOpcode::andr:
                    PrintInstruction("and", i.s.rd, i.s.rs, i.s.rt);
                    break;
                case SecondaryOpcode::orr:
                    PrintInstruction("or", i.s.rd, i.s.rs, i.s.rt);
                    break;
                case SecondaryOpcode::xorr:
                    PrintInstruction("xor", i.s.rd, i.s.rs, i.s.rt);
                    break;
                case SecondaryOpcode::nor:
                    PrintInstruction("nor", i.s.rd, i.s.rs, i.s.rt);
                    break;
                case SecondaryOpcode::slt:
                    PrintInstruction("slt", i.s.rd, i.s.rs, i.s.rt);
                    break;
                case SecondaryOpcode::sltu:
                    PrintInstruction("sltu", i.s.rd, i.s.rs, i.s.rt);
                    break;
            }
            break;
        case PrimaryOpcode::bxxx:
            const char* name;
            switch (i.n.rt) {
                case 0x00: name = "bltz"; break;
                case 0x10: name = "bltzal"; break;
                case 0x01: name = "bgez"; break;
                case 0x11: name = "bgezal"; break;
                default: Panic("Invalid bxxx opcode");
            }
            PrintInstructionWithConstant(name, cpu->sp.pc + (i.imm_se() << 2), i.n.rs);
            break;
        case PrimaryOpcode::jmp:
            PrintInstructionWithConstant("jmp", (cpu->next_pc & 0xF0000000) | (i.jump_target << 2));
            break;
        case PrimaryOpcode::jal:
            PrintInstructionWithConstant("jal", (cpu->next_pc & 0xF0000000) | (i.jump_target << 2));
            break;
        case PrimaryOpcode::beq:
            PrintInstructionWithConstant("beq", cpu->sp.pc + (i.imm_se() << 2), i.n.rs, i.n.rt);
            break;
        case PrimaryOpcode::bne:
            PrintInstructionWithConstant("bne", cpu->sp.pc + (i.imm_se() << 2), i.n.rs, i.n.rt);
            break;
        case PrimaryOpcode::blez:
            PrintInstructionWithConstant("blez", cpu->sp.pc + (i.imm_se() << 2), i.n.rs);
            break;
        case PrimaryOpcode::bgtz:
            PrintInstructionWithConstant("bgtz", cpu->sp.pc + (i.imm_se() << 2), i.n.rs);
            break;
        case PrimaryOpcode::addi:
            PrintInstructionWithConstant("addi", i.imm_se(), i.n.rt, i.n.rs);
            break;
        case PrimaryOpcode::addiu:
            PrintInstructionWithConstant("addiu", i.imm_se(), i.n.rt, i.n.rs);
            break;
        case PrimaryOpcode::slti:
            PrintInstructionWithConstant("slti", i.imm_se(), i.n.rt, i.n.rs);
            break;
        case PrimaryOpcode::sltiu:
            PrintInstructionWithConstant("sltiu", i.imm_se(), i.n.rt, i.n.rs);
            break;
        case PrimaryOpcode::andi:
            PrintInstructionWithConstant("andi", i.imm_se(), i.n.rt, i.n.rs);
            break;
        case PrimaryOpcode::ori:
            PrintInstructionWithConstant("ori", i.imm_se(), i.n.rt, i.n.rs);
            break;
        case PrimaryOpcode::xori:
            PrintInstructionWithConstant("xori", i.imm_se(), i.n.rt, i.n.rs);
            break;
        case PrimaryOpcode::lui:
            PrintInstructionWithConstant("lui", i.n.imm, i.n.rt);
            break;
        case PrimaryOpcode::cop0:
            switch (i.cop.cop_op) {
                case CoprocessorOpcode::mf:
                    PrintCP0Instruction("mfc0", i.cop.rt, i.cop.rd);
                    break;
                case CoprocessorOpcode::mt:
                    PrintCP0Instruction("mtc0", i.cop.rt, i.cop.rd);
                    break;
                case CoprocessorOpcode::rfe:
                    fmt::print("rfe");
                    break;
            }
            break;
        case PrimaryOpcode::cop2:
            fmt::print("gte");
            break;
        case PrimaryOpcode::lb:
            PrintLoadStoreInstruction("lb", i.n.rt, i.n.rs, (s32)i.imm_se());
            break;
        case PrimaryOpcode::lh:
            PrintLoadStoreInstruction("lh", i.n.rt, i.n.rs, (s32)i.imm_se());
            break;
        case PrimaryOpcode::lwl:
            PrintLoadStoreInstruction("lwl", i.n.rt, i.n.rs, (s32)i.imm_se());
            break;
        case PrimaryOpcode::lw:
            PrintLoadStoreInstruction("lw", i.n.rt, i.n.rs, (s32)i.imm_se());
            break;
        case PrimaryOpcode::lbu:
            PrintLoadStoreInstruction("lbu", i.n.rt, i.n.rs, (s32)i.imm_se());
            break;
        case PrimaryOpcode::lhu:
            PrintLoadStoreInstruction("lhu", i.n.rt, i.n.rs, (s32)i.imm_se());
            break;
        case PrimaryOpcode::lwr:
            PrintLoadStoreInstruction("lwr", i.n.rt, i.n.rs, (s32)i.imm_se());
            break;
        case PrimaryOpcode::sb:
            PrintLoadStoreInstruction("sb", i.n.rt, i.n.rs, (s32)i.imm_se());
            break;
        case PrimaryOpcode::sh:
            PrintLoadStoreInstruction("sh", i.n.rt, i.n.rs, (s32)i.imm_se());
            break;
        case PrimaryOpcode::swl:
            PrintLoadStoreInstruction("swl", i.n.rt, i.n.rs, (s32)i.imm_se());
            break;
        case PrimaryOpcode::sw:
            PrintLoadStoreInstruction("sw", i.n.rt, i.n.rs, (s32)i.imm_se());
            break;
        case PrimaryOpcode::swr:
            PrintLoadStoreInstruction("swr", i.n.rt, i.n.rs, (s32)i.imm_se());
            break;
        case PrimaryOpcode::lwc2:
            fmt::print("lwc2");
            break;
        case PrimaryOpcode::swc2:
            fmt::print("swc2");
            break;
        case PrimaryOpcode::lwc0:
        case PrimaryOpcode::lwc1:
        case PrimaryOpcode::lwc3:
        case PrimaryOpcode::swc0:
        case PrimaryOpcode::swc1:
        case PrimaryOpcode::cop1:
        case PrimaryOpcode::cop3:
            break;
        default:
            Panic("Invalid opcode");
    }

    fmt::print("\n");
}

void Disassembler::PrintInstruction(const char* name, u32 r1, u32 r2, u32 r3) {
    std::string string = fmt::format("{} ${}", name, rnames[r1]);
    if (r2 != 32) string.append(fmt::format(" ${}", rnames[r2]));
    if (r3 != 32) string.append(fmt::format(" ${}", rnames[r3]));

    string.append(fmt::format("{:>{}}# ", " ", 40 - string.size()));

    string.append(fmt::format("{}=0x{:X}", rnames[r1], cpu->gp.r[r1]));
    if (r2 != 32) string.append(fmt::format(", {}=0x{:X}", rnames[r2], cpu->gp.r[r2]));
    if (r3 != 32) string.append(fmt::format(", {}=0x{:X}", rnames[r3], cpu->gp.r[r3]));

    fmt::print(string);
}

void Disassembler::PrintInstructionWithConstant(const char* name, u32 constant, u32 r1, u32 r2, u32 r3) {
    std::string string = fmt::format("{}", name);
    if (r1 != 32) string.append(fmt::format(" ${}", rnames[r1]));
    if (r2 != 32) string.append(fmt::format(" ${}", rnames[r2]));
    if (r3 != 32) string.append(fmt::format(" ${}", rnames[r3]));
    string.append(fmt::format(" 0x{:08X}", constant));

    string.append(fmt::format("{:>{}}# ", " ", 40 - string.size()));

    if (r1 != 32) string.append(fmt::format("{}=0x{:X}", rnames[r1], cpu->gp.r[r1]));
    if (r2 != 32) string.append(fmt::format(", {}=0x{:X}", rnames[r2], cpu->gp.r[r2]));
    if (r3 != 32) string.append(fmt::format(", {}=0x{:X}", rnames[r3], cpu->gp.r[r3]));

    fmt::print(string);
}

void Disassembler::PrintLoadStoreInstruction(const char* name, u32 rt, u32 base, s32 offset) {
    std::string string = fmt::format("{} ${} {}(${})", name, rnames[rt], offset, rnames[base]);
    string.append(fmt::format("{:>{}}# {}=0x{:X}, address=0x{:08X}", " ", 40 - string.size(), rnames[rt], cpu->gp.r[rt],
                              cpu->gp.r[base] + offset));
    fmt::print(string);
}

void Disassembler::PrintCP0Instruction(const char* name, u32 reg1, u32 reg2) {
    std::string string = fmt::format("{} ${} ${}", name, rnames[reg1], coprnames[reg2]);
    string.append(fmt::format("{:>{}}# {}(GPR)=0x{:08X}, {}(COP0)=0x{:08X}", " ", 40 - string.size(), rnames[reg1],
                              cpu->gp.r[reg1], coprnames[reg2], cpu->cp.cpr[reg2]));
    fmt::print(string);
}

}  // namespace CPU