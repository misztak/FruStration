#pragma once

#include <vector>

#include "types.h"

namespace CPU {

class CPU;

class Disassembler {
public:
    explicit Disassembler(CPU* cpu);
    void DisassembleInstruction(u32 address, u32 value);
private:
    void PrintInstruction(const char* name, const std::vector<u32>& indices);
    void PrintInstructionWithConstant(const char* name, const std::vector<u32>& indices, u32 constant);
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

    CPU* cpu = nullptr;
};
}