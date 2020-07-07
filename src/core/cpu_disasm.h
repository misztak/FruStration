#pragma once

#include <string>

#include "types.h"

namespace CPU {

class CPU;

class Disassembler {
public:
    explicit Disassembler(CPU* cpu);
    std::string InstructionAt(u32 address, u32 value, bool with_curr_value = true);
private:
    void Disassemble(u32 address, u32 value);
    void PrintInstruction(const char* name, u32 r1, u32 r2 = 32, u32 r3 = 32);
    void PrintInstructionWithConstant(const char* name, u32 constant, u32 r1 = 32, u32 r2 = 32, u32 r3 = 32);
    void PrintLoadStoreInstruction(const char* name, u32 rt, u32 base, s32 offset);
    void PrintCP0Instruction(const char* name, u32 reg1, u32 reg2);

    const char* rnames[32] = {
        "zero", "at", "v0", "v1",
        "a0", "a1", "a2", "a3",
        "t0", "t1", "t2", "t3",
        "t4", "t5", "t6", "t7",
        "s0", "s1", "s2", "s3",
        "s4", "s5", "s6", "s7",
        "t8", "t9", "k0", "k1",
        "gp", "sp", "fp", "ra"
    };

    const char* coprnames[16] = {
        "0", "1", "2", "bpc",
        "4", "bda", "jumpdest", "dcic",
        "bad_vaddr", "bdam", "10", "bpcm",
        "sr", "cause", "epc", "prid"
    };

    bool with_current_value = true;
    std::string result;
    CPU* cpu = nullptr;
};
}