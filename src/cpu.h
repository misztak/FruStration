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
    u16 Load16(u32 address);
    void Store16(u32 address, u16 value);
    u8 Load8(u32 address);
    void Store8(u32 address, u8 value);
private:
    void Set(u32 index, u32 value);
    u32 Get(u32 index);
    void SetCP0(u32 index, u32 value);
    u32 GetCP0(u32 index);

    void SetDelayEntry(u32 reg, u32 value);
    void UpdateDelayEntries();

    GP_Registers gp;
    SP_Registers sp;
    CP0_Registers cp;
    //u32 next_pc = sp.pc;

    LoadDelayEntry delay_entries[2] = {};
    Instruction instr, next_instr;

    BUS* bus = nullptr;
};

}  // namespace CPU