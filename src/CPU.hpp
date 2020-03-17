#pragma once

#include "Types.hpp"
#include "CPU_Common.hpp"

class BUS;

namespace CPU {

class CPU {
public:
    CPU();
    void Init(BUS* bus);

    void Step();
    u32 Load32(u32 address);
    void Store32(u32 address, u32 value);

private:
    ALWAYS_INLINE PrimaryOpcode op(u32 instr) { return static_cast<PrimaryOpcode>(instr >> 26); }
    ALWAYS_INLINE u32 src(u32 instr) { return (instr >> 21) & 0x1F; }
    ALWAYS_INLINE u32 dst(u32 instr) { return (instr >> 16) & 0x1F; }
    ALWAYS_INLINE u32 imm(u32 instr) { return instr & 0xFFFF; }

    void SetReg(u32 index, u32 value);
    u32 GetReg(u32 index);

    GP_Registers gp;
    SP_Registers sp;

    BUS* bus = nullptr;
};

}  // namespace CPU