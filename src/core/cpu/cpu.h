#pragma once

#include "cpu_common.h"
#include "cpu_disasm.h"
#include "debugger.h"
#include "gte.h"

#include "bios.h"

class BUS;
class System;
class Debugger;

// TODO: remove CPU namespace
namespace CPU {

class CPU {
    friend class Disassembler;
    friend class ::Debugger;
    friend class ::BUS;

public:
    CPU(System* system);
    void Reset();

    void Step();
    u32 Load32(u32 address);
    void Store32(u32 address, u32 value);
    u16 Load16(u32 address);
    void Store16(u32 address, u16 value);
    u8 Load8(u32 address);
    void Store8(u32 address, u8 value);

    void DrawCpuState(bool* open);

    bool halt = false;

    GP_Registers gp;
    SP_Registers sp;
    CP0_Registers cp;

private:
    void Set(u32 index, u32 value);
    u32 Get(u32 index);
    void SetCP0(u32 index, u32 value);
    u32 GetCP0(u32 index);

    void SetDelayEntry(u32 reg, u32 value);
    void UpdatePC(u32 address);

    void Exception(ExceptionCode cause);

    u32 next_pc = 0, current_pc = 0;
    bool branch_taken = false, was_branch_taken = false;
    bool in_delay_slot = false, was_in_delay_slot = false;

    LoadDelayEntry pending_delay_entry = {}, new_delay_entry = {};
    Instruction instr;

    System* sys = nullptr;

    GTE gte;

    BIOS bios;
    Disassembler disassembler;
};

}    // namespace CPU