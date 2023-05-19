#pragma once

#include "util/bitfield.h"
#include "util/types.h"

class GTE {
public:
    void Reset();

    void ExecuteCommand(u32 cmd);

    void SetReg(u32 index, u32 value);
    u32 GetReg(u32 index);

private:

    union {
        BitField<u32, u32, 0, 5> cmd;
        BitField<u32, u32, 10, 1> lm;
        BitField<u32, u32, 13, 2> mvmva_t_vec;
        BitField<u32, u32, 15, 2> mvmva_m_vec;
        BitField<u32, u32, 17, 2> mvmva_m_mat;
        BitField<u32, u32, 19, 1> sf;
        BitField<u32, u32, 25, 7> opcode;

        u32 value = 0;
    } command;

};