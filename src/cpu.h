#pragma once

#include "cpu_common.h"

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
    void Set(u32 index, u32 value);
    u32 Get(u32 index);
    void SetCP0(u32 index, u32 value);
    u32 GetCP0(u32 index);

    GP_Registers gp;
    SP_Registers sp;
    CP0_Registers cp;

    Instruction instr, next_instr;

    BUS* bus = nullptr;
};

}  // namespace CPU