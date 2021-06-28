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
    void PrintInstructionWithConstant(const char* name, u32 constant, u32 r1 = 32, u32 r2 = 32);
    void PrintLoadStoreInstruction(const char* name, u32 rt, u32 base, s32 offset);
    void PrintCP0Instruction(const char* name, u32 reg1, u32 reg2);

    bool with_current_value = true;
    std::string result;
    CPU* cpu = nullptr;
};
}